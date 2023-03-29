/*
 * GPS.cpp
 *
 * Copyright 2018 Christopher B. Liebman
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *  Created on: Feb 26, 2018
 *      Author: chris.l
 */

#include <functional>
#include <cstdarg>
#include <cinttypes>
#include <stdio.h>
#include <stdarg.h>
#include "gps.h"



static const char* TAG = "gps";

static std::function<void()> _pps;

const uint8_t mt_set_speed[] = MT_SET_SPEED;
const uint8_t mt_set_timing_product[] = MT_SET_TIMING_PRODUCT;
const uint8_t mt_set_pps_nmea[] = MT_SET_PPS_NMEA;

static __isr void __time_critical_func(_pps_isr)(unsigned int gpio, long unsigned int mask)
{
    if (_pps){
        _pps();
    }
}

GPS::GPS() :
    _uart(GPS_UART),
    _valid_count(0),
    _min_micros(0),
    _max_micros(0),
    _last_micros(0),
    _timeouts(0),
    _valid(false),
    _nmea_timestamp_us(0),
    _pps_timestamp_us(0)
{
    _reason[0] = '\0';
    memset(&_buf, 0x0, sizeof(_buf));

}

GPS::~GPS()
{
    end();
}

void GPS::begin()
{
    _pps = std::bind( &GPS::pps, this);

    //Configure GPS UART
    gpio_set_function(PIN_GPS_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_GPS_RX, GPIO_FUNC_UART);

    uart_init(GPS_UART, GPS_UART_INITIAL_BAUD);
    uart_set_translate_crlf(GPS_UART, 0);

    sleep_ms(50);
    configure_mtk();
    configure_ubx();

    //Only if 9600 baud, that way we don't need to coldstart every time.
    uart_write_blocking(GPS_UART, (uint8_t *) MT_SET_SPEED_ALT, strlen(MT_SET_SPEED_ALT));
    uart_tx_wait_blocking(GPS_UART);
    uart_write_blocking(GPS_UART, (uint8_t *) MT_FULL_COLD_START, strlen(MT_FULL_COLD_START));
    uart_tx_wait_blocking(GPS_UART);

    uart_set_baudrate(GPS_UART, GPS_UART_BAUD);
    sleep_ms(50);
    configure_mtk();
    configure_ubx();

    
#ifdef FAMILY_ESP32
    
    pinMode(PIN_PPS, INPUT);
    attachInterrupt(PIN_PPS, _pps_isr, RISING);
#endif
#ifdef FAMILY_RP2040
    gpio_set_dir(PIN_PPS, false);
    gpio_set_irq_enabled_with_callback(PIN_PPS, GPIO_IRQ_EDGE_RISE, true, _pps_isr);
    //irq_set_exclusive_handler(IO_IRQ_BANK0, _pps_isr);
    //gpio_set_irq_enabled(PIN_PPS, GPIO_IRQ_EDGE_RISE, true);
    //irq_set_enabled(IO_IRQ_BANK0, true); 
#endif
}
void GPS::configure_mtk(){
    uart_write_blocking(GPS_UART, mt_set_timing_product, strlen(MT_SET_TIMING_PRODUCT));
    uart_tx_wait_blocking(GPS_UART);
    while(uart_is_readable(GPS_UART))
        uart_getc(GPS_UART);

    uart_write_blocking(GPS_UART, mt_set_pps_nmea, strlen(MT_SET_PPS_NMEA));
    uart_tx_wait_blocking(GPS_UART);
    while(uart_is_readable(GPS_UART))
        uart_getc(GPS_UART);

    uart_write_blocking(GPS_UART, mt_set_speed, strlen(MT_SET_SPEED));
    uart_tx_wait_blocking(GPS_UART);
    while(uart_is_readable(GPS_UART))
        uart_getc(GPS_UART);
}

void GPS::configure_ubx(){
    uart_write_blocking(GPS_UART, (uint8_t *) UBX_SET_SPEED, strlen(UBX_SET_SPEED));
    uart_tx_wait_blocking(GPS_UART);
    while(uart_is_readable(GPS_UART))
        uart_getc(GPS_UART);

}

void GPS::end()
{
#ifdef FAMILY_ESP32
    detachInterrupt(PIN_PPS);
#endif
#ifdef FAMILY_RP2040
    //irq_remove_handler(PIN_PPS, _pps_isr);
    //gpio_set_irq_enabled(PIN_PPS, GPIO_IRQ_EDGE_RISE, false);
    //irq_set_enabled(IO_IRQ_BANK0, false);
#endif
    _pps = nullptr;
}

char* GPS::time_to_str(const struct tm *t){
    static char result[26];

    sprintf(result, "%d/%.2d/%.2d %.2d:%.2d:%.2d.%d",
        1900 + t->tm_year,
        t->tm_mon,
        t->tm_mday, 
        t->tm_hour,
        t->tm_min, 
        t->tm_sec,
        (time_us_64()-_pps_timestamp_us)
    );

    return result;

}

