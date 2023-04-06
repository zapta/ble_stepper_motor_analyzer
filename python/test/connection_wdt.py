import asyncio
import platform
import sys
import time
import signal
from bleak import BleakClient, BleakScanner
import signal

# Adapt to your actual device.
device_address = "0C:8B:95:F2:B4:36"

signal.signal(signal.SIGINT, lambda number, frame: sys.exit())

print(f"OS: {platform.platform()}", flush=True)
print(f"Platform:: {platform.uname()}", flush=True)
print(f"Python {sys.version}", flush=True)


async def callback_handler(sender, data):
    print(f"*** Notification", flush=True)


async def test():
    print(f"Trying to connect to {device_address}", flush=True)
    device = await BleakScanner.find_device_by_address(device_address, timeout=10.0)
    assert device
    client = BleakClient(device)
    await client.connect()
    assert client.is_connected()
    print(f"Connected", flush=True)
    service = client.services.get_service("6b6a78d7-8ee0-4a26-ba7b-62e357dd9720")
    assert (service)
    notif_chrc = service.get_characteristic("ff02")
    assert (notif_chrc)
    await client.start_notify(notif_chrc, callback_handler)
    command_chrc = service.get_characteristic("ff06")
    assert (command_chrc)
    await client.write_gatt_char(command_chrc, bytearray([0x06, 3]), response=True)
    await asyncio.sleep(1)
    # await client.disconnect()
    print(f"Done", flush=True)


asyncio.run(test())
