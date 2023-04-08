import asyncio
import platform
import sys
import time
import signal
from bleak import BleakClient, BleakScanner
import signal

# Adapt to your actual device.
device_name = "STP-0C8B95F2B436"

signal.signal(signal.SIGINT, lambda number, frame: sys.exit())

print(f"OS: {platform.platform()}", flush=True)
print(f"Platform:: {platform.uname()}", flush=True)
print(f"Python {sys.version}", flush=True)


async def notification_callback(sender, data):
    print(f"*** Notification", flush=True)


async def test():
    print(f"Trying to connect to {device_name}", flush=True)
    device = await BleakScanner.find_device_by_filter(
        lambda device, adv: adv.local_name == device_name, timeout=5.0)
    assert device
    client = BleakClient(device)
    await client.connect()
    assert client.is_connected()
    print(f"Connected", flush=True)
    service = client.services.get_service("6b6a78d7-8ee0-4a26-ba7b-62e357dd9720")
    assert (service)
    notif_chrc = service.get_characteristic("ff02")
    assert (notif_chrc)
    # Tell the device to start sending notifications.
    await client.start_notify(notif_chrc, notification_callback)
    # command_chrc = service.get_characteristic("ff06")
    # assert (command_chrc)
    # Send to the device a command to disconnect in 5 secs.
    # await client.write_gatt_char(command_chrc, bytearray([0x06, 5]), response=True)
    await asyncio.sleep(1)
    # Commenting the statement below causes under windows
    # delayed disconnect 30 seconds after the program exist.
    await client.disconnect()
    print(f"Done", flush=True)


asyncio.run(test())
