#!python

# A python program to retrieve data from the BLE stepper motor analyzer
# probe and displaying it in realtime on the screen.

import argparse
import asyncio
import logging
import platform
import signal
import sys
import time
import pyqtgraph
from numpy import histogram
from pyqtgraph.Qt import QtWidgets
from collections import deque

# A workaround to avoid auto formatting.
if True:
    sys.path.append("..")
    from common.capture_signal import CaptureSignal
    from common.capture_signal_fetcher import CaptureSignalFetcher
    from common.chart import Chart
    from common.current_histogram import CurrentHistogram
    from common.distance_histogram import DistanceHistogram
    from common.filter import Filter
    from common.probe import Probe
    from common.probe_state import ProbeState
    from common.time_histogram import TimeHistogram
    from common import connections
    from common import ble_util

# NOTE: Color names list here https://matplotlib.org/stable/gallery/color/named_colors.html

# Default device address. Use the flag below to override it.
# To find device addresses, run scanner_main.py and look for
# devices whose name looks like STP-XXXXXXXXXXXX.
# The device address has different format on Windows and
# on Mac OSX.

# Allows to stop the program by typing ctrl-c.
signal.signal(signal.SIGINT, lambda number, frame: sys.exit())

# Print environment info for debugging.
print(f"OS: {platform.platform()}", flush=True)
print(f"Platform:: {platform.uname()}", flush=True)
print(f"Python {sys.version}", flush=True)

# Command line flags.
parser = argparse.ArgumentParser()

parser.add_argument('--scan',
                    dest="scan",
                    default=False,
                    action=argparse.BooleanOptionalAction,
                    help="If specified, scan for devices and exit.")

parser.add_argument("--device",
                    dest="device",
                    default=None,
                    help="Optional, the name or nickname of the device to connect to.")

parser.add_argument("--set-nickname",
                    dest="set_nickname",
                    default=None,
                    help="If true, update the device to have the given nickname and exit.'")

parser.add_argument("--max-amps",
                    dest="max_amps",
                    type=float,
                    default=2.0,
                    help="Max current display.")
parser.add_argument("--units", dest="units", default="steps", help="Units of movements.")
parser.add_argument("--steps-per-unit",
                    dest="steps_per_unit",
                    type=float,
                    default=1.0,
                    help="Steps per unit")
# Per https://github.com/hbldh/bleak/issues/1223 client.disconnect() may be
# problematic on some systems, so we provide this heuristics as a workaround,
parser.add_argument("--cleanup-forcing",
                    dest="cleanup_forcing",
                    default=None,
                    action=argparse.BooleanOptionalAction,
                    help="Specifies if to explicitly close the connection on program exit.")
args = parser.parse_args()

# ProbeStates from the device which are waiting to be processed.
# Keeping up to a large enough number to cover the display in case of
# a very long pause.
pending_states = deque(maxlen=600)

# Used to slightly smooth the abs current signal.
amps_abs_filter = Filter(0.5)

# Indicates if there is a pending request to reset the data
pending_reset = False

# Indicate if the user asked to pause the updates.
pause_enabled = False

# Indicates a pending request to toggle direction.
pending_direction_toggle = False

# After reset we ignore a few state notification to clear up
# the notification pipeline.
states_to_drop = 0

# Capture divider values to toggle through. Modify as desired..
capture_dividers = [1, 2, 5, 10, 20]
# The last divider that was set in the device.
last_set_capture_divider = None
# The current capture divider, with an arbitrary default.
capture_divider = capture_dividers[2]

# We use a single event loop for all asyncio operatios.
main_event_loop = asyncio.new_event_loop()

# Set latter when we connect to the device.
probe = None


async def do_nothing():
    """ A dummy async method. """
    None


# A special case where the user asked to just scan and exit.
if args.scan:
    asyncio.run(connections.scan_and_dump())
    sys.exit("\nScanning done.")

# Connect to the probe.
logging.basicConfig(level=logging.INFO)
probe = main_event_loop.run_until_complete(connections.connect_to_probe(args.device))
if not probe:
    # NOTE: Error message already been printed by connect_to_probe().
    sys.exit()

