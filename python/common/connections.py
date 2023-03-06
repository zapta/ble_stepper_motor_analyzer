import asyncio
import logging
import platform
import re
import sys

from bleak import BleakScanner


from .probe import Probe


def is_mac():
    """Return true if running under Mac OSX. """
    return sys.platform == "darwin"


def atexit_handler(_probe, _event_loop):
    print("\natexit: invoked")
    if _probe and _probe.is_connected():
        _event_loop.run_until_complete(_probe.write_command_conn_wdt(1))
        print("atexit: Device should disconnect in 1 sec", flush=True)
    else:
        print("atexit: Not connected")


async def scan_and_dump():
    print("Scanning 5 secs for advertising BLE devices ...", flush=True)
    devices = await BleakScanner.discover(timeout=5)
    i = 0
    for device in devices:
        i += 1
        # print(device, flush=True)
        name = device.name or ""
        print(f"{i:2} device address: {device.address}  ({name})", flush=True)


async def parse_device_spec(device_spec):
    # Get flag value.
    value = device_spec
    if not value:
        print(f"Device not specified.")
        return None
    value = value.strip().upper()
    print(f"Specified device: [{value}]", flush=True)

    # Handle the case of a direct address. Six dual hex digit values,
    # separated by colons.
    if is_mac():
        print(f"Running on Mac.", flush=True)
        # 9BAEB8A6-E88B-4B4B-1F1B-529C5141D620
        addr_match = re.search(
            "^[0-9A-F]{8}-[0-9A-F]{4}-[0-9A-F]{4}"
            "-[0-9A-F]{4}-[0-9A-F]{12}$", value)
    else:
        print(f"Not running on Mac.", flush=True)
        addr_match = re.search(
            "^[0-9A-F]{2}:[0-9A-F]{2}:[0-9A-F]{2}"
            ":[0-9A-F]{2}:[0-9A-F]{2}:[0-9A-F]{2}$", value)
    if addr_match:
        return value

    # Handle the case of a device name.
    name_match = re.search(
        "^STP-([0-9A-F]{2})([0-9A-F]{2})([0-9A-F]{2})"
        "([0-9A-F]{2})([0-9A-F]{2})([0-9A-F]{2})$", value)

    if not name_match:
        sys.exit(
            f"Can't figure device name or address, please check --device flag. Aborting.")

    # On non mac systems, the address can be parsed from the device name.
    if not is_mac:
        return (f"{name_match.group(1)}:{name_match.group(2)}"
                f":{name_match.group(3)}:{name_match.group(4)}"
                f":{name_match.group(5)}:{name_match.group(6)}")

    # On mac, the system assign its own device and therefore we need
    # to scan and look the device address by name.
    print(f"Running on Mac. Scanning to find device named {value}", flush=True)
    devices = await BleakScanner.discover(timeout=5)
    for device in devices:
        if device.name == value:
            return device.address

    sys.exit(
        f"Did not find a device named {value}.")


def get_device_key(device):
    return device.name


async def select_device_address():
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
            f"Found a single STP device: device address: {candidates_devices[0].address}  ({name})", flush=True)
        return candidates_devices[0].address

    # Sort for a deterministic order. Here all the devices
    # have proper STP-xxx names.
    candidates_devices.sort(key=get_device_key)

    while True:
        print("\n-----", flush=True)
        i = 0
        for device in candidates_devices:
            i += 1
            print(f"\n{i}. {device.address}  {device.name}",  flush=True)

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
            return candidates_devices[num - 1].address


async def connect_to_probe_at_address(device_address):
    # device_address = args.device_address
    print(
        f"Trying to connect to device at address [{device_address}]...", flush=True)
    probe = await Probe.find_by_address(device_address)
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


async def connect_to_probe(device_spec):
    """device_spec is device address or name"""
    # Determine device address.
    device_address = await parse_device_spec(device_spec)

    if not device_address:
        device_address = await select_device_address()

    print(f"Device address: [{device_address}]", flush=True)

    # logging.basicConfig(level=logging.INFO)
    probe = await connect_to_probe_at_address(device_address)
    return probe
