/*
 * NTP.h
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
 *  Created on: Feb 27, 2018
 *      Author: chris.l
 */

#ifndef NTP_H_
#define NTP_H_
#include "net.h"
#include "gps.h"

typedef struct ntp_time
{
    uint32_t seconds;
    uint32_t fraction;
} NTPTime;

class NTP
{
public:
    NTP(GPS& gps);
    virtual ~NTP();

    void     begin();

    uint32_t getReqCount() { return _req_count; }
    uint32_t getRspCount() { return _rsp_count; }


    GPS&     _gps;
    udp_pcb* _udp;
    uint32_t _req_count;
    uint32_t _rsp_count;
    uint8_t  _precision;

    void getNTPTime(NTPTime *time);
    int8_t computePrecision();
};

void ntp_udp_recv_cb(void* arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);

#endif /* NTP_H_ */
