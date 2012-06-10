#RPI-CECD

This is a quick hack to make HDMI-CEC work with OpenElec on Raspberry Pi.

##Building:
* Get https://github.com/raspberrypi/firmware
* Build OpenElec to get the toolchain
* ... or download the official SDK from:
  https://github.com/raspberrypi/tools/tree/master/arm-bcm2708
* Point $SDKSTAGE to the target's filesystem root
* export CC=/path/to/armv6zk-openelec-linux-gnueabi-gcc
* make

##Make it work on OpenElec:
__NOTE:__ rpi-cecd is included in the OpenElec RPi nightly builds as of ~2012-06-08 

* Enable the HTTP API in XBMC's settings
* scp rpi-cecd root@'your-openelec-ip':~
* ssh root@'your-openelec-ip'
* ./rpi-cecd &
* echo '#!/bin/sh' >> /storage/.config/autostart.sh
* echo '/storage/rpi-cecd &' >> /storage/.config/autostart.sh
* chmod +x /storage/.config/autostart.sh
* ...
* Profit


