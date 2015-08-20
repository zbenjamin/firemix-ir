# firemix-ir
pull lepton ir data and push int into memory for firemix



setup on beaglebone black:
   dtc -O dtb -o BB-SPI0-01-00A0.dtbo -b 0 -@ BB-SPI0-01-00A0.dts
   cp BB-SPI0-01-00A0.dtbo /lib/firmware/
   echo BB-SPI0-01 > /sys/devices/bone_capemgr.*/slots
   for some reason the device shows up as /dev/spidev1.0 instead of the expected spidev0.1
   add the following to uEnv.txt from the device on your laptop:
   optargs=quiet drm.debug=7 capemgr.enable_partno=BB-SPI0-01

route add default gw 192.168.7.1 usb0
