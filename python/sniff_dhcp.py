#!/usr/bin/env python3
#
#  sniff_dhcp.py
#
#  Copyright (C) 2022 Zhang Maiyun <myzhang1029@hotmail.com>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.
#


"""Sniff hostname and MACs from DHCPREQUEST broadcasts."""

import queue
import time

import scapy
import scapy.all

lines: queue.SimpleQueue[str] = queue.SimpleQueue()


def digest_dhcp_packet(frame):
    """Process frames containing DHCP payloads."""
    dhcp = frame.getlayer(scapy.layers.dhcp.DHCP)
    mac = None
    hostname = None
    ip = None
    for opt in dhcp.options:
        if opt[0] == "end":
            break
        k = opt[0]
        value = opt[1]
        if k == "message-type" and value in (1, 3):
            # DISCOVER and REQUEST
            mac = frame.getlayer(scapy.layers.l2.Ether).src
        if k == "hostname":
            hostname = value.decode("utf-8")
        if k == "requested_addr":
            ip = value
    if all((ip, hostname, mac)):
        lines.put(f"{ip}\t{hostname}\t{mac}")


sniffer = scapy.sendrecv.AsyncSniffer(
    prn=digest_dhcp_packet,
    store=False, lfilter=lambda p: p.haslayer(scapy.layers.dhcp.DHCP))
sniffer.start()

while True:
    # Exhaust the buffer
    try:
        line = lines.get_nowait()
        print(line)
    except queue.Empty:
        break
    # Wait 1 minute
    time.sleep(60)
