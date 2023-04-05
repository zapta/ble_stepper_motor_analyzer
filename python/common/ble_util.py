from bleak import BleakScanner
import re


def is_valid_nickname(device_nickname, empty_ok = False):
    if  device_nickname is None:
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


def extract_device_name_and_nickname(adv):
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

    return device_name, device_nickname
