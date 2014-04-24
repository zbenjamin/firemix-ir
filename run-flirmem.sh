#!/bin/sh

echo BB-SPI0-01 | sudo tee /sys/devices/bone_capemgr.*/slots

/root/firemix-ir/flirmem

