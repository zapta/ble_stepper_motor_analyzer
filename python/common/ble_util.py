from bleak import BleakScanner
import re


def is_valid_nickname(device_nickname, empty_ok=False):
    if device_nickname is None:
        return False
    if device_nickname == "":
        return empty_ok
    match = re.search("^[a-zA-Z0-9. -_#]{1,16}$", device_nickname)
    return bool(match)


def is_valid_device_name(device_name):
    if not device_name:
        return False
    match = re.search(
        "^STP-([0-9A-F]{2})([0-9A-F]{2})([0-9A-F]{2})"
        "([0-9A-F]{2})([0-9A-F]{2})([0-9A-F]{2})$", device_name)
    return bool(match)


def is_adv_complete(adv):
    """Specific for stepper probe devices"""
    return (adv.local_name and is_valid_device_name(adv.local_name) and
            (adv.manufacturer_data or adv.service_uuids))


def extract_device_name_nickname_rssi(adv):
    """Extracts optional name and optional nickname from adv data.
    Should be graceful to general, stepper devices BLE devices. """
    device_name = adv.local_name or ""

    # Extract device name from manufacturer data
    device_nickname = ""
    if is_valid_device_name(device_name) and adv.manufacturer_data:
        # 4369 is the BLE code that the device uses to encode the nickname.
        manufacturer_value = adv.manufacturer_data.get(4369, None)
        if manufacturer_value:
            device_nickname = manufacturer_value.decode('utf-8')

    # Extract rssi.
    rssi = "xx"
    if adv.rssi:
        rssi = str(adv.rssi)

    return device_name, device_nickname, rssi


def scan_tuple_sort_key(tuple):
    # Tuple is [name, nickname]
    return [tuple[1], tuple[2], tuple[0]]


async def scan_and_dump():
    print("Scanning 5 secs for advertising BLE devices ...", flush=True)
    devices = await BleakScanner.discover(return_adv=True, timeout=5)
    if not devices:
        print("No BLE devices found", flush=True)
        return
    tuples = []
    max_address_len = 7
    max_name_len = 4
    max_nickname_len = 8
    for device, adv in devices.values():
        name, nickname, rssi = extract_device_name_nickname_rssi(adv)
        tuples.append([device.address, name, nickname, rssi])
        max_address_len = max(max_address_len, len(device.address))
        max_name_len = max(max_name_len, len(name))
        max_nickname_len = max(max_nickname_len, len(nickname))

    tuples.sort(key=scan_tuple_sort_key)

    print(
        f"\n{' #':2}  {'ADDRESS':{max_address_len}}   {'NAME':{max_name_len}}   {'NICKNAME':{max_nickname_len}}   RSSI",
        flush=True)
    print("-" * (17 + max_address_len + max_name_len + max_nickname_len))
    i = 0
    for tuple in tuples:
        i += 1
        print(
            f"{i:2}  {tuple[0]:{max_address_len}}   {tuple[1]:{max_name_len}}   {tuple[2]:{max_nickname_len}}   {tuple[3]:4}",
            flush=True)
