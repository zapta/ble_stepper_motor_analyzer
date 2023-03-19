# Represents a fetched coils currents histogram data.

from __future__ import annotations
import logging
from typing import List
from .probe_info import ProbeInfo

logger = logging.getLogger(__name__)


class CurrentHistogram:

    def __init__(self, bucket_width: int, buckets: List[float]):
        self.__bucket_width: int = bucket_width
        self.__buckets: List[float] = buckets

    @classmethod
    def decode(cls, data: bytearray, probe_info: ProbeInfo,
               steps_per_unit: float) -> (CurrentHistogram | None):
        format = data[0]
        if format != 0x10:
            logger.error(f"Unexpected current histogram format {format}.")
            return None

        bucket_count = data[1]

        buckets = []
        for i in range(bucket_count):
            offset = 2 + i * 2
            current_ticks = int.from_bytes(data[offset:offset + 2], byteorder='big', signed=False)
            current_amps = current_ticks / probe_info.current_ticks_per_amp()
            buckets.append(current_amps)

        return CurrentHistogram(probe_info.histogram_bucket_steps_per_sec() / steps_per_unit,
                                buckets)

    def centers(self) -> List[float]:
        w = self.__bucket_width
        v = w / 2
        result = []
        for i in range(len(self.__buckets)):
            result.append(v)
            v += w
        return result

    def heights(self) -> List[float]:
        return self.__buckets

    def bucket_width(self) -> int:
        return self.__bucket_width