if args.set_nickname is not None :
    # NOTE: Writing an empty nickname is equivalent to deleting it.
    if not ble_util.is_valid_nickname(args.set_nickname, empty_ok=True):
        sys.exit(f"Invalid --set-nickname value: [{args.set_nickname}].", flush=True)
    main_event_loop.run_until_complete(probe.write_command_set_nickname(args.set_nickname))
    # Keep the event loop busy for a little bit, to let the command complete.
    # TODO: Is there a cleaner way?  See https://github.com/hbldh/bleak/discussions/1274
    for i in range(1000):
        main_event_loop.run_until_complete(do_nothing())
        time.sleep(0.001)
    sys.exit(f"\nDevice updated with nickname [{args.set_nickname}].")

# An object that tracks the incremental fetch of the capture
# signal. We don't perform all of them at once to avoid choppy
# state chart updates.
capture_signal_fetcher = CaptureSignalFetcher(probe)

# Here we are connected successfully to the BLE device. Start the GUI.

# Desired window size.
win_width = 1100
win_height = 700

# We set the actual size later. This is a workaround to force an
# early compaction of the buttons row.
win = pyqtgraph.GraphicsLayoutWidget(show=True, size=[win_width, win_height])
# title = f"BLE Stepper Motor Analyzer [{device_address}]"
title = f"BLE Stepper Motor Analyzer [{probe.name()}]"
# if args.device_nickname:
if probe.nickname():
    title += f" [{probe.nickname()}]"
win.setWindowTitle(title)

# Layout class doc: https://doc.qt.io/qt-5/qgraphicsgridlayout.html

win.ci.layout.setColumnPreferredWidth(0, 240)
win.ci.layout.setColumnPreferredWidth(1, 240)
win.ci.layout.setColumnPreferredWidth(2, 240)
win.ci.layout.setColumnPreferredWidth(3, 240)
win.ci.layout.setColumnPreferredWidth(4, 380)

win.ci.layout.setColumnStretchFactor(0, 1)
win.ci.layout.setColumnStretchFactor(1, 1)
win.ci.layout.setColumnStretchFactor(2, 1)
win.ci.layout.setColumnStretchFactor(3, 1)
win.ci.layout.setColumnStretchFactor(4, 1)

# Graph 1 - Distance Chart.
plot: pyqtgraph.PlotItem = win.addPlot(name="Plot1", colspan=5)
plot.setLabel('left', 'Distance', args.units)
plot.setXRange(-10, 0)
plot.showGrid(False, True, 0.7)
plot.setAutoPan(x=True)
graph1 = Chart(plot, pyqtgraph.mkPen('yellow'))

# Graph 2 - Speed Chart.
win.nextRow()
plot = win.addPlot(name="Plot2", colspan=5)
plot.setLabel('left', 'Speed', f"{args.units}/s")
plot.setXRange(-10, 0)
plot.showGrid(False, True, 0.7)
plot.setAutoPan(x=True)
plot.setXLink('Plot1')  # synchronize time axis
graph2 = Chart(plot, pyqtgraph.mkPen('orange'))

# Graph 3 - Current Chart.
win.nextRow()
plot = win.addPlot(name="Plot3", colspan=5)
plot.setLabel('left', 'Current', 'A')
plot.setXRange(-10, 0)
plot.setYRange(0, 2)
plot.showGrid(False, True, 0.7)
plot.setAutoPan(x=True)
plot.setXLink('Plot1')  # synchronize time axis
graph3 = Chart(plot, pyqtgraph.mkPen('green'))

# NOTE: See graph configuration params here
# https://pyqtgraph.readthedocs.io/en/latest/api_reference/graphicsItems/viewbox.html

# Graph 4 - Current Histogram.
win.nextRow()
plot4 = win.addPlot(name="Plot4")
plot4.setLabel('left', 'Current', 'A')
plot4.setLabel('bottom', 'Speed', f'{args.units}/s')
plot4.setLimits(yMin=0)
plot4.setYRange(0, args.max_amps)
graph4 = pyqtgraph.BarGraphItem(x=[0], height=[0], width=0.3, brush='yellow')
plot4.addItem(graph4)