bool GPS::getTime(struct timeval* tv){
    uint64_t cur_micros  = time_us_64();

    if(!_valid)
        return false;

    tv->tv_sec  = mktime(&_nmea_timestamp);
    tv->tv_usec = (uint32_t)(cur_micros - _pps_timestamp_us);
    
    //If the pps timestamp is newer than the nmea timestamp the pps pulse has so we are 1 second later 
    
    if(_pps_timestamp_us > _nmea_timestamp_us){
        //moved to pps isr
        //tv->tv_sec += 1;
    }else{
        uint64_t us_since_nmea_lock = cur_micros-_nmea_timestamp_us;
        uint64_t seconds_since_nmea_lock = us_since_nmea_lock / US_PER_SEC;

        uint64_t us_since_pps_lock = cur_micros-_pps_timestamp_us;
        uint64_t seconds_since_pps_lock = us_since_pps_lock / US_PER_SEC;
        uint64_t us_remainder_since_pps_lock = us_since_pps_lock - (seconds_since_pps_lock*US_PER_SEC);
    
    
        tv->tv_sec += seconds_since_nmea_lock;
        // and since the second happens precisely every second, the latest remainder should be the most accurate info we have?
        tv->tv_usec = us_remainder_since_pps_lock;
    }

    //printf("getTime DBG: %lu %lu %lu %lu\n", tv->tv_sec, tv->tv_usec);
    // if micros_delta is at or bigger than one second then use the max just under 1 second.
    // TODO: does this make sense?
    if (tv->tv_usec >= US_PER_SEC || tv->tv_usec < 0){
        printf("[WARNING] GPS::getTime() tv_usec too large/small?\n");
        tv->tv_usec = 999999;
    }

    return true;
}

double GPS::getDispersion()
{
    return us2s(MAX(MICROS_PER_SEC-_max_micros, MICROS_PER_SEC-_min_micros));
}

void GPS::process()
{
    uint64_t process_time = time_us_64();

    if (_valid && process_time-_pps_timestamp_us > (PPS_VALID_TIME_MS*US_PER_MS)){
        //invalidate("PPS timeout!");
    }

    if (_valid && process_time-_nmea_timestamp_us > (NMEA_VALID_TIME_MS*US_PER_MS)){
        //invalidate("NMEA timeout!");
    }

    if (_reason[0] != '\0')
    {
        printf("[WARNING] REASON: %s\n", _reason);
        _reason[0] = '\0';
    }


    while (uart_is_readable(_uart))
    {
    	 _buf[_buf_idx] = uart_getc(_uart);
        if( _buf_idx > 1 && _buf[_buf_idx] == (char) '\n'){
            break;
        }
        _buf_idx++;
    }
    
    if(_buf_idx > 1 && _buf[_buf_idx] == (char) '\n'){
        //We have a full NMEA sentence to parse.

        switch (minmea_sentence_id(_buf, false)) {
            case MINMEA_SENTENCE_RMC: {
                struct minmea_sentence_rmc frame;
                if (minmea_parse_rmc(&frame, _buf)) {
                    if(frame.valid){
                        minmea_getdatetime(&_nmea_timestamp, &frame.date, &frame.time);
                        _nmea_timestamp_us = time_us_64();

                        _valid = true;

                        printf("VALID | %s | PPS (%" PRIu64 " uS), PPStoNMEA (%" PRIu64 " uS)\n", time_to_str(&_nmea_timestamp), (_pps_timestamp_us-_pps_timestamp_us_prev), (_nmea_timestamp_us-_pps_timestamp_us));
                    }
                }
            } break;

            case MINMEA_SENTENCE_GGA: {
                struct minmea_sentence_gga frame;
                if (minmea_parse_gga(&frame, _buf)) {
                    //printf("$GGA: fix quality: %d\n", frame.fix_quality);
                }
            } break;

            case MINMEA_SENTENCE_GSV: {
                struct minmea_sentence_gsv frame;
                if (minmea_parse_gsv(&frame, _buf)) {
                    //TODO: store satellite data somewhere so it can be plotted and we can confirm we are using enough
                }
            } break;
        }

#ifdef NMEA_DEBUG
        printf("NMEA_DEBUG: %s", _buf);
#endif
        //TODO: store last x epochs of NMEA strings?
        memset(&_buf, 0x0, sizeof(_buf));
        _buf_idx = 0;
    }
}

// Mark as not valid
void __time_critical_func(GPS::invalidate)(const char* fmt, ...)
{
    // only update the reason if there is not one already
    if (_reason[0] == '\0')
    {
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(_reason, REASON_SIZE-1, fmt, ap);
        _reason[REASON_SIZE-1] = '\0';
        va_end(ap);
    }
    _valid       = false;
    _last_micros = 0;
}

// Interrupt handler for a PPS (Pulse Per Second) signal from GPS module.
void __time_critical_func(GPS::pps)(){
    uint64_t _ts_us = time_us_64();
    uint32_t cur_micros = time_us_32();

    _pps_timestamp_us_prev = _pps_timestamp_us;
    _pps_timestamp_us = _ts_us;
    uint64_t us_elapsed = _ts_us-_pps_timestamp_us_prev;

    _nmea_timestamp.tm_sec += 1;

    if(us_elapsed > PPS_VALID_TIME_MS*US_PER_MS)
        return;    

    if (_min_micros == 0 || us_elapsed < _min_micros)
        _min_micros = us_elapsed;

    if (us_elapsed > _max_micros)
        _max_micros = us_elapsed;

}

