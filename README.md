#RPI-CECD

This project ~~is~~ was a quick hack to make HDMI-CEC work with OpenElec/Raspbmc/XBMC on Raspberry Pi.

##Project status as of 2012-08-11
libcec (http://libcec.pulse-eight.com/) as of 1.8.0 have added support for the RPi.
This, in combination with OpenElec replacing rpi-cecd with libcec on the RPi platform, makes this project deprecated.

I might fix issues with the current code, but I won't merge new features.

It was a fun toy project while it lasted, but good riddance.

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
~~__NOTE:__ rpi-cecd is included in the OpenElec RPi nightly builds as of ~2012-06-08~~

##Raspbmc
It seems that the binary also work on raspbmc, although I have not tested it.
Get the latest from:
https://github.com/olajep/rpi-cecd/raw/binary/rpi-cecd