# Graph 5 - Time Histogram.
plot5 = win.addPlot(name="Plot5")
plot5.setLabel('left', 'Time', '%')
plot5.setLabel('bottom', 'Speed', f"{args.units}/s")
plot5.setLimits(yMin=0, yMax=100)
graph5 = pyqtgraph.BarGraphItem(x=[0], height=[0], width=0.3, brush='salmon')
plot5.addItem(graph5)

# Graph 6 - Distance Histogram.
plot6 = win.addPlot(name="Plot6")
plot6.setLabel('left', 'Distance', '%')
plot6.setLabel('bottom', 'Speed', f"{args.units}/s")
plot6.setLimits(yMin=0, yMax=100)
graph6 = pyqtgraph.BarGraphItem(x=[0], height=[0], width=0.3, brush='skyblue')
plot6.addItem(graph6)

# Graph 7 - Phase Diagram.
plot7 = win.addPlot(name="Plot8")
plot7.setLabel('left', 'Coil B', 'A')
plot7.setLabel('bottom', 'Coil A', 'A')
plot7.showGrid(True, True, 0.7)
plot7.setXRange(-args.max_amps, args.max_amps)
plot7.setYRange(-args.max_amps, args.max_amps)

# Graph 8 - Capture Signals.
plot8 = win.addPlot(name="Plot7")
plot8.setLabel('left', 'Current', 'A')
plot8.setLabel('bottom', 'Time', 's')
plot8.setYRange(-args.max_amps, args.max_amps)

# Add a row for the buttons
win.nextRow()
buttons_layout = win.addLayout(colspan=5)
buttons_layout.setSpacing(20)
buttons_layout.layout.setHorizontalSpacing(30)

# Button1 - Toggle Direction.
button1_proxy = QtWidgets.QGraphicsProxyWidget()
button1 = QtWidgets.QPushButton('Toggle dir.')
button1_proxy.setWidget(button1)
buttons_layout.addItem(button1_proxy, row=0, col=0)

# Button2 - Reset Data
button2_proxy = QtWidgets.QGraphicsProxyWidget()
button2 = QtWidgets.QPushButton('Reset Data')
button2_proxy.setWidget(button2)
buttons_layout.addItem(button2_proxy, row=0, col=1)

# Button3 - Time Scale (toggles capture divider)
button3_proxy = QtWidgets.QGraphicsProxyWidget()
button3 = QtWidgets.QPushButton(f'Time Scale X{capture_divider}')
button3_proxy.setWidget(button3)
buttons_layout.addItem(button3_proxy, row=0, col=2)

# Button4 - Pause.
button4_proxy = QtWidgets.QGraphicsProxyWidget()
button4 = QtWidgets.QPushButton('Pause')
button4_proxy.setWidget(button4)
buttons_layout.addItem(button4_proxy, row=0, col=3)

# We cache the last reported state so we can compute speed.
last_state = None


def add_state(state: ProbeState):
    """ Called when a new state notification is received from the device. """
    global probe, graph1, graph2, last_state, updates_counter

    # Compute speed based on change from previous state.
    if last_state is None:
        # print(f"No last state", flush=True)
        speed = 0
    else:
        delta_t = state.timestamp_secs - last_state.timestamp_secs
        # We don't expect repeating timestamp. Avoid a divide by zero.
        if delta_t <= 0:
            print(f"Duplicate notifcation TS {delta_t}", flush=True)
            return
        # Normal intervals are 0.020 sec. If it's larger, we missed
        # notification packets.
        if delta_t > 0.025:
            print(f"Data loss: {delta_t*1000:3.0f} ms", flush=True)
        speed = (state.steps - last_state.steps) / delta_t

    amps_abs_filter.add(state.amps_abs)

    # Update distance chart.
    graph1.add_point(state.timestamp_secs, state.steps / args.steps_per_unit)
    # Update speed chart.
    graph2.add_point(state.timestamp_secs, speed / args.steps_per_unit)
    # Update current chart.
    graph3.add_point(state.timestamp_secs, amps_abs_filter.value())

    last_state = state


def on_reset_button():
    """ Called when Reset button is clicked. """
    global pending_reset
    pending_reset = True


