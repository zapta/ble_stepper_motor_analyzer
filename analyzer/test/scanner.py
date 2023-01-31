#!python

import asyncio
from bleak import BleakScanner

async def scan():
    print("Scanning for 5 secs ...", flush=True)
    scanner = BleakScanner()
    devices = await scanner.discover(timeout=5)
    for device in devices:
        print(device, flush=True)
        #name = device.name or ""
        #print(f"address: {device.address}  name: {name}", flush=True)


asyncio.run(scan())
