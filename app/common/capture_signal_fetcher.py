# A state machine to retrieve the signal capture buffer over
# a few BLE calls.


from __future__ import annotations
import logging
from typing import List
from .capture_signal import CaptureSignal
from .probe_info import ProbeInfo
from .probe import Probe


logger = logging.getLogger(__name__)


class CaptureSignalFetcher:
    def __init__(self, probe: Probe):
        self.__probe = probe
        self.__packets = []
        self.__new_cycle = True

    def reset(self):
        self.__packets = []
        self.__new_cycle = True

    async def loop(self) -> (CaptureSignal | None):
        # Send command to snap a new capture signal.
        if self.__new_cycle:
            await self.__probe.write_command_capture_signal_snapshot()
            self.__packets = []
            self.__new_cycle = False
            return

        # Read next packet
        packet = await self.__probe.read_next_capture_signal_packet()
        if not packet:
            logger.error(f"Error reading capture signal packet.")
            self.reset()
            return None

        if (packet[0] != 0x40):
            logger.error(
                f"Unexpected capture signal packet format id: {packet[0]}.")
            self.reset()
            return None

        if ((packet[1] & 0x80) == 0):
            logger.error(
                f"Capture signal data not available. {packet[0]:02x}, {packet[1]:02x}")
            self.reset()
            return None

        self.__packets.append(packet)

        if (packet[1] & 0x01) != 0:
            return None  # More packets to read.

        result = CaptureSignal.decode(
            self.__packets, self.__probe.probe_info())
        self.reset()
        return result

    def amps_a(self) -> float:
        return self.amps_a

    def amps_b(self) -> float:
        return self.amps_b
