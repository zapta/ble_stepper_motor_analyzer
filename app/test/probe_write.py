
import argparse
import asyncio
import logging
import signal
import sys
import atexit

# A workaround to avoid auto formatting.
if True:
    sys.path.append("..")
    from common import connections
    from common.probe_state import ProbeState
    from common.probe import Probe


# Allows to stop the program by typing ctrl-c.
signal.signal(signal.SIGINT, lambda number, frame: sys.exit())

# Command line flags.
parser = argparse.ArgumentParser()
parser.add_argument("--device", dest="device",
                    default=None, help="The device name or address")
args = parser.parse_args()

logging.basicConfig(level=logging.INFO)

# Global variables
probe = None
main_event_loop = asyncio.new_event_loop()


# def state_notification_callback_handler(state: ProbeState):
#     """ An handler that is called on incoming device state reports (50 Hz) """
#     global first_state
#     if not first_state:
#         first_state = state
#         print(f"T[secs],Steps,A[Amps],B[Amps],I[Amps]", flush=True)
#     # Make the timestamp and distance relative to logger start state.
#     rel_timestamp_secs = state.timestamp_secs - first_state.timestamp_secs
#     rel_distance_steps = state.steps - first_state.steps
#     print(f"{rel_timestamp_secs:.3f},{rel_distance_steps:.2f},{state.amps_a:.2f},{state.amps_b:.2f},{state.amps_abs:.2f}", flush=True)



async def async_main():
    global probe
    # Connect to device.
    probe = await connections.connect_to_probe(args.device)
    assert (probe)
    atexit.register(connections.atexit_handler, _probe=probe,
                    _event_loop=main_event_loop)
    i = 0
    while True:
        i += 1
        print(f"\n{i} Sending command...", flush=True)
        await probe.write_command_conn_wdt(3)
        print(f"done.", flush=True)
        await asyncio.sleep(1)


main_event_loop.run_until_complete(async_main())
main_event_loop.run_forever()
