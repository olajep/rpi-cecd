#RPI-CECD


This is a temporary hack to make HDMI-CEC work with OpenElec on Raspberry Pi

##Building:
* Get https://github.com/raspberrypi/firmware
* Build OpenElec to get the toolchain ( I don't know if you can download it from somewhere )
* Adjust cross_compile accordingly
* source ./cross_compile
* make

##Make it work on OpenElec:
* scp rpi-cecd root@'your-openelec-ip':~
* ssh root@'your-openelec-ip'
* ./rpi-cecd &
* You have to this on every boot, again this is just a quick hack.
