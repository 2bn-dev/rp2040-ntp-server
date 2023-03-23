/*
 * GPS.h
 *
 * Copyright 2017 Christopher B. Liebman
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

#ifndef GPS_H_
#define GPS_H_

#include <pico/stdlib.h>
#include <pico/stdio.h>
#include <pico/time.h>
#include <string.h>
#include <ctime>

#include "minmea.h"

#include "common.h"

#define REASON_SIZE       128
#define NMEA_BUFFER_SIZE  128

#define PPS_VALID_TIME_MS       1001 // period from previous PPS pulse, if exceeded, timestamp no longer considered valid
#define NMEA_VALID_TIME_MS      1100 // period from previous NMEA RMC, if exceeded, timestamp no longer considered valid

#define MICROS_PER_SEC         1000000

#define us2s(x) (((double)x)/(double)MICROS_PER_SEC) // microseconds to seconds


//GPS NMEA configuration statements

#define MT_SET_SPEED  "$PMTK251,115200*1F\r\n"
#define MT_SET_SPEED_ALT "$PGCMD,232,6*58\r\n" // Requires cold restart :|
#define MT_FULL_COLD_START "$PMTK104*37\r\n"  
#define MT_SET_TIMING_PRODUCT "$PMTK256,1*2E\r\n" //Configure GNSS Timing product to enhance 1PPS output timing accuracy.
#define MT_SET_PPS_NMEA "$PMTK255,1*2D\r\n" //Enable "fixed" PPS -> NMEA output period

#define UBX_SET_SPEED "$PUBX,41,1,0023,0003,115200,0*1E\r\n"



class GPS
{
public:
    GPS();
    virtual ~GPS();

    void     begin();
    void     process();
    void     end();

    bool     isValid()       { return _valid; }
    uint32_t getJitter()     { return _max_micros - _min_micros; }
    uint32_t getValidCount() { return _valid_count; }
    time_t   getValidSince() { return _valid_since; }
    //uint8_t  getSatelliteCount() { return _nmea.getNumSatellites(); }
    bool     getTime(struct timeval* tv);
    double   getDispersion();

private:
    uart_inst_t*      _uart;
    char              _buf[NMEA_BUFFER_SIZE];
    volatile uint32_t _valid_count;  // number of times we have gone valid
    volatile time_t   _valid_since;
    volatile uint32_t _min_micros;
    volatile uint32_t _max_micros;
    volatile uint32_t _last_micros;
    volatile uint32_t _timeouts;

    volatile uint64_t _last_pps_us;

    volatile bool     _valid;
    char              _reason[REASON_SIZE];


    uint8_t           _buf_idx = 0;
    struct tm         _nmea_timestamp;
    uint64_t          _nmea_timestamp_us;
    uint64_t          _pps_timestamp_us;


    void pps();        // interrupt handler
    void invalidate(const char* fmt, ...);
    void configure_mtk();
    void configure_ubx();
};

#endif /* GPS_H_ */
