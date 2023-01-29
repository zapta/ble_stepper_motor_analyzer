:: A batch file to flash an esp32 analyzer device.

esptool.exe ^
  --chip esp32 ^
  --baud 512200 ^
  write_flash ^
    0x1000  ..\firmware\bootloader.bin ^
    0x8000  ..\firmware\partitions.bin ^
    0x10000 ..\firmware\firmware.bin

