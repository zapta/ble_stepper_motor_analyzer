#!/bin/bash
#
# Per https://esp32.com/viewtopic.php?t=23384

# To use:
# 1. Build and run the binary with build_type=debug
# 2. Copy/paste the backtrace string to $backtrace below.
# 3. Run this script with no arguments.

backtrace="0x40132471:0x3ffbf770 0x4008332d:0x3ffbf790 0x400842d9:0x3ffbbfc0 0x4008474b:0x3ffbbff0 0x40097722:0x3ffbc020 0x400847b5:0x3ffbc040 0x40084f36:0x3ffbc060 0x40122a23:0x3ffbc0b0 0x40102382:0x3ffbc0d0 0x401326c9:0x3ffbc0f0 0x40102ea9:0x3ffbc120 0x4010339d:0x3ffbc170 0x40101e19:0x3ffbc1d0 0x4010217a:0x3ffbc240 0x40132569:0x3ffbc260 0x401011d7:0x3ffbc290 0x40101200:0x3ffbc2c0 0x400d5a63:0x3ffbc2e0 0x400d5877:0x3ffbc310 0x400d5356:0x3ffbc350 0x400d56ba:0x3ffbc380 0x401331bf:0x3ffbc3a0 0x40094649:0x3ffbc3d0"

decoder="/c/Users/user/.platformio/packages/toolchain-xtensa-esp32/bin/xtensa-esp32-elf-addr2line.exe"
elf_file="../../platformio/.pio/build/esp32dev/firmware.elf"

${decoder} -p -C -e ${elf_file} ${backtrace}

