

#include "lwipopts.h"
#include "main.h"

#include "bsp/board.h"
#include "tusb.h"


//#include "gps.h"
//#include "ntp.h"

#include "common.h"



//GPS gps;
//NTP ntp(gps);


int main(void){
  // initialize TinyUSB
  board_init();

  // init device stack on configured roothub port
  tud_init(BOARD_TUD_RHPORT);

  //Configure GPS UART
  //gpio_set_function(PIN_GPS_TX, GPIO_FUNC_UART);
  //gpio_set_function(PIN_GPS_RX, GPIO_FUNC_UART);
  //uart_init(GPS_UART, GPS_UART_BAUD);

  // initialize lwip
  init_lwip();
  while (!netif_is_up(&netif_data));

  // Start DHCP client
  dhcp_start(&netif_data);

  // Start HTTP Server
  httpd_init();

  // Start NTP Server
  //ntp.begin();
  
  while (1)
  {
    tud_task();
    service_traffic();
    //gps.process();
  }

  return 0;
}

