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
parser.add_argument("--device", dest="device", default=None,
                    help="The device name or address")
args = parser.parse_args()

logging.basicConfig(level=logging.INFO)

# Global variables
probe = None
main_event_loop = asyncio.new_event_loop()


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
