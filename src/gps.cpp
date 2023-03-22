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
static std::function<void()> _invalidate;
static std::function<void()> _nmea_timeout;

const uint8_t mt_set_speed[] = MT_SET_SPEED;
const uint8_t mt_set_timing_product[] = MT_SET_TIMING_PRODUCT;
const uint8_t mt_set_pps_nmea[] = MT_SET_PPS_NMEA;

static __isr void _pps_isr(unsigned int gpio, long unsigned int mask)
{
    if (_pps){
        _pps();
    }
}

static bool invalidate_callback(struct repeating_timer *t){
    if (_invalidate){
        _invalidate();
    }    

    return true;
}

static bool nmea_timeout_callback(struct repeating_timer *t){
    if (_nmea_timeout){
        _nmea_timeout();
    }

    return true;
}


GPS::GPS() :
    _uart(GPS_UART),
    _pps_timer(),
    _seconds(0),
    _valid_delay(0),
    _valid_count(0),
    _min_micros(0),
    _max_micros(0),
    _last_micros(0),
    _timeouts(0),
    _pps_pin(PIN_PPS),
    _gps_valid(false),
    _valid(false),
    _nmea_late(false),
    _nmea_timestamp_us(time_us_64()),
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
    PPS_TIMIMG_PIN_INIT();
    _pps = std::bind( &GPS::pps, this);
    _invalidate = std::bind( &GPS::timeout, this);
    _nmea_timeout = std::bind( &GPS::nmeaTimeout, this);

    //Configure GPS UART
    gpio_set_function(PIN_GPS_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_GPS_RX, GPIO_FUNC_UART);

    uart_init(GPS_UART, 9600);
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

    
    add_repeating_timer_ms(VALID_TIMER_MS, &invalidate_callback, NULL, &_pps_timer);
    add_repeating_timer_ms(NMEA_TIMER_MS, &nmea_timeout_callback, NULL, &_nmea_timer);
#ifdef FAMILY_ESP32
    
    pinMode(_pps_pin, INPUT);
    attachInterrupt(_pps_pin, _pps_isr, RISING);
#endif
#ifdef FAMILY_RP2040
    gpio_set_dir(_pps_pin, false);
    gpio_set_irq_enabled_with_callback(_pps_pin, GPIO_IRQ_EDGE_RISE, true, _pps_isr);
    //irq_set_exclusive_handler(IO_IRQ_BANK0, _pps_isr);
    //gpio_set_irq_enabled(_pps_pin, GPIO_IRQ_EDGE_RISE, true);
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
    cancel_repeating_timer(&_pps_timer);
#ifdef FAMILY_ESP32
    detachInterrupt(_pps_pin);
#endif
#ifdef FAMILY_RP2040
    //irq_remove_handler(_pps_pin, _pps_isr);
    //gpio_set_irq_enabled(_pps_pin, GPIO_IRQ_EDGE_RISE, false);
    //irq_set_enabled(IO_IRQ_BANK0, false);
#endif
    _pps = nullptr;
}

void GPS::getTime(struct timeval* tv)
{
    uint64_t cur_micros  = time_us_64();
    tv->tv_sec  = mktime(&_nmea_timestamp);
    tv->tv_usec = (uint32_t)(cur_micros - _pps_timestamp_us);

    //
    // if micros_delta is at or bigger than one second then
    // use the max just under 1 second.
    //
    if (tv->tv_usec >= 1000000 || tv->tv_usec < 0)
    {
        printf("[WARNING] GPS::getTime() tv_usec too large/small?\n");
        tv->tv_usec = 999999;
    }
}

double GPS::getDispersion()
{
    return us2s(MAX(MICROS_PER_SEC-_max_micros, MICROS_PER_SEC-_min_micros));
}

void GPS::process()
{
    if (_reason[0] != '\0')
    {
        printf("[WARNING] REASON: %s\n", _reason);
        _reason[0] = '\0';
    }

    //memset(&_buf, 0x0, sizeof(_buf));

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
                        _gps_valid = true;
                        _valid = true;
                        _valid_delay = VALID_DELAY; //TODO: wat is this?
                        printf("Valid RMC | PPS -> NMEA delay: %" PRIu64 "uS\n", (_nmea_timestamp_us-_pps_timestamp_us));
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
        printf("NMEA: %s", _buf);
#endif
        //TODO: store last x epochs of NMEA strings?
        memset(&_buf, 0x0, sizeof(_buf));
        _buf_idx = 0;
    }
}

void GPS::nmeaTimeout()
{
    //TODO: something
    //_nmea_late = true;
}


void /*__time_critical_func*/ GPS::timeout()
{
    if (_valid && time_us_64()-_pps_timestamp_us > (VALID_TIMER_MS*1000))
    {
        invalidate("timeout!");
    }
}

/*
 * Mark as not valid
 */
void /*__time_critical_func*/ GPS::invalidate(const char* fmt, ...)
{
    //
    // only update the reason if there is not one already
    //
    if (_reason[0] == '\0')
    {
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(_reason, REASON_SIZE-1, fmt, ap);
        _reason[REASON_SIZE-1] = '\0';
        va_end(ap);
    }
    _valid       = false;
    _gps_valid   = false;
    _valid_delay = 0;
    _last_micros = 0;
}

/*
 * Interrupt handler for a PPS (Pulse Per Second) signal from GPS module.
 */
void /*__time_critical_func*/ GPS::pps()
{

    uint32_t cur_micros = time_us_32();
    _pps_timestamp_us = time_us_64();
    
#if 0
    //
    // don't trust PPS if GPS is not valid.
    //
    if (!_gps_valid)
    {
        PPS_TIMING_PIN_OFF();
        return;
    }
#endif

    //
    // increment seconds
    //
    _seconds += 1;


    //
    // if we are still counting down then keep waiting
    //
    if (_valid_delay)
    {
        --_valid_delay;
        if (_valid_delay == 0)
        {
            // clear stats and mark us valid
            _min_micros  = 0;
            _max_micros  = 0;
            _valid       = true;
            _valid_since = _seconds;
            ++_valid_count;
        }
    }

    //
    // the first time around we just initialize the last value
    //
    if (_last_micros == 0)
    {
        _last_micros = cur_micros;
        return;
    }

    uint32_t micros_count = cur_micros - _last_micros;
    _last_micros           = cur_micros;

    if (_min_micros == 0 || micros_count < _min_micros)
    {
        _min_micros = micros_count;
    }

    if (micros_count > _max_micros)
    {
        _max_micros = micros_count;
    }
}

