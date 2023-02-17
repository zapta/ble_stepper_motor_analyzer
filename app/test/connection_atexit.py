# A test program for a bleak connection with atexit cleanup.

import asyncio
import platform
import sys
import atexit
import time
import signal
from bleak import BleakClient, BleakScanner
import signal
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--conn_cleanup", dest="conn_cleanup",
                    default='auto', choices=['auto', 'yes', 'no'],
                    help="Specifies if to explicitly close the connection on program exit.")
args = parser.parse_args()


# print(f"Cleanup: {args.conn_cleanup}", flush=True)


signal.signal(signal.SIGINT, lambda number, frame: sys.exit())

client = None
event_loop = asyncio.new_event_loop()

# Return True or False.


def should_cleanup_connection():
    str = args.conn_cleanup
    if str == "yes":
        return True
    if str == "no":
        return False
    return ('linux' in platform.platform().lower())


def atexit_cleanup():
    is_connected = (client and client.is_connected)
    if not should_cleanup_connection():
        if is_connected:
            print(f"Cleanup disabled (connected)")
        else:
            print(f"Cleanup disabled (not connected)")
    elif is_connected:
        print(f"At exit: disconnecting device.", flush=True)
        event_loop.run_until_complete(client.disconnect())
    else:
        print(f"At exit: No connection to cleanup.", flush=True)


atexit.register(atexit_cleanup)


# Adapt to your actual device.
device_address = "0C:8B:95:F2:B4:36"

print(f"OS: {platform.platform()}", flush=True)
print(f"Platform:: {platform.uname()}", flush=True)
print(f"Python {sys.version}", flush=True)
print(f"Cleanup: '{args.conn_cleanup}' -> {should_cleanup_connection()}",
      flush=True)


async def test():
    global client
    print(f"Trying to connect to {device_address}", flush=True)
    device = await BleakScanner.find_device_by_address(device_address, timeout=10.0)
    assert device
    client = BleakClient(device)
    assert client
    await client.connect(timeout=10.0)
    assert client.is_connected
    print(f"Connected", flush=True)
    print(f"Waiting 5 secs...", flush=True)
    time.sleep(5.0)
    print(f"Test done", flush=True)

# event_loop = asyncio.new_event_loop()
event_loop.run_until_complete(test())
