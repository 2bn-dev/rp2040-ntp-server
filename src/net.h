/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Peter Lawrence
 *
 * influenced by lrndis https://github.com/fetisov/lrndis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef SRC_NET_H_
#define SRC_NET_H_

#include "bsp/board.h"
#include "tusb.h"

#include "lwip/init.h"
#include "lwip/timeouts.h"
#include "lwip/ethip6.h"
#include "lwip/dhcp.h"
#include "dhcp.h"
#include "httpd.h"

#include "common.h"

#define INIT_IP4(a,b,c,d) { PP_HTONL(LWIP_MAKEU32(a,b,c,d)) }


// lwip context
extern struct netif netif_data;

// shared between tud_network_recv_cb() and service_traffic()
extern struct pbuf *received_frame;

// this is used by this code, ./class/net/net_driver.c, and usb_descriptors.c
// ideally speaking, this should be generated from the hardware's unique ID (if available)
// it is suggested that the first byte is 0x02 to indicate a link-local address 
extern const uint8_t tud_network_mac_address[6];

// network parameters of this MCU 
// --  should be overridden by DHCP  -- 
extern const ip4_addr_t ipaddr;
extern const ip4_addr_t netmask;
extern const ip4_addr_t gateway;



err_t linkoutput_fn(struct netif *netif, struct pbuf *p);
err_t ip4_output_fn(struct netif *netif, struct pbuf *p, const ip4_addr_t *addr);
#if LWIP_IPV6
err_t ip6_output_fn(struct netif *netif, struct pbuf *p, const ip6_addr_t *addr);
#endif
err_t netif_init_cb(struct netif *netif);
void init_lwip(void);
bool tud_network_recv_cb(const uint8_t *src, uint16_t size);
uint16_t tud_network_xmit_cb(uint8_t *dst, void *ref, uint16_t arg);
void service_traffic(void);
void tud_network_init_cb(void);

#endif
