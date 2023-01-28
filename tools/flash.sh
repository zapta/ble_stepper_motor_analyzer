#!/bin/bash

release="../platformio/.pio/build/release"

./esptool.exe --chip esp32 write_flash  \
    0x1000 ${release}/bootloader.bin \
    0x8000 ${release}/partitions.bin \
    0x10000 ${release}/firmware.bin

