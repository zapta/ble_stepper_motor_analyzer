#!python

# A python program to scan the bluetooth BLE devices
# nearby, looking for stepper motor analyzer ids which
# look like "STP-EA2307AE0794"

import asyncio
from bleak import BleakScanner

async def scan():
    print("Scanning for 5 secs ...", flush=True)
    devices = await BleakScanner.discover(timeout=5)
    for device in devices:
        # print(device, flush=True)
        name = device.name or ""
        print(f"address: {device.address}  name: {name}", flush=True)


asyncio.run(scan())
