# import asyncio
from asyncio import futures
# import logging
import platform
# import re
import sys
# import concurrent.futures

from bleak import BleakScanner
from common.probe import Probe
from common import ble_util


def atexit_handler(probe, event_loop):
    # On Mac OSX we don't need to close the connection since the OS does it
    # itself. This eliminates the two secs or so delay exiting the program.
    # On Windows and Linux the OS doesn't close the connection and on Windows
    # it event try to reconnect if the device disconnect due to expired WDT.
    # This information is correct as of Apr 2023.
    platform_name = platform.uname().system
    if probe and probe.is_connected() and  platform_name not in [ "Darwin" ]:
            print("Disconnecting bluetooth connection.", flush=True)
            event_loop.run_until_complete(probe.disconnect())


def device_select_tuple_sort_key(tuple):
    # Tuple is [name, nickname]
    return tuple[0]


async def select_device_name():
    print("Scanning 5 secs for advertising BLE devices ...", flush=True)
    all_devices = await BleakScanner.discover(return_adv=True, timeout=5)
    candidates_devices = []
    for device, adv in all_devices.values():
        name, nickname, rssi = ble_util.extract_device_name_nickname_rssi(adv)
        # TODO: Use full name validation regex
        if name.startswith("STP-"):
            candidates_devices.append([name, nickname])

    if len(candidates_devices) == 0:
        print("No idle STP device found.", flush=True)
        sys.exit()

    if len(candidates_devices) == 1:
        print(f"Found a single STP device: {candidates_devices[0][0]}  ", flush=True)
        return candidates_devices[0][0]

    # Sort for a deterministic order. Here all the devices
    # have proper STP-xxx names.
    candidates_devices.sort(key=device_select_tuple_sort_key)

    while True:
        print("\n-----", flush=True)
        i = 0
        # Device is a tuple of [name, nickname]
        for device in candidates_devices:
            i += 1
            print(f"\n{i}. {device[0]}  {device[1]}", flush=True)

        ok = False
        try:
            num = int(input(f"\nSelect device 1 to {len(candidates_devices)}, 0 abort: "))
            if num == 0:
                print("\nUser asked to abort.\n", flush=True)
                sys.exit()
            if num > 0 and num <= len(candidates_devices):
                ok = True
        except ValueError:
            pass

        if ok:
            return candidates_devices[num - 1][0]


async def connect_to_probe(device_name):
    if not device_name:
        device_name = await select_device_name()
    print(f"Trying to connect to device [{device_name}]...", flush=True)
    probe = await Probe.find_by_name(device_name, timeout=5)
    if not probe:
        print(f"Device not found", flush=True)
        return None
    if not await probe.connect():
        print(f"Failed to connect", flush=True)
        return None

    print(f"Connected to probe", flush=True)
    probe.info().dump()
    if probe.info().current_ticks_per_amp() == 0:
        print(
            f"Device reported an invalid configuration of 0 current ticks"
            f" per Amp (hardware config {probe.info().hardware_config()}). Aborting.",
            flush=True)
        sys.exit()
    return probe
