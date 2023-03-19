import asyncio
import logging
import platform
import re
import sys

from bleak import BleakScanner


from .probe import Probe


def atexit_handler(_probe, _event_loop):
    print("\natexit: invoked", flush=True)
    if _probe and _probe.is_connected():
        _event_loop.run_until_complete(_probe.write_command_conn_wdt(1))
        print("atexit: Device should disconnect in 1 sec", flush=True)
    else:
        print("atexit: Not connected", flush=True)


async def scan_and_dump():
    print("Scanning 5 secs for advertising BLE devices ...", flush=True)
    devices = await BleakScanner.discover(timeout=5)
    i = 0
    for device in devices:
        i += 1
        # print(device, flush=True)
        name = device.name or ""
        print(f"{i:2} device address: {device.address}  ({name})", flush=True)


def get_device_key(device):
    return device.name


async def select_device_name():
    print("Scanning 5 secs for advertising BLE devices ...", flush=True)
    all_devices = await BleakScanner.discover(timeout=5)
    candidates_devices = []
    for device in all_devices:
        name = device.name or ""
        if name.startswith("STP-"):
            candidates_devices.append(device)

    if len(candidates_devices) == 0:
        sys.exit("No idle STP device found.")
        return None

    if len(candidates_devices) == 1:
        print(
            f"Found a single STP device: {candidates_devices[0].name}  ", flush=True)
        return candidates_devices[0].name

    # Sort for a deterministic order. Here all the devices
    # have proper STP-xxx names.
    candidates_devices.sort(key=get_device_key)

    while True:
        print("\n-----", flush=True)
        i = 0
        for device in candidates_devices:
            i += 1
            print(f"\n{i}. {device.name}",  flush=True)

        ok = False
        try:
            num = int(
                input(f"\nSelect device 1 to {len(candidates_devices)}, 0 abort: "))
            if num == 0:
                sys.exit("\nUser asked to abort.\n")
            if num > 0 and num <= len(candidates_devices):
                ok = True
        except ValueError:
            pass

        if ok:
            return candidates_devices[num - 1].name


async def connect_to_probe(device_name):
    """device_spec is device address or name"""

    if device_name:
        name_match = re.search(
            "^STP-([0-9A-F]{2})([0-9A-F]{2})([0-9A-F]{2})"
            "([0-9A-F]{2})([0-9A-F]{2})([0-9A-F]{2})$", device_name)

        if not name_match:
            sys.exit(f"Invalid device name: {device_name}. Aborting.")

    else:
        device_name = await select_device_name()

    print(
        f"Trying to connect to device [{device_name}]...", flush=True)
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
        sys.exit(f"Device reported an invalid configuration of 0 current ticks"
                 f" per Amp (hardware config {probe.info().hardware_config()}). Aborting.")
    return probe
