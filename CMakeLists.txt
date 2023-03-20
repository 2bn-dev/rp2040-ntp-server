cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)
project(rp2040-ntp-server)

pico_sdk_init()

add_executable(rp2040-ntp-server
    
    ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/usb_descriptors.c
    ${CMAKE_CURRENT_LIST_DIR}/src/net.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ntp.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/gps.cpp

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
            ${CMAKE_CURRENT_LIST_DIR}/lib/tinyusb/lib/networking/rndis_reports.c
            ${CMAKE_CURRENT_LIST_DIR}/lib/minmea/minmea.c
    )




target_link_libraries(
	rp2040-ntp-server
    tinyusb_device
    tinyusb_board
	es100
	pico_stdlib
	pico_time
    pico_multicore
    pico_lwip
    pico_lwip_nosys
    pico_lwip_core
    pico_lwip_http
    pico_lwip_core4
    pico_lwip_netif
    pico_lwip_arch
    pico_async_context_poll
	hardware_i2c
	)

pico_enable_stdio_usb(rp2040-ntp-server 1)
pico_enable_stdio_uart(rp2040-ntp-server 0)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(rp2040-ntp-server)



