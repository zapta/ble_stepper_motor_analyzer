
import argparse
import asyncio
import logging
import signal
import sys
import atexit
import time

# A workaround to avoid auto formatting.
if True:
    sys.path.append("..")
    from common import connections
    from common.probe_state import ProbeState
    from common.probe import Probe
    from common.capture_signal_fetcher import CaptureSignalFetcher


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
#first_state = None
notifications = 0


def state_notification_callback_handler(state: ProbeState):
    """ An handler that is called on incoming device state reports (50 Hz) """
    global notifications
    notifications += 1

async def main():
    """ Connects and initialize the device."""
    global probe
    # Connect to device.
    probe = await connections.connect_to_probe(args.device)
    assert (probe)
    await probe.write_command_conn_wdt(5)
    atexit.register(connections.atexit_handler, _probe=probe,
                    _event_loop=main_event_loop)
    await probe.set_state_notifications(state_notification_callback_handler)
    capture_signal_fetcher = CaptureSignalFetcher(probe)
    prev_time= time.time()
    prev_notifications = notifications
    while True:
      await probe.write_command_conn_wdt(5)
      #print(f"Loop", flush=True)
      capture_signal = await capture_signal_fetcher.loop()
      if capture_signal:
        time_now = time.time()
        elapsed = time_now - prev_time
        prev_time = time_now
        notifications_now = notifications
        elapsed_notifications = notifications_now - prev_notifications
        prev_notifications = notifications_now
        print(f"Got signal in  {elapsed : .3f} {elapsed_notifications}", flush=True)

main_event_loop.run_until_complete(main())
# main_event_loop.run_forever()
