#!/bin/bash

release="../platformio/.pio/build/release"

./esptool.exe \
  --chip esp32 --port "COM6" --baud 512200 --before default_reset --after hard_reset  \
  write_flash  \
    -z --flash_mode dio --flash_freq 40m --flash_size 4MB \
    0x1000 ${release}/bootloader.bin \
    0x8000 ${release}/partitions.bin \
    0x10000 ${release}/firmware.bin

