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
#include <stdio.h>
#include <stdarg.h>
#include "gps.h"



static const char* TAG = "gps";

static std::function<void()> _pps;
static std::function<void()> _invalidate;
static std::function<void()> _nmea_timeout;

static __isr void _pps_isr()
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
    _nmea_late(true)
{
    _reason[0] = '\0';
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
    uart_init(GPS_UART, GPS_UART_BAUD);

    add_repeating_timer_ms(VALID_TIMER_MS, &invalidate_callback, NULL, &_pps_timer);
    add_repeating_timer_ms(NMEA_TIMER_MS, &nmea_timeout_callback, NULL, &_nmea_timer);
#ifdef FAMILY_ESP32
    
    pinMode(_pps_pin, INPUT);
    attachInterrupt(_pps_pin, _pps_isr, RISING);
#endif
#ifdef FAMILY_RP2040
    gpio_set_dir(_pps_pin, false);
    irq_set_exclusive_handler(IO_IRQ_BANK0, _pps_isr);
    gpio_set_irq_enabled(_pps_pin, GPIO_IRQ_EDGE_RISE, true);
    irq_set_enabled(IO_IRQ_BANK0, true); 
#endif
}

void GPS::end()
{
    cancel_repeating_timer(&_pps_timer);
#ifdef FAMILY_ESP32
    detachInterrupt(_pps_pin);
#endif
#ifdef FAMILY_RP2040
    irq_remove_handler(_pps_pin, _pps_isr);
    gpio_set_irq_enabled(_pps_pin, GPIO_IRQ_EDGE_RISE, false);
    irq_set_enabled(IO_IRQ_BANK0, false);
#endif
    _pps = nullptr;
}

void GPS::getTime(struct timeval* tv)
{
    uint32_t cur_micros  = time_us_32();
    tv->tv_sec  = _seconds;
    tv->tv_usec = (uint32_t)(cur_micros - _last_micros);

    //
    // if micros_delta is at or bigger than one second then
    // use the max just under 1 second.
    //
    if (tv->tv_usec >= 1000000 || tv->tv_usec < 0)
    {
        tv->tv_usec = 999999;
    }
}

double GPS::getDispersion()
{
    return us2s(MAX(MICROS_PER_SEC-_max_micros, MICROS_PER_SEC-_min_micros));
}
/*
void GPS::process()
{
    if (_reason[0] != '\0')
    {
        //dlog.warning(TAG, F("REASON: %s"), _reason);
        _reason[0] = '\0';
    }

    while (uart_is_readable(_uart))
    {
    	char c = uart_getc(_uart);
    	//dlog.trace(TAG, F("c: %c"), c);
        if (0)//_nmea.process(c))
        {
            struct timeval tv;
            getTime(&tv);
            //dlog.debug(TAG, F("'%s'"), _nmea.getSentence());

            //const char * id = _nmea.getMessageID();

            //
            // if it was a RMC and its valid then check and maybe update the time
            //
            if (_nmea.getYear() > 2017 && strcmp("RMC", id) == 0)
            {
                struct tm tm;
                tm.tm_year  = _nmea.getYear() - 1900;
                tm.tm_mon   = _nmea.getMonth() - 1;
                tm.tm_mday  = _nmea.getDay();
                tm.tm_hour  = _nmea.getHour();
                tm.tm_min   = _nmea.getMinute();
                tm.tm_sec   = _nmea.getSecond();
                time_t new_seconds = mktime(&tm);

                //
                // we only update seconds if the message arrived in the last half of a second,
                // if its in the first half then its most likely delayed from the previous second.
                time_t old_seconds = _seconds;
                if (old_seconds != new_seconds)
                {
                    if (!_nmea_late)
                    {
                        _seconds = new_seconds;
                        invalidate("seconds adjusted!");
                        //dlog.info(TAG, F("adjusting seconds from %lu to %lu from:'%s'"), old_seconds, new_seconds, _nmea.getSentence());
                    }
                    else
                    {
                        //dlog.debug(TAG, F("ignoring late NMEA time: '%s'"),_nmea.getSentence());
                    }
                }

                _nmea_late = false;
                
            }

            if (_nmea.isValid() && _nmea.getNumSatellites() >= 4)
            {
                //
                // if gps was not valid, it is now
                //
                if (!_gps_valid)
                {
                    _valid_delay = VALID_DELAY;
                    _gps_valid       = true;
                    //dlog.info(TAG, F("GPS valid!"));
                }
            }
            else // nmea not valid or sat count < 4
            {
                if (_gps_valid || _valid_delay)
                {
                    invalidate("NMEA:%s SATS:%d from: '%s'",
                            _nmea.isValid() ? "valid" : "invalid",
                            _nmea.getNumSatellites(), _nmea.getSentence());
                }
            }
        }
    }
}
*/

void GPS::process(){
}

void GPS::nmeaTimeout()
{
    _nmea_late = true;
}


/*
 * Mark as not valid
 */
void /*__time_critical_func*/ GPS::timeout()
{
    if (_valid && time_us_64()-_last_pps_us < (VALID_TIMER_MS*1000))
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

