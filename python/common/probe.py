# The BLE device API. Used to by analyzer_main to access the device.
# If you are writing your own device monitoring main program, this is
# the API to use.

from __future__ import annotations

import logging
from typing import Callable, Optional

from bleak import BleakClient, BleakScanner
from bleak.backends.service import BleakGATTCharacteristic, BleakGATTService

from common import ble_util

from common.current_histogram import CurrentHistogram
from common.distance_histogram import DistanceHistogram
from common.probe_info import ProbeInfo
from common.probe_state import ProbeState
from common.time_histogram import TimeHistogram

logger = logging.getLogger(__name__)



class Probe:

    def __init__(self, client: BleakClient, name: str, nickname: str):
        self.__client = client
        self.__name = name
        self.__nickname = nickname
        self.__probe_info = None
        self.__stepper_state_chrc = None
        self.__stepper_current_histogram_chrc = None
        self.__stepper_time_histogram_chrc = None
        self.__stepper_distance_histogram_chrc = None
        self.__stepper_command_chrc = None
        self.__capture_signal_chrc = None

    def __str__(self) -> str:
        return self.__client.address

    # Get device address.
    def address(self) -> str:
        return self.__client.address

    # Get device name.
    def name(self) -> str:
        return self.__name
    
    # Get device nickname. Optional, may be empty.
    def nickname(self) -> str:
        return self.__nickname

    # Get chached device info.

    def probe_info(self) -> (ProbeInfo | None):
        return self.__probe_info

    # Internal method to fetch information of a specific BLE service.
    async def __find_service_or_disconnect(self, name: str, uuid: str) -> (BleakGATTService | None):
        if not self.is_connected():
            logger.error(f"Not connected (__find_service_or_disconnect).")
            return None
        service = self.__client.services.get_service(uuid)
        if not service:
            logger.error(f"Failed to find service {name} at device {self.address()}.")
            await self.disconnect()
            return None
        logger.info(f"Found {name} info service {service}.")
        return service

    # Internal method to fetch information of a specific BLE characteristic.
    async def __find_chrc_or_disconnect(self, service: BleakGATTService, name: str,
                                        uuid: str) -> (BleakGATTCharacteristic | None):
        if not self.is_connected():
            logger.error(f"Not connected (__find_chrc_or_disconnect).")
            return None
        chrc = service.get_characteristic(uuid)
        if not chrc:
            logger.error(f"Failed to find characteristic {name} of device {self.address()}.")
            await self.disconnect()
            return None
        logger.info(f"Found {name} characteristic {chrc}.")
        return chrc

    # An internal method to read a BLE charateristic's value.
    async def __read_chrc_or_disconnect(self, service: BleakGATTService, name: str,
                                        uuid: str) -> (bytearray | None):
        if not self.is_connected():
            logger.error(f"Not connected (__read_chrc_or_disconnect).")
            return None
        chrc = await self.__find_chrc_or_disconnect(service, name, uuid)
        if not chrc:
            return None
        val_bytes = await self.__client.read_gatt_char(chrc)
        return val_bytes

    device_advertisement_data = None

    @classmethod
    def find_by_name_lambda(cls, advertisement_data, device_name):
        """A callback for find_by_name(), matches the device name/nickname."""
        global device_advertisement_data
        # Not a probe device or waiting for second adv packet.
        if not ble_util.is_adv_complete(advertisement_data):
            return False
        name, nickname = ble_util.extract_device_name_and_nickname(advertisement_data)
        # Since bleak can call this callback before the second ad packet with the
        # nickname is available, we require that bot name and nickname exist.
        device_match =  name == device_name or nickname == device_name
        if device_match:
            device_advertisement_data = advertisement_data
        return device_match

 
    @classmethod
    async def find_by_name(cls, requested_device_name, timeout):
        """Find a device by its full name or its nickname"""
        global device_advertisement_data
        device_advertisement_data = None
        device = await BleakScanner.find_device_by_filter(
            lambda device, advertisement_data: Probe.find_by_name_lambda(
                advertisement_data, requested_device_name),
            timeout=timeout)
        if not device:
            logger.error(f"Device {requested_device_name} not found.")
            return None
        device_name, device_nickname = ble_util.extract_device_name_and_nickname(device_advertisement_data)
        logger.info(f"Found device {device_name} ({device_nickname}) at address {device.address}")
        client = BleakClient(device)
        probe = Probe(client, device_name, device_nickname)
        return probe

    def is_connected(self) -> bool:
        return self.__client.is_connected

    async def connect(self, timeout: float = 20.0) -> bool:
        if self.is_connected():
            return True

        await self.__client.connect(timeout=timeout)
        if not self.is_connected():
            logger.error(f"Failed to connect to {self.address()}.")
            return False

        for s in self.__client.services.services.values():
            logger.info(f"* Service: {s}")

        # device_info_service = await self.__find_service_or_disconnect("Device Info", "180a")
        # if not device_info_service:
        #     return False

        # stepper_service = await self.__find_service_or_disconnect("Stepper", "68e1a034-8125-4525-8a30-8799018c4bd0")
        stepper_service = await self.__find_service_or_disconnect(
            "Stepper", "6b6a78d7-8ee0-4a26-ba7b-62e357dd9720")
        if not stepper_service:
            return False

        model_number_bytes = await self.__read_chrc_or_disconnect(stepper_service, "Model Number",
                                                                  "2A24")
        if not model_number_bytes:
            return False

        manufacturer_bytes = await self.__read_chrc_or_disconnect(stepper_service, "Manufacturer",
                                                                  "2A29")
        if not manufacturer_bytes:
            return False

        probe_info_bytes = await self.__read_chrc_or_disconnect(stepper_service, "Probe Info",
                                                                "ff01")
        if not probe_info_bytes:
            return False

        # Get stepper state characteristic.
        stepper_state_chrc = await self.__find_chrc_or_disconnect(stepper_service, "Stepper State",
                                                                  "ff02")
        if not stepper_state_chrc:
            return False

        # Get current histogram characteristic.
        stepper_current_histogram_chrc = await self.__find_chrc_or_disconnect(
            stepper_service, "Current Histogram", "ff03")
        if not stepper_current_histogram_chrc:
            return False

        # Get time histogram characteristic.
        stepper_time_histogram_chrc = await self.__find_chrc_or_disconnect(
            stepper_service, "Time Histogram", "ff04")
        if not stepper_time_histogram_chrc:
            return False

        # Get distance histogram characteristic.
        stepper_distance_histogram_chrc = await self.__find_chrc_or_disconnect(
            stepper_service, "Distance Histogram", "ff05")
        if not stepper_distance_histogram_chrc:
            return False

        # Get stepper command characteristic.
        stepper_command_chrc = await self.__find_chrc_or_disconnect(stepper_service, "Command",
                                                                    "ff06")
        if not stepper_command_chrc:
            return False

        # Get capture signal characteristic.
        capture_signal_chrc = await self.__find_chrc_or_disconnect(stepper_service, "Capture",
                                                                   "ff07")
        if not capture_signal_chrc:
            return False

        # Set this object.
        self.__probe_info = ProbeInfo.decode(probe_info_bytes, model_number_bytes.decode(),
                                             manufacturer_bytes.decode())
        self.__stepper_state_chrc = stepper_state_chrc
        self.__stepper_current_histogram_chrc = stepper_current_histogram_chrc
        self.__stepper_time_histogram_chrc = stepper_time_histogram_chrc
        self.__stepper_distance_histogram_chrc = stepper_distance_histogram_chrc
        self.__stepper_command_chrc = stepper_command_chrc
        self.__capture_signal_chrc = capture_signal_chrc

        logger.info(f"Connected to {self.address()}.")
        return True

    # Get device info. This is static device specific data that doesn't
    # change.
    def info(self) -> (ProbeInfo | None):
        return self.__probe_info

    # This is the polling method to get the device state. For a continious
    # state report, use the notification method instead.
    async def read_device_state(self) -> Optional[ProbeState]:
        if not self.is_connected():
            logger.error(f"Not connected (read_state)")
            return None
        val_bytes = await self.__client.read_gatt_char(self.__stepper_state_chrc)
        return ProbeState.decode(val_bytes, self.__probe_info)

    async def read_current_histogram(self, steps_per_unit=1.0) -> Optional[CurrentHistogram]:
        if not self.is_connected():
            logger.error(f"Not connected (read_current_histogram).")
            return None
        val_bytes = await self.__client.read_gatt_char(self.__stepper_current_histogram_chrc)
        return CurrentHistogram.decode(val_bytes, self.__probe_info, steps_per_unit)

    async def read_time_histogram(self, steps_per_unit=1.0) -> Optional[TimeHistogram]:
        if not self.is_connected():
            logger.error(f"Not connected (read_time_histogram).")
            return None
        val_bytes = await self.__client.read_gatt_char(self.__stepper_time_histogram_chrc)
        return TimeHistogram.decode(val_bytes, self.__probe_info, steps_per_unit)

    async def read_distance_histogram(self, steps_per_unit=1.0) -> Optional[DistanceHistogram]:
        if not self.is_connected():
            logger.error(f"Not connected (read_distance_histogram).")
            return None
        val_bytes = await self.__client.read_gatt_char(self.__stepper_distance_histogram_chrc)
        return DistanceHistogram.decode(val_bytes, self.__probe_info, steps_per_unit)

    async def write_command_reset_data(self):
        if not self.is_connected():
            logger.error(f"Not connected (write_command_reset_data).")
            return
        await self.__client.write_gatt_char(self.__stepper_command_chrc, bytearray([0x01]))

    async def write_command_capture_signal_snapshot(self):
        if not self.is_connected():
            logger.error(f"Not connected (write_command_capture_signal_snapshot).")
            return
        await self.__client.write_gatt_char(self.__stepper_command_chrc, bytearray([0x02]))

    async def write_command_set_capture_divider(self, divider):
        if not self.is_connected():
            logger.error(f"Not connected (write_command_set_capture_divider).")
            return
        arg = max(0, min(255, int(divider)))
        await self.__client.write_gatt_char(self.__stepper_command_chrc, bytearray([0x03, arg]))

    # Changes forward/backward direction interpretation. The new direction
    # is persisted on the device.
    async def write_command_toggle_direction(self):
        if not self.is_connected():
            logger.error(f"Not connected (write_command_toggle_direction).")
            return
        # print("Toggle direction command", flush=True)
        await self.__client.write_gatt_char(self.__stepper_command_chrc, bytearray([0x04]))

    # Should be done with steppers disconnected or turned off. New zero calibration
    # is persisted on the device.
    async def write_command_zero_calibration(self):
        if not self.is_connected():
            logger.error(f"Not connected (write_command_zero_calibration).")
            return
        print("Zero calibration command", flush=True)
        await self.__client.write_gatt_char(self.__stepper_command_chrc, bytearray([0x05]))

    # Called periodically to maintain connection wdt. Useful for systems where connection
    # is maintained by the OS even after the program exits.
    async def write_command_conn_wdt(self, secs):
        if not self.is_connected():
            logger.error(f"Not connected (write_command_conn_wdt).")
            return
        # print(f"Conn wdt command {secs} secs", flush=True)
        await self.__client.write_gatt_char(self.__stepper_command_chrc, bytearray([0x06, secs]))

    async def write_command_set_nickname(self, nickname):
        if not self.is_connected():
            logger.error(f"Not connected (write_command_set_nickname).")
            return
        str_bytes = nickname.encode()
        str_len = len(str_bytes)
        cmd_bytes = bytearray([0x07, str_len]) + str_bytes
        # print(f"cmd_bytes: {cmd_bytes}")
        await self.__client.write_gatt_char(self.__stepper_command_chrc, cmd_bytes)

    async def read_next_capture_signal_packet(self) -> Optional[bytearray]:
        if not self.is_connected():
            logger.error(f"Not connected (read_capture_signal_packet).")
            return None
        return await self.__client.read_gatt_char(self.__capture_signal_chrc)

    async def set_state_notifications(self, handler: Callable[[ProbeState], None]):
        # Adapter handler.
        async def callback_handler(sender, data):
            probe_state = ProbeState.decode(data, self.__probe_info)
            if handler:
                handler(probe_state)

        if not self.is_connected():
            logger.error(f"Not connected (callback_handler).")
            return None
        await self.__client.start_notify(self.__stepper_state_chrc, callback_handler)
        logger.info(f"Started device state notifications.")

    # Problematic on Windows per https://github.com/hbldh/bleak/issues/1223
    async def disconnect(self):
        logger.info(f"Disconnecting.")
        if not self.is_connected():
            self.__client.disconnect()
        # self.__probe_info = None
        # self.__stepper_state_chrc = None
        # self.__stepper_current_histogram_chrc = None
        # self.__stepper_time_histogram_chrc = None
        # self.__stepper_distance_histogram_chrc = None
        return
