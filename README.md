#RPI-CECD

This is a quick hack to make HDMI-CEC work with OpenElec/Raspbmc on Raspberry Pi.
The goal is to make your TV remote work with XBMC

##LG Menu Buttons Workaround
For LG models lacking the back/menu buttons use the following key combos:
* Back = Stop + Select(OK)
* Menu = Stop + Pause

##Building:
* Build OpenElec to get the toolchain and sysroot
* Point $SYSROOT to the target's filesystem root
```
export SYSROOT=/path/to/OpenELEC.tv/build.OpenELEC-RPi.arm-devel/toolchain/armv6zk-openelec-linux-gnueabi/sysroot
```
* Point $CXX to the cross compiler
```
export CXX=/path/to/OpenELEC.tv/build.OpenELEC-RPi.arm-devel/toolchain/bin/armv6zk-openelec-linux-gnueabi-g++
```
* make
```
make
```

##How to make it work on OpenElec:
__NOTE:__ rpi-cecd is included in the OpenElec RPi nightly builds as of ~2012-06-08

##Raspbmc
It seems that the binary also work on raspbmc, altough I have not tested it.
Get the lastest from:
https://github.com/olajep/rpi-cecd/raw/binary/rpi-cecd


