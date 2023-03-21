# Represented BLE device info that was fetched from the device upon connection.

from __future__ import annotations

import logging
import sys

logger = logging.getLogger(__name__)


class ProbeInfo:

    def __init__(self, model: str, manufacturer: str, hardware_config: int,
                 current_ticks_per_amp: int, time_ticks_per_sec: int,
                 histogram_bucket_steps_per_sec: int, app_version_str: str):
        self.__model = model
        self.__manufacturer = manufacturer
        self.__hardware_config = hardware_config
        self.__current_ticks_per_amp = current_ticks_per_amp
        self.__time_ticks_per_sec = time_ticks_per_sec
        self.__histogram_bucket_steps_per_sec = histogram_bucket_steps_per_sec
        self.__app_version_str = app_version_str

    @classmethod
    def decode(cls, data: bytearray, model: str, manufacturer: str) -> (ProbeInfo | None):
        if len(data) < 9:
            logger.error(f"Invalid probe info data length {len(data)}.")
            return None
        # Uint8
        format = data[0]
        if format != 0x01:
            logger.error(f"Unexpected probe info format {format}.")
            return None
        # Uint8
        hardware_config = data[1]
        # Uint16
        current_ticks_per_amp = int.from_bytes(data[2:4], byteorder='big', signed=False)
        # Uint24
        time_ticks_per_sec = int.from_bytes(data[4:7], byteorder='big', signed=False)
        # Uint16
        histogram_bucket_steps_per_sec = int.from_bytes(data[7:9], byteorder='big', signed=False)

        # Added March 2023.
        if len(data) > 9:
            device_version_str_len = data[9]
            device_version_str = data[10:10 + device_version_str_len].decode()
        else:
            device_version_str = "[No device version str]"
        
        return ProbeInfo(model, manufacturer, hardware_config, current_ticks_per_amp,
                         time_ticks_per_sec, histogram_bucket_steps_per_sec, device_version_str)

    def model(self) -> str:
        return self.__model

    def manufacturer(self) -> str:
        return self.__manufacturer

    def hardware_config(self) -> int:
        return self.__hardware_config

    def current_ticks_per_amp(self) -> int:
        return self.__current_ticks_per_amp

    def time_ticks_per_sec(self) -> int:
        return self.__time_ticks_per_sec

    def histogram_bucket_steps_per_sec(self) -> int:
        return self.__histogram_bucket_steps_per_sec

    def device_version_str(self) -> str:
        return self.__device_version_str

    # Dump.
    def dump(self, file=sys.stdout) -> None:
        print(f"Version: [{self.__app_version_str}]", file=file, flush=True)
        print(f"Model: [{self.__model}]", file=file, flush=True)
        print(f"Manufacturer: [{self.__manufacturer}]", file=file, flush=True)
        print(f"Hardware config: [{self.__hardware_config}]", file=file, flush=True)
        print(f"Current ticks per amp: [{self.__current_ticks_per_amp}]", file=file, flush=True)
        print(f"Time ticks per sec: [{self.__time_ticks_per_sec}]", file=file, flush=True)
        print(f"Histogram bucket steps/sec: [{self.__histogram_bucket_steps_per_sec}]",
              file=file,
              flush=True)
