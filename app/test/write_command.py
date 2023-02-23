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

async def test():
    print(f"Trying to connect to {device_address}", flush=True)
    device = await BleakScanner.find_device_by_address(device_address, timeout=10.0)
    assert device
    async with BleakClient(device) as client:
        assert client.is_connected
        print(f"Connected", flush=True)
        service = client.services.get_service("6b6a78d7-8ee0-4a26-ba7b-62e357dd9720")
        assert(service)
        print(f"Found service", flush=True)
        chrc = service.get_characteristic("ff06")
        assert(chrc)
        print(f"Found characteristic", flush=True)
        for i in range(5):
          print(f"\n{i + 1} Sending command", flush=True)
          await client.write_gatt_char(chrc, bytearray([0x06, 0x03]))
          print("done.")
          await asyncio.sleep(1)

asyncio.run(test())