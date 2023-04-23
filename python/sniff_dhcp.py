#!/usr/bin/env python3
#
#  sniff_dhcp.py
#
#  Copyright (C) 2022-2023 Zhang Maiyun <me@myzhangll.xyz>
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
import sys
import time
from dataclasses import dataclass
from typing import Dict, List, Optional, Tuple

import scapy
from scapy.all import DHCP, DNS, DNSRR, UDP, Ether, LLMNRResponse, getmacbyip, getmacbyip6

import manage_hosts

class MissingData(Exception):
    """Missing data."""

class NoAddressError(MissingData):
    """Raised when no address is found."""

class NoMACError(MissingData):
    """Raised when no MAC is found."""

@dataclass
class Host:
    """A host."""
    # Source of the information
    source: str
    ipv4: Optional[str] = None
    ipv6: Optional[str] = None
    # Any form of hostname
    hostname: Optional[str] = None
    # mDNS hostname
    dns_hostname: Optional[str] = None
    # MAC address
    mac: Optional[str] = None

    def to_items(self) -> List[Tuple[str, str, str, str]]:
        """Convert to a list of items."""
        messages = []
        if not self.ipv4 and not self.ipv6:
            raise NoAddressError(f"No IP address for {self}")
        if not self.mac:
            # Try to resolve MAC address again
            self.fill_mac_from_ip()
        if not self.mac:
            raise NoMACError(f"No MAC address for {self}")
        if self.hostname:
            hostnames = [self.hostname]
            if self.dns_hostname:
                hostnames.append(self.dns_hostname)
        elif self.dns_hostname:
            hostnames = [self.dns_hostname]
        else:
            hostnames = ["<unknown>"]
        if self.ipv4:
            for hostname in hostnames:
                messages.append((self.ipv4, self.mac, hostname, "Sniff-Source: " + self.source))
        if self.ipv6:
            for hostname in hostnames:
                messages.append((self.ipv6, self.mac, hostname, "Sniff-Source: " + self.source))
        return messages

    def to_mh_tsv(self) -> str:
        """Convert to manage_hosts TSV format."""
        return '\n'.join(f"{i[0]}\t{i[1]}\t{i[2]} # {i[3]}" for i in self.to_items())

    def ipv4_from_arpa(self, arpa_form: bytes) -> None:
        """Set IPv4 address from arpa form."""
        ip = arpa_form.removesuffix(b".in-addr.arpa.").decode("utf-8")
        # Convert from reverse pointer format to normal IP
        self.ipv4 = ".".join(reversed(ip.split(".")))

    def ipv6_from_arpa(self, arpa_form: bytes) -> None:
        """Set IPv6 address from arpa form."""
        ip = arpa_form.removesuffix(b".ip6.arpa.").decode("utf-8")
        # Convert from reverse pointer format to normal IP
        rev_segs = reversed(ip.split("."))
        words = ("".join(segs) for segs in zip(*[iter(rev_segs)] * 4, strict=True))
        self.ipv6 = ':'.join(words).lower()

    def fill_mac_from_ip(self) -> None:
        """Resolve MAC address from IP address."""
        mac = None
        if self.ipv4:
            mac = getmacbyip(self.ipv4)
        if not mac and self.ipv6:
            mac = getmacbyip6(self.ipv6)
        if mac:
            self.mac = mac


def digest_dhcp_packet(frame) -> Optional[Host]:
    """Process frames containing DHCP payloads."""
    mac = None
    hostname = None
    ip = None
    for opt in frame[DHCP].options:
        if opt[0] == "end":
            break
        k = opt[0]
        value = opt[1]
        if k == "message-type" and value in (1, 3):
            # DISCOVER and REQUEST
            mac = frame[Ether].src
        if k == "hostname":
            hostname = value.decode("utf-8")
        if k == "requested_addr":
            ip = value
    if all((ip, hostname, mac)):
        return Host(ipv4=ip, hostname=hostname, mac=mac, source="DHCP")
    return None