def on_pause_button():
    """ Called when Pause button is clicked. """
    global pause_enabled, button4
    if pause_enabled:
        button4.setText("Pause")
        pause_enabled = False
    else:
        button4.setText("Continue")
        pause_enabled = True


def on_scale_button():
    """ Called when Scale button is clicked. """
    global capture_divider, last_set_capture_divider
    n = len(capture_dividers)
    next_divider_index = 0
    for i in range(n):
        if capture_dividers[i] == capture_divider:
            next_divider_index = (i + 1) % n
    capture_divider = capture_dividers[next_divider_index]
    button3.setText(f"Time Scale X{capture_divider}")


def on_direction_button():
    """ Called when Direction button is clicked. """
    global pending_direction_toggle
    pending_direction_toggle = True


# In addition to update the charts with the state reports, we
# also need to update the other charts in the background. We spread
# this work evenly using 'task slots'.

task_index = 0
task_start_time = time.time()

task_read_current_histogram = None
task_read_time_histogram = None
task_read_distance_histogram = None
task_read_capture_signal = None


async def timer_handler_tasks(task_index: int) -> bool:
    """ Execute next task slice, return True if completed."""
    global probe, pause_enabled, task_read_current_histogram, task_read_time_histogram, task_read_distance_histogram, task_read_capture_signal

    # Send conn WDT heartbeat, keeping the connection for additional
    # 5 secs.
    if task_index == 0:
        await probe.write_command_conn_wdt(5)
        return True

    # All tasks below are performed only when not paused.
    if pause_enabled:
        return True

    # Read current histogram
    if task_index == 1:
        if not task_read_current_histogram:
            # print("CURRENT: Creating task", flush=True)
            task_read_current_histogram = asyncio.create_task(
                probe.read_current_histogram(args.steps_per_unit))
            return False
        if not task_read_current_histogram.done():
            # print("CURRENT: Task not ready", flush=True)
            return False
        # print("CURRENT: Task ready", flush=True)
        histogram: CurrentHistogram = task_read_current_histogram.result()
        task_read_current_histogram = None
        graph4.setOpts(x=histogram.centers(),
                       height=histogram.heights(),
                       width=0.75 * histogram.bucket_width())
        return True

    # Read time histogram
    if task_index == 2:
        if not task_read_time_histogram:
            # print("TIME: Creating task", flush=True)
            task_read_time_histogram = asyncio.create_task(
                probe.read_time_histogram(args.steps_per_unit))
            return False
        if not task_read_time_histogram.done():
            # print("TIME: Task not ready", flush=True)
            return False
        # print("TIME: Task ready", flush=True)
        histogram: TimeHistogram = task_read_time_histogram.result()
        task_read_time_histogram = None
        graph5.setOpts(x=histogram.centers(),
                       height=histogram.heights(),
                       width=0.75 * histogram.bucket_width())
        return True

    # Read distance histogram.
    if task_index == 3:
        if not task_read_distance_histogram:
            # print("DISTANCE: Creating task", flush=True)
            task_read_distance_histogram = asyncio.create_task(
                probe.read_distance_histogram(args.steps_per_unit))
            return False
        if not task_read_distance_histogram.done():
            # print("DISTANCE: Task not ready", flush=True)
            return False
        # print("DISTANCE: Task ready", flush=True)
        histogram: DistanceHistogram = task_read_distance_histogram.result()
        task_read_distance_histogram = None
        graph6.setOpts(x=histogram.centers(),
                       height=histogram.heights(),
                       width=0.75 * histogram.bucket_width())
        return True

    # Next chunk of fetching the capture signal.
    if task_index == 4:
        if not task_read_capture_signal:
            # print("CAPTURE: Creating initial task", flush=True)
            capture_signal_fetcher.reset()
            task_read_capture_signal = asyncio.create_task(capture_signal_fetcher.loop())
            return False
        if not task_read_capture_signal.done():
            # print("DISTANCE: Task not ready", flush=True)
            return False
        capture_signal: CaptureSignal = task_read_capture_signal.result()
        if not capture_signal:
            # print("CAPTURE: Creating additional task", flush=True)
            task_read_capture_signal = asyncio.create_task(capture_signal_fetcher.loop())
            return False
        task_read_capture_signal = None
        # print("CAPTURE: Data ready", flush=True)
        # A new capture available, update phase and signal plots.
        plot8.clear()
        plot8.plot(capture_signal.times_sec(), capture_signal.amps_a(), pen='yellow')
        plot8.plot(capture_signal.times_sec(), capture_signal.amps_b(), pen='skyblue')
        plot7.clear()
        plot7.plot(capture_signal.amps_a(), capture_signal.amps_b(), pen='greenyellow')
        return True


