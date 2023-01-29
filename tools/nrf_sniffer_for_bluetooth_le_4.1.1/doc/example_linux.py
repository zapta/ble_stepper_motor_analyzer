# Copyright (c) Nordic Semiconductor ASA
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form, except as embedded into a Nordic
#    Semiconductor ASA integrated circuit in a product or a software update for
#    such product, must reproduce the above copyright notice, this list of
#    conditions and the following disclaimer in the documentation and/or other
#    materials provided with the distribution.
#
# 3. Neither the name of Nordic Semiconductor ASA nor the names of its
#    contributors may be used to endorse or promote products derived from this
#    software without specific prior written permission.
#
# 4. This software, with or without modification, must only be used with a
#    Nordic Semiconductor ASA integrated circuit.
#
# 5. Any software provided in binary form under this license must not be reverse
#    engineered, decompiled, modified and/or disassembled.
#
# THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
# OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import os, sys, traceback
import time
import logging
from SnifferAPI import Sniffer, UART, Logger, Pcap

def setup():
    # Find connected sniffers
    ports = UART.find_sniffer()
    
    if len(ports) > 0:
        # Initialize the sniffer on the first port found with baudrate 1000000.
        # If you are using an old firmware version <= 2.0.0, simply remove the baudrate parameter here.
        sniffer = Sniffer.Sniffer(portnum=ports[0], baudrate=1000000)
    
    else:
        print("No sniffers found!")
        return

    # Start the sniffer module. This call is mandatory.
    sniffer.start()
    # Scan for new advertisers
    sniffer.scan()


    for _ in range(10):
        time.sleep(1)
        devlist = sniffer.getDevices()
        # Find device with name "Example".
        found_dev = devlist.find('Example')
        if found_dev:
            follow(sniffer, found_dev)
            break

def follow(sniffer, dev):
    pipeFilePath = os.path.join(Logger.DEFAULT_LOG_FILE_DIR, "nordic_ble.pipe")
    logging.info('###### Found dev %s. Start wireshark with -Y btle -k -i %s', dev, os.path.abspath(pipeFilePath))

    myPipe = PcapPipe()
    myPipe.open_and_init(pipeFilePath)
    sniffer.follow(dev)
    sniffer.subscribe("NEW_BLE_PACKET", myPipe.newBlePacket)

    try:
        loop(sniffer)
    except:
        pass

    myPipe.close()

def loop(sniffer):
    nLoops = 0
    nPackets = 0
    connected = False
    while True:
        time.sleep(0.1)

        packets   = sniffer.getPackets()
        nLoops   += 1
        nPackets += len(packets)

        if connected != sniffer.inConnection or nLoops % 20 == 0:
            connected = sniffer.inConnection
            logging.info("connected %s, nPackets %s", sniffer.inConnection, nPackets)

class PcapPipe(object):
    def open_and_init(self, pipeFilePath):
        try:
            os.mkfifo(pipeFilePath)
        except OSError:
            logging.warn("fifo already exists?")
            raise SystemExit(1)
        self._pipe = open(pipeFilePath, 'wb')
        self.write(Pcap.get_global_header())

    def write(self, message):
        if not self._pipe: return
        try:
            self._pipe.write(message)
            self._pipe.flush()
        except IOError:
            exc_type, exc_value, exc_tb = sys.exc_info()
            logger.error('Got exception trying to write to pipe: %s', exc_value)
            self.close()

    def close(self):
        logging.debug("closing pipe")
        if not self._pipe: return
        self._pipe.close()
        self._pipe = None

    def newBlePacket(self, notification):
        packet = notification.msg["packet"]
        pcap_packet = Pcap.create_packet(bytes([packet.boardId] + packet.getList()),
                                         packet.time)
        self.write(pcap_packet)

if __name__ == '__main__':
    logger = logging.getLogger()
    logger.addHandler(logging.StreamHandler())
    logger.setLevel(logging.INFO)
    setup()
