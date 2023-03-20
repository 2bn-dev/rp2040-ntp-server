

#include "lwipopts.h"

#include "bsp/board.h"
#include "tusb.h"

#include "net.h"
#include "gps.h"
#include "ntp.h"

#include "common.h"

#include "pico/async_context.h"
#include "pico/async_context_poll.h"
#include "pico/lwip_nosys.h"

async_context_poll_t context;
GPS gps;
NTP ntp(gps);

#define LWIP_DEBUG 1

int main(void){
  // initialize TinyUSB
  board_init();

  // init device stack on configured roothub port
  tud_init(BOARD_TUD_RHPORT);

  async_context_poll_init_with_defaults(&context);

  stdio_usb_init();
  tud_task();

  // initialize lwip
  init_lwip(); //inits the interface
  lwip_nosys_init(&context.core);
  
  while (!netif_is_up(&netif_data)){
    tud_task();
    async_context_poll(&context.core);
    sleep_ms(500);
    printf("[WARNING] netif isn't up yet?\n");
  }
  printf("netif_is_up()\n");
  // Start DHCP client
  dhcp_start(&netif_data);

  printf("dhcp_start()\n");
  // Start HTTP Server
  httpd_init();

  // Start NTP Server
  gps.begin();
  ntp.begin();

  printf("IP address: %s\n", ip4addr_ntoa(netif_ip4_addr(&netif_data)));
  printf("main loop start!\n");
  while (1)
  {
    tud_task();
    service_traffic();
    async_context_poll(&context.core);
    gps.process();
  }

  return 0;
}

