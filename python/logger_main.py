#!python

# A python program to connect to the device and log basic stepper info.

import argparse
import asyncio
import logging
import signal
import sys

from probe import Probe
from probe_state import ProbeState


# The default device used by the developers. For conviniance.
DEFAULT_DEVICE_ADDRESS = "EE:E7:C3:26:42:83"

# Allows to stop the program by typing ctrl-c.
signal.signal(signal.SIGINT, lambda number, frame: sys.exit())

logging.basicConfig(level=logging.INFO)

parser = argparse.ArgumentParser()
parser.add_argument("-d", "--device_address", dest="device_address",
                    default=DEFAULT_DEVICE_ADDRESS, help="The device address")
args = parser.parse_args()

# Caches the first device state the logger recieves.
first_state = None


async def connect_to_probe():
    """ Co-routing. Returns Probe or None. """
    device_address = args.device_address
    print(
        f"Trying to connect to device [{device_address}]...", file=sys.stderr, flush=True)
    probe = await Probe.find_by_address(device_address, 1.0)
    if not probe:
        print(f"Device not found", file=sys.stderr, flush=True)
        return None
    if not await probe.connect():
        print(f"Failed to connect", file=sys.stderr, flush=True)
        return None
    print(f"Connected to probe", file=sys.stderr, flush=True)
    probe.info().dump(file=sys.stderr)
    return probe


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


# Connect to device.
main_event_loop = asyncio.new_event_loop()
probe = main_event_loop.run_until_complete(connect_to_probe())

# Install the notifications callback handler.
main_event_loop.run_until_complete(
    probe.set_state_notifications(state_notification_callback_handler))

# Start the logging.
main_event_loop.run_forever()
