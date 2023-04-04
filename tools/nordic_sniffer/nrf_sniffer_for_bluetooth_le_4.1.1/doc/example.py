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

import time
from SnifferAPI import Sniffer, UART

nPackets = 0
mySniffer = None

def setup():
    global mySniffer
    
    # Find connected sniffers
    ports = UART.find_sniffer()
    
    if len(ports) > 0:
        # Initialize the sniffer on the first COM port found with baudrate 1000000.
        # If you are using an old firmware version <= 2.0.0, simply remove the baudrate parameter here.
        mySniffer = Sniffer.Sniffer(portnum=ports[0], baudrate=1000000)
    
    else:
        print("No sniffers found!")
        return
    
    # Start the sniffer module. This call is mandatory.
    mySniffer.start()
    # Scan for new advertisers
    mySniffer.scan()

    # Wait to allow the sniffer to discover device mySniffer.
    time.sleep(5)
    # Retrieve list of discovered devicemySniffer.
    d = mySniffer.getDevices()
    # Find device with name "Example".
    dev = d.find('Example')
    
    if dev is not None:
        # Follow (sniff) device "Example". This call sends a REQ_FOLLOW command over UART.
        mySniffer.follow(dev)
    else:
        print("Could not find device")

def loop():
    # Enter main loop
    nLoops = 0
    while True:
        time.sleep(0.1)
        # Get (pop) unprocessed BLE packets.
        packets = mySniffer.getPackets()
        
        processPackets(packets) # function defined below
        
        nLoops += 1
        
        # print diagnostics every so often
        if nLoops % 20 == 0:
            print(mySniffer.getDevices())
            print("inConnection", mySniffer.inConnection)
            print("currentConnectRequest", mySniffer.currentConnectRequest)
            print("packetsInLastConnection", mySniffer.packetsInLastConnection)
            print("nPackets", nPackets)
            print()
        
# Takes list of packets
def processPackets(packets):
    for packet in packets:
        # packet is of type Packet
        # packet.blePacket is of type BlePacket
        global nPackets
        # if packet.OK:
        # Counts number of packets which are not malformed.
        nPackets += 1
    
setup()
if mySniffer is not None:
    loop()
