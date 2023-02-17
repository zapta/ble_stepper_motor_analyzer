# A test program for a bleak connection that is explicitly closed.

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
    #global client
    print(f"Trying to connect to {device_address}", flush=True)
    device = await BleakScanner.find_device_by_address(device_address, timeout=10.0)
    assert device
    async with BleakClient(device) as client:
        assert client.is_connected
        print(f"Connected", flush=True)
        print(f"Waiting 5 secs...", flush=True)
        time.sleep(5.0)
        print(f"Closing...", flush=True)
        assert client.is_connected
        await client.disconnect()
        print(f"Test done", flush=True)

asyncio.run(test())
