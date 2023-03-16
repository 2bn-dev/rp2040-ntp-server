cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)
project(usb_ethernet)

pico_sdk_init()

add_executable(usb_ethernet
    main.c
    usb_descriptors.c
    lib/tinyusb/lib/lwip/src/core/ipv4/dhcp.c
)

target_include_directories(usb_ethernet PUBLIC 
    lib/tinyusb/lib/lwip/src/include/
    lib/tinyusb/lib/lwip/src/include/ipv4/
    lib/tinyusb/lib/lwip/src/include/lwip/
    lib/tinyusb/lib/lwip/src/include/lwip/apps/
    lib/tinyusb/src/
    lib/tinyusb/lib/networking/
    ${CMAKE_CURRENT_LIST_DIR}/
)

include(../../CMakeLists.txt)


target_sources(usb_ethernet PUBLIC
            lib/tinyusb/lib/lwip/src/core/altcp.c
            lib/tinyusb/lib/lwip/src/core/altcp_alloc.c
            lib/tinyusb/lib/lwip/src/core/altcp_tcp.c
            lib/tinyusb/lib/lwip/src/core/def.c
            lib/tinyusb/lib/lwip/src/core/dns.c
            lib/tinyusb/lib/lwip/src/core/inet_chksum.c
            lib/tinyusb/lib/lwip/src/core/init.c
            lib/tinyusb/lib/lwip/src/core/ip.c
            lib/tinyusb/lib/lwip/src/core/mem.c
            lib/tinyusb/lib/lwip/src/core/memp.c
            lib/tinyusb/lib/lwip/src/core/netif.c
            lib/tinyusb/lib/lwip/src/core/pbuf.c
            lib/tinyusb/lib/lwip/src/core/raw.c
            lib/tinyusb/lib/lwip/src/core/stats.c
            lib/tinyusb/lib/lwip/src/core/sys.c
            lib/tinyusb/lib/lwip/src/core/tcp.c
            lib/tinyusb/lib/lwip/src/core/tcp_in.c
            lib/tinyusb/lib/lwip/src/core/tcp_out.c
            lib/tinyusb/lib/lwip/src/core/timeouts.c
            lib/tinyusb/lib/lwip/src/core/udp.c
            lib/tinyusb/lib/lwip/src/core/ipv4/autoip.c
            lib/tinyusb/lib/lwip/src/core/ipv4/dhcp.c
            lib/tinyusb/lib/lwip/src/core/ipv4/etharp.c
            lib/tinyusb/lib/lwip/src/core/ipv4/icmp.c
            lib/tinyusb/lib/lwip/src/core/ipv4/igmp.c
            lib/tinyusb/lib/lwip/src/core/ipv4/ip4.c
            lib/tinyusb/lib/lwip/src/core/ipv4/ip4_addr.c
            lib/tinyusb/lib/lwip/src/core/ipv4/ip4_frag.c
            lib/tinyusb/lib/lwip/src/netif/ethernet.c
            lib/tinyusb/lib/lwip/src/netif/slipif.c
            lib/tinyusb/lib/lwip/src/apps/http/httpd.c
            lib/tinyusb/lib/lwip/src/apps/http/fs.c
            lib/tinyusb/lib/networking/rndis_reports.c
            )




target_link_libraries(
	usb_ethernet
    tinyusb_device
    tinyusb_board
    #lwip
	es100
	pico_stdlib
	pico_time
    pico_multicore
	hardware_i2c
	)

pico_enable_stdio_usb(usb_ethernet 0)
pico_enable_stdio_uart(usb_ethernet 1)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(usb_ethernet)



