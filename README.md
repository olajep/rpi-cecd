#RPI-CECD

This is a quick hack to make HDMI-CEC work with OpenElec on Raspberry Pi.

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


