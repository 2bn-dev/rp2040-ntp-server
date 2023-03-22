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

/*
this appears as either a RNDIS or CDC-ECM USB virtual network adapter; the OS picks its preference

RNDIS should be valid on Linux and Windows hosts, and CDC-ECM should be valid on Linux and macOS hosts

The MCU appears to the host as IP address 192.168.7.1, and provides a DHCP server, DNS server, and web server.
*/
/*
Some smartphones *may* work with this implementation as well, but likely have limited (broken) drivers,
and likely their manufacturer has not tested such functionality.  Some code workarounds could be tried:

The smartphone may only have an ECM driver, but refuse to automatically pick ECM (unlike the OSes above);
try modifying ./examples/devices/net_lwip_webserver/usb_descriptors.c so that CONFIG_ID_ECM is default.

The smartphone may be artificially picky about which Ethernet MAC address to recognize; if this happens, 
try changing the first byte of tud_network_mac_address[] below from 0x02 to 0x00 (clearing bit 1).
*/

#include "net.h"
#include "netif/etharp.h"
#include "pico/unique_id.h"

// lwip context
struct netif netif_data;

// shared between tud_network_recv_cb() and service_traffic()
struct pbuf *received_frame = NULL;

// this is used by this code, ./class/net/net_driver.c, and usb_descriptors.c
// ideally speaking, this should be generated from the hardware's unique ID (if available)
// it is suggested that the first byte is 0x02 to indicate a link-local address 
uint8_t tud_network_mac_address[6] = {0x00,0x00,0x00,0x00,0x00,0x00};

// network parameters of this MCU 
// --  should be overridden by DHCP  -- 
//ip4_addr_t ipaddr  = INIT_IP4(192, 168, 7, 1);
//ip4_addr_t netmask = INIT_IP4(255, 255, 255, 0);
//ip4_addr_t gateway = INIT_IP4(0, 0, 0, 0);



err_t linkoutput_fn(struct netif *netif, struct pbuf *p)
{
  (void)netif;

  for (;;)
  {
    /* if TinyUSB isn't ready, we must signal back to lwip that there is nothing we can do */
    if (!tud_ready())
      return ERR_USE;

    /* if the network driver can accept another packet, we make it happen */
    if (tud_network_can_xmit(p->tot_len))
    {
      tud_network_xmit(p, 0 /* unused for this example */);
      return ERR_OK;
    }

    /* transfer execution to TinyUSB in the hopes that it will finish transmitting the prior packet */
    tud_task();
  }
}

err_t ip4_output_fn(struct netif *netif, struct pbuf *p, const ip4_addr_t *addr)
{
  //printf("ip4_output\n");
  return etharp_output(netif, p, addr);
}

#if LWIP_IPV6
err_t ip6_output_fn(struct netif *netif, struct pbuf *p, const ip6_addr_t *addr)
{
  return ethip6_output(netif, p, addr);
}
#endif

err_t netif_init_cb(struct netif *netif)
{
  LWIP_ASSERT("netif != NULL", (netif != NULL));
  netif->mtu = CFG_TUD_NET_MTU;
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
  netif->state = NULL;
  netif->name[0] = 'E';
  netif->name[1] = 'X';
  netif->linkoutput = linkoutput_fn;
  netif->output = ip4_output_fn;
#if LWIP_IPV6
  netif->output_ip6 = ip6_output_fn;
#endif
  printf("netif_init_cb()\n");
  return ERR_OK;
}

void init_lwip(void)
{
  struct netif *netif = &netif_data;

  //lwip_init();

  // the lwip virtual MAC address must be different from the host's; to ensure this, we toggle the LSbita
  pico_unique_board_id_t flash_id;
  pico_get_unique_board_id(&flash_id);
  memcpy(tud_network_mac_address, &flash_id, sizeof(tud_network_mac_address)); //flash ID is 64 bits/8 bytes, MAC is 48 bits / 6 bytes
  tud_network_mac_address[0] |= 0x40; //set the second bit in the mac to 0x1 make this a "locally administered" address

  netif->hwaddr_len = sizeof(tud_network_mac_address);
  memcpy(netif->hwaddr, tud_network_mac_address, sizeof(tud_network_mac_address));
  netif->hwaddr[5] ^= 0x01;
  
  printf("init_lwip()\n"); 
  netif = netif_add_noaddr(netif, NULL, netif_init_cb, netif_input);
#if LWIP_IPV6
  netif_create_ip6_linklocal_address(netif, 1);
#endif
  netif_set_default(netif);
}

bool tud_network_recv_cb(const uint8_t *src, uint16_t size)
{
  // this shouldn't happen, but if we get another packet before 
  //  parsing the previous, we must signal our inability to accept it 
  if (received_frame) return false;

  //printf("tud_network_recv_cb()");
  if (size)
  {
    struct pbuf *p = pbuf_alloc(PBUF_RAW, size, PBUF_POOL);

    if (p)
    {
      // pbuf_alloc() has already initialized struct; all we need to do is copy the data
      memcpy(p->payload, src, size);

      // store away the pointer for service_traffic() to later handle 
      received_frame = p;
    }
  }

  return true;
}

uint16_t tud_network_xmit_cb(uint8_t *dst, void *ref, uint16_t arg)
{
  struct pbuf *p = (struct pbuf *)ref;
  printf("tud_network_xmit_cb(%d) \n", p->tot_len);
  (void)arg; //unused

  return pbuf_copy_partial(p, dst, p->tot_len, 0);
}

void service_traffic(void)
{
  // handle any packet received by tud_network_recv_cb()
  if (received_frame)
  {
    //printf("received_frame\n");
    //netif_data.input(received_frame, &netif_data);
    ethernet_input(received_frame, &netif_data);

    pbuf_free(received_frame);
    received_frame = NULL;
    tud_network_recv_renew();
  }

  sys_check_timeouts();
}

void tud_network_init_cb(void)
{
  //printf("tud_network_init_cb()\n");
  // if the network is re-initializing and we have a leftover packet, we must do a cleanup 
  if (received_frame)
  {
    pbuf_free(received_frame);
    received_frame = NULL;
  }
}
