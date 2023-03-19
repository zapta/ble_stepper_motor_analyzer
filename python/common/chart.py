# Abstracts a multi chunk horizontal scrolling chart based
# on the pyqtgraph's example.

import numpy as np
import pyqtgraph as pg

# import asyncio

# Number of data points per chunk.
CHUNK_SIZE = 100

# Max number of chunks. To display N chunks, set to N+1
# since the oldest and newest chunk contribute only
# CHUNK_SIZE display points to keep the overall number
# of display point stable.
MAX_CHUNKS = 10 + 1


class Chart:

    def __init__(self, plot: pg.PlotItem, pen):
        global CHUNK_SIZE, MAX_CHUNKS
        self.plot = plot
        self.pen = pen
        self.start_time = None
        self.chunks_curves = []
        self.chunks_data = []
        self.points_counter = 0

    def add_point(self, t_secs: float, y: float):
        global CHUNK_SIZE, MAX_CHUNKS

        # Remember starting time
        if self.start_time is None:
            self.start_time = t_secs

        # Time diff in secs from previous point.
        delta_t = t_secs - self.start_time

        # Shift all chunks left in the time domain.
        for c in self.chunks_curves:
            c.setPos(-delta_t, 0)

        # Location in current chunk.
        i = self.points_counter % CHUNK_SIZE

        if i == 0:
            # Append a new chunk.
            current_chunk_curve = self.plot.plot()
            current_chunk_curve.setPen(self.pen)
            self.chunks_curves.append(current_chunk_curve)
            current_chunk_data = np.empty((CHUNK_SIZE + 1, 2))
            self.chunks_data.append(current_chunk_data)

            # Trim excess chunk beyond maxChunks.
            while len(self.chunks_curves) > MAX_CHUNKS:
                c = self.chunks_curves.pop(0)
                self.plot.removeItem(c)
                self.chunks_data.pop(0)

        else:
            # Here when adding to an existing (last) chunk.
            current_chunk_curve = self.chunks_curves[-1]
            current_chunk_data = self.chunks_data[-1]

        # If all chunks exists, delete a point from the
        # oldest chunk to compensate for the point we are adding
        # below. This will keep the total number of points constant.
        if len(self.chunks_curves) == MAX_CHUNKS:
            #print(f"Trimming last chunk", flush=True)
            oldest_chunk_curve = self.chunks_curves[0]
            oldest_chunk_data = self.chunks_data[0]
            oldest_chunk_curve.setData(x=oldest_chunk_data[i + 1:, 0],
                                       y=oldest_chunk_data[i + 1:, 1])

        # Set x, y values of the new point.
        current_chunk_data[i + 1, 0] = t_secs - self.start_time
        current_chunk_data[i + 1, 1] = y

        # If a new chunk, set the starting point for continuity
        # with previous chunk.
        if i == 0:
            if len(self.chunks_data) > 1:
                prev_chunk_data = self.chunks_data[-2]
                prev_point = prev_chunk_data[-1]
                current_chunk_data[0] = prev_point
            else:
                current_chunk_data[0] = current_chunk_data[1]

        # Update the current chunk curve with current chunk data.
        # All other curve stay the same.
        current_chunk_curve.setData(x=current_chunk_data[:i + 2, 0],
                                    y=current_chunk_data[:i + 2, 1])
        self.points_counter += 1

    def clear(self):
        for c in self.chunks_curves:
            self.plot.removeItem(c)
        self.chunks_curves = []
        self.chunks_data = []
        self.start_time = None
        self.points_counter = 0
