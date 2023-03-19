cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)
project(rp2040-ntp-server)

pico_sdk_init()

add_executable(rp2040-ntp-server
    
    ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/usb_descriptors.c
    ${CMAKE_CURRENT_LIST_DIR}/src/net.cpp
    #${CMAKE_CURRENT_LIST_DIR}/src/ntp.cpp
    #${CMAKE_CURRENT_LIST_DIR}/src/gps.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/arch/sys_arch.c

)

target_include_directories(rp2040-ntp-server PUBLIC 
    ${CMAKE_CURRENT_LIST_DIR}/src/
    ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/include/
    ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/include/ipv4/
    ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/include/lwip/
    ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/include/lwip/apps/
    ${CMAKE_CURRENT_LIST_DIR}/lib/tinyusb/src/
    ${CMAKE_CURRENT_LIST_DIR}/lib/tinyusb/lib/networking/
    ${CMAKE_CURRENT_LIST_DIR}/lib/minmea/
)

include(${CMAKE_CURRENT_LIST_DIR}/lib/es100/CMakeLists.txt)
#include(${CMAKE_CURRENT_LIST_DIR}/lib/libnmea/CMakeLists.txt)


target_sources(rp2040-ntp-server PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/core/altcp.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/core/altcp_alloc.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/core/altcp_tcp.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/core/def.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/core/dns.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/core/inet_chksum.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/core/init.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/core/ip.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/core/mem.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/core/memp.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/core/netif.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/core/pbuf.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/core/raw.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/core/stats.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/core/sys.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/core/tcp.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/core/tcp_in.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/core/tcp_out.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/core/timeouts.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/core/udp.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/core/ipv4/acd.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/core/ipv4/autoip.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/core/ipv4/dhcp.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/core/ipv4/etharp.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/core/ipv4/icmp.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/core/ipv4/igmp.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/core/ipv4/ip4.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/core/ipv4/ip4_addr.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/core/ipv4/ip4_frag.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/netif/ethernet.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/netif/slipif.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/apps/http/httpd.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/lwip/src/apps/http/fs.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/tinyusb/lib/networking/rndis_reports.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/minmea/minmea.c
    )




target_link_libraries(
	rp2040-ntp-server
    tinyusb_device
    tinyusb_board
    #lwip
	es100
	pico_stdlib
	pico_time
    pico_multicore
	hardware_i2c
	)

pico_enable_stdio_usb(rp2040-ntp-server 0)
pico_enable_stdio_uart(rp2040-ntp-server 1)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(rp2040-ntp-server)