def timer_handler():
    """ Called periodically by the PyGraphQt library. """
    global probe, task_index, task_start_time, graph1, graph2, graph3, graph4, graph5, graph6, plot8
    global capture_signal_fetcher, pending_reset, pause_enabled
    global buttons_layout, last_state, states_to_drop
    global capture_divider, last_set_capture_divider, pending_direction_toggle
    global main_event_loop, do_nothing

    # Process any pending events from background notifications.
    main_event_loop.run_until_complete(do_nothing())

    if pending_reset:
        print(f"Reset", flush=True)
        main_event_loop.run_until_complete(probe.write_command_reset_data())

        # Drop next three states to clear the notification pipeline.
        pending_states.clear()
        last_state = None
        states_to_drop = 3

        graph1.clear()
        graph2.clear()
        graph3.clear()

        graph4.setOpts(x=[0], height=[0])
        graph5.setOpts(x=[0], height=[0])
        graph6.setOpts(x=[0], height=[0])

        plot8.clear()
        capture_signal_fetcher.reset()
        pending_reset = False

    if pending_direction_toggle:
        print(f"Changing direction", flush=True)
        main_event_loop.run_until_complete(probe.write_command_toggle_direction())
        pending_direction_toggle = False

    if capture_divider != last_set_capture_divider:
        main_event_loop.run_until_complete(probe.write_command_set_capture_divider(capture_divider))
        last_set_capture_divider = capture_divider
        print(f"Capture divider set to {last_set_capture_divider}", flush=True)

    # Process pending states, if any. We process up to 25 states at a time
    # to avoid event loop starvation after long pauses.
    if not pause_enabled:
        n = 0
        while pending_states and n < 25:
            state = pending_states.popleft()
            add_state(state)
            n += 1

    # Slow down by forcing minimal task slot time (in secs).
    time_now = time.time()
    if time_now - task_start_time < 0.100:
        return

    # Time for next task slot.
    task_start_time = time_now
    task_done = main_event_loop.run_until_complete(timer_handler_tasks(task_index))
    if task_done:
        task_index += 1
        # Currently we have tasks [0, 4]
        if task_index > 4:
            task_index = 0


# Attached handlers to buttons.
button1.clicked.connect(lambda: on_direction_button())
button2.clicked.connect(lambda: on_reset_button())
button3.clicked.connect(lambda: on_scale_button())
button4.clicked.connect(lambda: on_pause_button())

# Number of state updates so far.
updates_counter = 0


# Receives the state updates from the device.
def state_notification_callback_handler(probe_state: ProbeState):
    global pending_states, states_to_drop, updates_counter
    # add_state(probe_state)
    if states_to_drop > 0:
        states_to_drop -= 1
        # print(f"Dropped a state, left to drop: {states_to_drop}", flush=True)
        return

    # Dump the state periodically.
    if updates_counter % 100 == 0:
        print(f"{updates_counter:06d}: {probe_state}", flush=True)
    updates_counter += 1

    # print(f"PUSH {probe_state.timestamp_secs}", flush=True)
    pending_states.append(probe_state)


# NOTE: The notification system keeps a reference to the  event
# loop which is main_event_loop and uses it to post events.
# Running the event loop periodically in the timer handler
# below keeps these events being services.
main_event_loop.run_until_complete(
    probe.set_state_notifications(state_notification_callback_handler))

timer = pyqtgraph.QtCore.QTimer()
timer.timeout.connect(timer_handler)
# Delay between timer calls, in milliseconds. We try to
# keep the event loop running as tight as possible.
timer.start(1)

if __name__ == '__main__':
    pyqtgraph.exec()
