# RP2040 NTP Server
------------------

**This is in-development code, not a finished tool**


An NTP Server based on code from https://github.com/liebman/ESPNTPServer designed to work with an rp2040, via TinyUSB USB network device.

The rp2040 presents a USB NIC to the host PC, then the host PC bridges that nic with the LAN network. I may also add wifi at some point, but I don't have any pico w's at the moment.

Intended to be (potentially) usable with:

 * a GPS with 1PPS output
 * Everset ES100 WWVB BPSK receiver
 * CMAX Timing CME6005 modules (cheap WWVB AM receivers)
 * LTE/5g? provided clock
 * ATSC? provided clock (don't know if this is accurate enough or feasibly captured)
 * P25 provided clock (same as above)
 * WWV by phone service?


## Building:
```
mkdir build
cd build
cmake ..
make -j 8
```
Then copy the generated uf2 file to the pico.

