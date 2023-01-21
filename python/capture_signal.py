# Represents a fetched signal capture buffer.

from __future__ import annotations
import logging
from typing import List
from probe_info import ProbeInfo


logger = logging.getLogger(__name__)


class CaptureSignal:
    def __init__(self, times_sec: List[float], amps_a: List[float], amps_b: List[float]):
        self.__times_sec = times_sec
        self.__amps_a = amps_a
        self.__amps_b = amps_b

    @classmethod
    def decode(cls, packets: List[bytearray], probe_info: ProbeInfo) -> (CaptureSignal | None):
        if len(packets) == 0:
            logger.error(f"No capture signal packets.")
            return None

        divider = int.from_bytes(
            packets[0][4:5],  byteorder='big', signed=False)
        time_step_secs = divider / probe_info.time_ticks_per_sec()

        # Decode data points.
        time_sec_list = []
        amps_a_list = []
        amps_b_list = []
        for packet in packets:
            # NOTE: For now we ignore the packet sequence number and offset field and
            # assume that the packets match.
            n = int.from_bytes(packet[5:7],  byteorder='big', signed=False)
            for i in range(n):
                # 9 is the byte Offset of the a/b pair in the packet.
                base = 9 + (i * 4)
                ticks_a = int.from_bytes(
                    packet[base:base+2],  byteorder='big', signed=True)
                ticks_b = int.from_bytes(
                    packet[base+2:base+4],  byteorder='big', signed=True)
                time_sec = len(amps_a_list) * time_step_secs
                amps_a = ticks_a / probe_info.current_ticks_per_amp()
                amps_b = ticks_b / probe_info.current_ticks_per_amp()
                time_sec_list.append(time_sec)
                amps_a_list.append(amps_a)
                amps_b_list.append(amps_b)
        return CaptureSignal(time_sec_list, amps_a_list, amps_b_list)

    def times_sec(self) -> List[float]:
        return self.__times_sec

    def amps_a(self) -> List[float]:
        return self.__amps_a

    def amps_b(self) -> List[float]:
        return self.__amps_b