def digest_dnsrr_ptr_airdrop_like(dnsrr: DNSRR, arcount: int, arlist) -> Optional[Host]:
    """Extract information from PTR records like *._companion-link._tcp.local.
    
    Returns `None` if the PTR record is not used.
    """
    # These are the ones I have seen
    servnames = (b"_googlecast._tcp.local.", b"_companion-link._tcp.local.")
    # Things we want to extract
    data = Host(source="mDNS PTR and SRV")
    # DNS-hostname to IP (bytes to str)
    hosts4 = {}
    hosts6 = {}
    for servname in servnames:
        # _X._proto.local. N IN PTR <hostname>._X._proto.local.
        if dnsrr.rdata.endswith(servname) and dnsrr.rrname == servname:
            hostname_servname = dnsrr.rdata
            data.hostname = hostname_servname.removesuffix(b"." + servname).decode("utf-8")
            dns_hostname = b""
            # See if there is an additional section with the IP and the DNS hostname
            for i in range(arcount):
                ar = arlist[i]
                # A or AAAA: extract the relationship to `hosts`
                if ar.type == 1:
                    hosts4[ar.rrname] = ar.rdata
                elif ar.type == 28:
                    hosts6[ar.rrname] = ar.rdata
                # SRV: find the corresponding DNS hostname and save all
                elif ar.type == 33 and ar.rrname == hostname_servname:
                    dns_hostname = ar.target
            data.ipv4 = hosts4.get(dns_hostname)
            data.ipv6 = hosts6.get(dns_hostname)
            data.dns_hostname = dns_hostname.decode("utf-8").removesuffix(".local.")
            data.fill_mac_from_ip()
            return data
    return None


def digest_mdns_or_llmnr_packet(frame) -> Optional[Host]:
    """Process frames containing mDNS or LLMNR payloads."""
    udp = frame[UDP]
    if udp.sport not in (5353, 5355) and udp.dport not in (5353, 5355):
        # Not mDNS nor LLMNR
        return None
    if DNS in frame:
        answers = frame[DNS].an
        ars = frame[DNS].ar
        ancount = frame[DNS].ancount
        arcount = frame[DNS].arcount
    elif LLMNRResponse in frame:
        answers = frame[LLMNRResponse].an
        ars = frame[LLMNRResponse].ar
        ancount = frame[LLMNRResponse].ancount
        arcount = frame[LLMNRResponse].arcount
    else:
        return None
    for i in range(ancount):
        dnsrr = answers[i]
        # A records
        if dnsrr.type == 1:
            hostname = dnsrr.rrname.decode("utf-8").removesuffix(".local.")
            data = Host(ipv4=dnsrr.rdata, hostname=hostname, source="mDNS A")
            data.fill_mac_from_ip()
            return data
        # AAAA records
        elif dnsrr.type == 28:
            hostname = dnsrr.rrname.decode("utf-8").removesuffix(".local.")
            data = Host(ipv6=dnsrr.rdata, hostname=hostname, source="mDNS AAAA")
            data.fill_mac_from_ip()
            return data
        # PTR records
        elif dnsrr.type == 12:
            maybe_data = digest_dnsrr_ptr_airdrop_like(dnsrr, arcount, ars)
            if maybe_data:
                return maybe_data
            data = Host(source="mDNS PTR")
            # Then we only care about arpa. types
            if dnsrr.rrname.endswith(b".in-addr.arpa."):
                data.ipv4_from_arpa(dnsrr.rrname)
                data.fill_mac_from_ip()
                data.hostname = dnsrr.rdata.removesuffix(
                    b".local.").decode("utf-8")
                return data
            elif dnsrr.rrname.endswith(b".ip6.arpa."):
                data.ipv6_from_arpa(dnsrr.rrname)
                data.fill_mac_from_ip()
                data.hostname = dnsrr.rdata.removesuffix(
                    b".local.").decode("utf-8")
                return data
    return None


# Queue for lines to be printed
results: queue.SimpleQueue[Host] = queue.SimpleQueue()

def digest_packet(frame) -> None:
    """Process captured frames."""
    if DHCP in frame and (host := digest_dhcp_packet(frame)):
        results.put(host)
    if (DNS in frame or LLMNRResponse in frame) and (host := digest_mdns_or_llmnr_packet(frame)):
        results.put(host)


sniffer = scapy.sendrecv.AsyncSniffer(
    prn=digest_packet,
    filter="udp port 67 or udp port 68 or udp port 5353 or udp port 5355",
    store=False)
sniffer.start()

db = manage_hosts.MacDatabase("stagingscan.json")

while True:
    # Exhaust the buffer
    try:
        host = results.get_nowait()
        try:
            # Save the host
            for line in host.to_items():
                # Filter out bogus
                if line[0] == "::" or line[0] == "0.0.0.0":
                    break
                if line[1] == "00:00:00:00:00:00":
                    break
                if line[1] == "ff:ff:ff:ff:ff:ff":
                    break
                if line[0].startswith("169.254."):
                    break
                # This one produces a lot of unwanted noise
                if line[0].startswith("fd"):
                    break
            else:
                # Not rejected, save
                # Print the line
                print(host.to_mh_tsv())
                for line in host.to_items():
                    db.add(line[0], line[2], line[1], (line[3],))
        except MissingData as err:
            print(err, file=sys.stderr)
    except queue.Empty:
        print("No new hosts")
        db.save()
        time.sleep(1)
