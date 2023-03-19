import asyncio
import platform
import sys
import time
import signal
from bleak import BleakClient, BleakScanner
import signal

sys.path.append("..")
from common.filter import Filter

# ESP32 device
device_address = "0C:8B:95:F2:B4:36"
service_uuid = "6b6a78d7-8ee0-4a26-ba7b-62e357dd9720"
command_uuid = "ff06"
capture_uuid = "ff07"

# nRF device
#device_address = "EE:E7:C3:26:42:83"
#service_uuid =   "68e1a034-8125-4525-8a30-8799018c4bd0"
#command_uuid =   "37e75add-a610-448d-9fd3-3e3130e2c7f6"
#capture_uuid =   "37e75add-a610-448d-9fd3-3e3130e2c7f7"

signal.signal(signal.SIGINT, lambda number, frame: sys.exit())

print(f"OS: {platform.platform()}", flush=True)
print(f"Platform:: {platform.uname()}", flush=True)
print(f"Python {sys.version}", flush=True)


async def test():
    print(f"Trying to connect to {device_address}", flush=True)
    device = await BleakScanner.find_device_by_address(device_address, timeout=10.0)
    assert device
    async with BleakClient(device) as client:
        assert client.is_connected
        print(f"Connected", flush=True)
        service = client.services.get_service(service_uuid)
        assert (service)
        print(f"Found service", flush=True)
        command_chrc = service.get_characteristic(command_uuid)
        assert (command_chrc)
        capture_chrc = service.get_characteristic(capture_uuid)
        assert (capture_chrc)
        print(f"Found characteristics", flush=True)
        for i in range(5):
            print(f"Sending command", flush=True)
            await client.write_gatt_char(command_chrc, bytearray([0x02]))
            print(f"Reading", flush=True)
            bytes = await client.read_gatt_char(capture_chrc)
            print(f"Read1: {len(bytes)}", flush=True)
            bytes = await client.read_gatt_char(capture_chrc)
            print(f"Read2: {len(bytes)}", flush=True)
            # await asyncio.sleep(3)


asyncio.run(test())
