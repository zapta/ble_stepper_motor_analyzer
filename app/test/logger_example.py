#!python

# A python program to connect to the device and log basic stepper info.



import argparse
import platform
import asyncio
import logging
import signal
import sys
import atexit
import time

sys.path.append("..")
from common.probe import Probe
from common.probe_state import ProbeState
from common import connections

parser = argparse.ArgumentParser()
parser.add_argument('--scan', dest="scan", default=False,
                    action=argparse.BooleanOptionalAction, help="If specified, scan for devices and exit.")
parser.add_argument("-d", "--device", dest="device",
                    default=None, help="The device name or address")
args = parser.parse_args()

main_event_loop = asyncio.new_event_loop()


# # The default device used by the developers. For conviniance.
# DEFAULT_DEVICE_ADDRESS = "0C:8B:95:F2:B4:36"

# Allows to stop the program by typing ctrl-c.
signal.signal(signal.SIGINT, lambda number, frame: sys.exit())

probe = None


async def dummy():
    pass


def atexit_cleanup():
    global probe
    is_connected = (probe and probe.is_connected())
    is_linux = 'linux' in platform.platform().lower()
    print(
        f"atexit: is_connected = {is_connected}, is_linux = {is_linux}", flush=True)
    if is_connected and is_linux:
        print(f"atexit: Disconnecting", flush=True)
        main_event_loop.run_until_complete(probe.disconnect())
        for i in range(20):
            main_event_loop.run_until_complete(dummy())
            time.sleep(0.1)


atexit.register(atexit_cleanup)

logging.basicConfig(level=logging.INFO)

# parser = argparse.ArgumentParser()
# parser.add_argument("-d", "--device_address", dest="device_address",
#                     default=DEFAULT_DEVICE_ADDRESS, help="The device address")
# args = parser.parse_args()

# Caches the first device state the logger recieves.
first_state = None


# Sets probe.
# async def connect_to_probe():
#     """ Co-routing. Returns Probe or None. """
#     global probe
#     device_address = args.device_address
#     print(
#         f"Trying to connect to device [{device_address}]...", file=sys.stderr, flush=True)
#     probe = await Probe.find_by_address(device_address, 1.0)
#     if not probe:
#         print(f"Device not found", file=sys.stderr, flush=True)
#         return None
#     if not await probe.connect():
#         print(f"Failed to connect", file=sys.stderr, flush=True)
#         return None
#     print(f"Connected to probe", file=sys.stderr, flush=True)
#     probe.info().dump(file=sys.stderr)


def state_notification_callback_handler(state: ProbeState):
    """ Called on each device state notification (50 Hz) """
    global first_state
    if not first_state:
        first_state = state
        print(f"T[secs],Steps,A[Amps],B[Amps],I[Amps]", flush=True)
    # Make the timestamp and distance relative to logger start state.
    rel_timestamp_secs = state.timestamp_secs - first_state.timestamp_secs
    rel_distance_steps = state.steps - first_state.steps
    print(f"{rel_timestamp_secs:.3f},{rel_distance_steps:.2f},{state.amps_a:.2f},{state.amps_b:.2f},{state.amps_abs:.2f}", flush=True)


async def init():
    # Connect to device.
    probe = await connections.connect_to_probe(args.device)
    assert (probe)
    # Install the notifications callback handler.
    await probe.set_state_notifications(state_notification_callback_handler)


if args.scan:
    asyncio.run(connections.scan_and_dump())
    print(f"User requested just to scan.", flush=True)
else:
    main_event_loop.run_until_complete(init())
    main_event_loop.run_forever()
