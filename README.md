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

##How to make it work on OpenElec:
__NOTE:__ rpi-cecd is included in the OpenElec RPi nightly builds as of ~2012-06-08


