# Represents a fetched stepper distance histogram data.

from __future__ import annotations
import logging
from probe_info import ProbeInfo


logger = logging.getLogger(__name__)


class DistanceHistogram:
    def __init__(self, bucket_width: int, buckets: List[float]):
        self.__bucket_width: int = bucket_width
        self.__buckets: List[float] = buckets

    @classmethod
    def decode(cls, data: bytearray, probe_info: ProbeInfo, steps_per_unit: float) -> (DistanceHistogram | None):
        format = data[0]
        if format != 0x30:
            logger.error(f"Unexpected distance histogram format {format}.")
            return None

        bucket_count = data[1]

        # print("\n", flush=True)
        buckets = []
        for i in range(bucket_count):
            offset = 2 + i*2
            distance_mils = int.from_bytes(
                data[offset: offset+2],  byteorder='big', signed=False)
            distance_percents = distance_mils / 10.0
            # print(f"  {i} {distance_mils}, {distance_percents}", flush=True)
            buckets.append(distance_percents)

        # print("\n", flush=True)
        return DistanceHistogram(probe_info.histogram_bucket_steps_per_sec() / steps_per_unit, buckets)

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
