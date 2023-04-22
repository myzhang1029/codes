#!/usr/bin/env python3
#
#  sniff_dhcp.py
#
#  Copyright (C) 2022 Zhang Maiyun <me@myzhangll.xyz>
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

import json
import queue
import time
from typing import Dict

import scapy
import scapy.all

# Queue for lines to be printed.
# Dict["ip"|"hostname"|"mac", value]
lines: queue.SimpleQueue[Dict[str, str]] = queue.SimpleQueue()


def digest_dhcp_packet(frame):
    """Process frames containing DHCP payloads."""
    dhcp = frame[scapy.layers.dhcp.DHCP]
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
            mac = frame[scapy.layers.l2.Ether].src
        if k == "hostname":
            hostname = value.decode("utf-8")
        if k == "requested_addr":
            ip = value
    if all((ip, hostname, mac)):
        lines.put({"ip": ip, "hostname": hostname,
                  "mac": mac, "source": "DHCP"})


def digest_mdns_or_llmnr_packet(frame):
    """Process frames containing mDNS or LLMNR payloads."""
    udp = frame[scapy.layers.inet.UDP]
    if udp.sport not in (5353, 5355) and udp.dport not in (5353, 5355):
        # Not mDNS nor LLMNR
        return
    if scapy.layers.dns.DNS in frame:
        answers = frame[scapy.layers.dns.DNS].an
        ancount = frame[scapy.layers.dns.DNS].ancount
    elif scapy.layers.llmnr.LLMNRResponse in frame:
        answers = frame[scapy.layers.llmnr.LLMNRResponse].an
        ancount = frame[scapy.layers.llmnr.LLMNRResponse].ancount
    else:
        return
    for i in range(ancount):
        dnsrr = answers[i]
        # Print A records
        if dnsrr.type == 1:
            hostname = dnsrr.rrname.decode("utf-8").removesuffix(".local.")
            ip = dnsrr.rdata
            lines.put({"ip": ip, "hostname": hostname, "source": "mDNS A"})
        # Print AAAA records
        elif dnsrr.type == 28:
            hostname = dnsrr.rrname.decode("utf-8").removesuffix(".local.")
            ip = dnsrr.rdata
            lines.put({"ip": ip, "hostname": hostname, "source": "mDNS AAAA"})
        # Print PTR records
        elif dnsrr.type == 12:
            strip_types = (b"_googlecast._tcp.local.",
                           b"_companion-link._tcp.local.")
            for strip_type in strip_types:
                if dnsrr.rdata.endswith(strip_type) and dnsrr.rrname == strip_type:
                    hostname = dnsrr.rdata.removesuffix(
                        b"." + strip_type).decode("utf-8")
                    lines.put({"hostname": hostname, "source": "mDNS PTR"})
                    break
            else:
                # Then we only care about arpa. types
                if dnsrr.rrname.endswith(b".in-addr.arpa."):
                    ip = dnsrr.rrname.removesuffix(
                        b".in-addr.arpa.").decode("utf-8")
                    # Convert from reverse pointer format to normal IP
                    ip = ".".join(reversed(ip.split(".")))
                elif dnsrr.rrname.endswith(b".ip6.arpa."):
                    ip = dnsrr.rrname.removesuffix(
                        b".ip6.arpa.").decode("utf-8")
                    # Convert from reverse pointer format to normal IP
                    rev_segs = reversed(ip.split("."))
                    words = ("".join(segs)
                             for segs in zip(*[iter(rev_segs)] * 4, strict=True))
                    ip = ':'.join(words).lower()
                else:
                    continue
                hostname = dnsrr.rdata.removesuffix(b".local.").decode("utf-8")
                lines.put(
                    {"ip": ip, "hostname": hostname, "source": "mDNS PTR"})


def digest_packet(frame):
    """Process captured frames."""
    if scapy.layers.dhcp.DHCP in frame:
        digest_dhcp_packet(frame)
    if scapy.layers.dns.DNS in frame or scapy.layers.llmnr.LLMNRResponse in frame:
        digest_mdns_or_llmnr_packet(frame)


sniffer = scapy.sendrecv.AsyncSniffer(
    prn=digest_packet,
    filter="udp port 67 or udp port 68 or udp port 5353 or udp port 5355",
    store=False)
sniffer.start()

while True:
    # Exhaust the buffer
    try:
        line = lines.get_nowait()
        print(json.dumps(line))
    except queue.Empty:
        pass
    time.sleep(0)
