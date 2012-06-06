#RPI-CECD

This is a temporary hack to make HDMI-CEC work with OpenElec on Raspberry Pi

##Building:
* Get https://github.com/raspberrypi/firmware
* Build OpenElec to get the toolchain
* ... or download the official SDK from:
  https://github.com/raspberrypi/tools/tree/master/arm-bcm2708
* Point $SDKSTAGE to the target filesystem root
* make

##Make it work on OpenElec:
* Enable the HTTP API in XBMC's settings
* scp rpi-cecd root@'your-openelec-ip':~
* ssh root@'your-openelec-ip'
* ./rpi-cecd &
* You have to do this on every boot


