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
from collections import defaultdict
from dataclasses import dataclass, field
from ipaddress import IPv4Address, IPv6Address, ip_address, ip_network
from typing import Optional, TypeVar

import scapy
from scapy.all import (DHCP, DNS, DNSRR, UDP, Ether, LLMNRResponse, getmacbyip,
                       getmacbyip6)

import manage_hosts

HostSelf = TypeVar("HostSelf", bound="Host")


class NoMACError(Exception):
    """Raised when no MAC is found."""


@dataclass
class Host:
    """A host."""
    ip: set[IPv4Address | IPv6Address] = field(default_factory=set)
    # Any form of hostname
    hostname: Optional[str] = None
    # mDNS hostname (without .local.)
    dns_hostname: Optional[str] = None
    # MAC address
    mac: Optional[str] = None
    # Comments
    comments: set[str] = field(default_factory=set)

    def to_items(self: HostSelf) -> list[tuple[IPv4Address | IPv6Address, str, str, set[str]]]:
        """Convert to a list of items."""
        messages = []
        if self.hostname:
            hostnames = [self.hostname]
            if self.dns_hostname:
                hostnames.append(self.dns_hostname)
        elif self.dns_hostname:
            hostnames = [self.dns_hostname]
        else:
            hostnames = ["<unknown>"]
        if not self.mac:
            # Try to resolve MAC address again
            self.fill_mac_from_ip()
        if not self.mac and self.ip:
            raise NoMACError(f"No MAC address for {self}")
        for addr in self.ip:
            for hostname in hostnames:
                messages.append((addr, self.mac, hostname, self.comments))
        return messages

    def with_source(self: HostSelf, source: str) -> HostSelf:
        """Add source."""
        self.comments.add(f"Sniff-Source: {source}")
        return self

    def with_ip(self: HostSelf, addr: Optional[IPv4Address | IPv6Address]) -> HostSelf:
        """Add an IP address."""
        if addr:
            self.ip.add(addr)
        return self

    def fill_mac_from_ip(self: HostSelf) -> HostSelf:
        """Resolve MAC address from IP address."""
        for addr in self.ip:
            if self.mac:
                break
            if isinstance(addr, IPv4Address):
                self.mac = getmacbyip(addr.compressed)
            elif isinstance(addr, IPv6Address) and addr not in ip_network("fc00::/7"):
                self.mac = getmacbyip6(addr.compressed)
        return self


def arpa_to_ip(arpa_form: bytes) -> IPv4Address | IPv6Address:
    """Convert from arpa form to IP address."""
    if arpa_form.endswith(b".in-addr.arpa."):
        ip = arpa_form.removesuffix(b".in-addr.arpa.").decode("utf-8")
        # Convert from reverse pointer format to normal IP
        ip = '.'.join(reversed(ip.split(".")))
        return IPv4Address(ip)
    elif arpa_form.endswith(b".ip6.arpa."):
        ip = arpa_form.removesuffix(b".ip6.arpa.").decode("utf-8")
        # Convert from reverse pointer format to normal IP
        rev_segs = reversed(ip.split("."))
        words = ("".join(segs)
                 for segs in zip(*[iter(rev_segs)] * 4, strict=True))
        ip = ":".join(words)
        return IPv6Address(ip)
    else:
        raise ValueError(
            f"{arpa_form!r} does not appear to be a reverse DNS pointer")


def digest_dhcp_packet(frame) -> Host:
    """Process frames containing DHCP payloads."""
    result = Host().with_source("DHCP")
    for opt in frame[DHCP].options:
        if opt[0] == "end":
            break
        k = opt[0]
        value = opt[1]
        if k == "message-type" and value in (1, 3):
            # DISCOVER and REQUEST
            result.mac = frame[Ether].src
        if k == "hostname":
            result.hostname = value.decode("utf-8")
        if k == "requested_addr":
            result.with_ip(ip_address(value))
    return result


@dataclass
class ExtractedDNSAnswer:
    """Information extracted from a single mDNS/LLMNR answer."""
    # These are the ones I have seen
    AIRPLAY_LIKE_SERVS = (
        b"_airplay._tcp.local.",
        b"_companion-link._tcp.local.",
        b"_device-info._tcp.local.",
        b"_googlecast._tcp.local.",
        b"_rdlink._tcp.local.",
    )
    # DNS-hostname (with .local.) to IP (bytes to set of IPv4/IPv6 addresses)
    # From A/AAAA records
    dns_to_ip: defaultdict[bytes, set[IPv4Address | IPv6Address]] = field(
        default_factory=lambda: defaultdict(set))
    # (hostname, servname) to DNS-hostname (with .local.)
    # From SRV records
    host_to_dns: defaultdict[tuple[bytes, bytes],
                             bytes] = field(default_factory=lambda: defaultdict(bytes))
    # hostname to other information
    # From TXT records
    host_to_comments: defaultdict[bytes, set[str]] = field(
        default_factory=lambda: defaultdict(set))

    def populate_from_a_or_aaaa(self, rrname: bytes, rdata: str) -> None:
        """Populate from A or AAAA record:

        rrname: DNS hostname
        rdata: IP address
        """
        self.dns_to_ip[rrname].add(ip_address(rdata))

    def populate_from_srv(self, rrname: bytes, target: bytes) -> None:
        """Populate from SRV record:

        rrname: hostname.servname
        target: DNS hostname
        """
        for servname in self.AIRPLAY_LIKE_SERVS:
            if rrname.endswith(servname):
                hostname = rrname.removesuffix(b"." + servname)
                self.host_to_dns[(hostname, servname)] = target
                break
        else:
            print(f"Unknown SRV record: {rrname} -> {target}")

    def populate_from_ptr(self, rrname: bytes, rdata: bytes) -> None:
        """Populate from PTR record:

        1. rrname: servname; rdata: hostname.servname
        2. rrname: IP in reverse pointer format; rdata: DNS hostname
        """
        if rrname in self.AIRPLAY_LIKE_SERVS:
            # Case 1: Do nothing because they don't provide any useful information
            pass
        elif rrname.endswith(b".in-addr.arpa.") or rrname.endswith(b".ip6.arpa."):
            # Case 2
            self.dns_to_ip[rdata].add(arpa_to_ip(rrname))
        else:
            print(f"Unknown PTR record: {rrname} -> {rdata}")

    def populate_from_txt(self, rrname: bytes, rdata: list[bytes]) -> None:
        """Populate from TXT record:

        rrname: hostname.servname
        rdata: <some data>
        """
        for servname in self.AIRPLAY_LIKE_SERVS:
            if rrname.endswith(servname):
                hostname = rrname.removesuffix(b"." + servname)
                for data in rdata:
                    if data.startswith(b"model="):
                        model = data.split(b"=", 1)[1].decode("utf-8")
                        self.host_to_comments[hostname].add(f"Model: {model}")
                break

    def extract_hosts(self) -> list[Host]:
        """Extract hosts from the information and discard this object."""
        hosts: list[Host] = []
        # First exhaust host_to_dns and consume related information
        while self.host_to_dns:
            (hostname, servname), dnsname = self.host_to_dns.popitem()
            if dnsname in self.dns_to_ip:
                ip = self.dns_to_ip.pop(dnsname)
                comments = self.host_to_comments.pop(
                    hostname, set())
                host = Host(
                    comments=comments,
                    dns_hostname=dnsname.removesuffix(
                        b".local.").decode("utf-8"),
                    hostname=hostname.decode("utf-8"),
                    ip=ip
                ).with_source("mDNS PTR and SRV").fill_mac_from_ip()
                hosts.append(host)
        # Then exhaust dns_to_ip and consume related information
        while self.dns_to_ip:
            dnsname, ip = self.dns_to_ip.popitem()
            if not ip:
                continue
            host = Host(
                dns_hostname=dnsname.removesuffix(b".local.").decode("utf-8"),
                ip=ip
            ).fill_mac_from_ip()
            if isinstance(next(iter(ip)), IPv4Address):
                host.with_source("mDNS A")
            else:
                host.with_source("mDNS AAAA")
            hosts.append(host)
        # Outstanding host_to_comments are discarded
        return hosts


def digest_mdns_or_llmnr_packet(frame) -> list[Host]:
    """Process a single packet containing mDNS or LLMNR payloads."""
    udp = frame[UDP]
    if udp.sport not in (5353, 5355) and udp.dport not in (5353, 5355):
        # Not mDNS nor LLMNR
        return []
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
        return []
    data = ExtractedDNSAnswer()
    for i in range(ancount):
        dnsrr = answers[i]
        # A/AAAA records
        if dnsrr.type == 1 or dnsrr.type == 28:
            data.populate_from_a_or_aaaa(dnsrr.rrname, dnsrr.rdata)
        # PTR records
        elif dnsrr.type == 12:
            data.populate_from_ptr(dnsrr.rrname, dnsrr.rdata)
        # SRV records
        elif dnsrr.type == 33:
            data.populate_from_srv(dnsrr.rrname, dnsrr.target)
        # TXT records
        elif dnsrr.type == 16:
            data.populate_from_txt(dnsrr.rrname, dnsrr.rdata)
    for i in range(arcount):
        dnsrr = ars[i]
        # A/AAAA records
        if dnsrr.type == 1 or dnsrr.type == 28:
            data.populate_from_a_or_aaaa(dnsrr.rrname, dnsrr.rdata)
        # PTR records
        elif dnsrr.type == 12:
            data.populate_from_ptr(dnsrr.rrname, dnsrr.rdata)
        # SRV records
        elif dnsrr.type == 33:
            data.populate_from_srv(dnsrr.rrname, dnsrr.target)
        # TXT records
        elif dnsrr.type == 16:
            data.populate_from_txt(dnsrr.rrname, dnsrr.rdata)
    return data.extract_hosts()


# Queue for lines to be printed
results: queue.SimpleQueue[list[Host]] = queue.SimpleQueue()


def digest_packet(frame) -> None:
    """Process captured frames."""
    if DHCP in frame and (host := digest_dhcp_packet(frame)):
        results.put([host])
    if (DNS in frame or LLMNRResponse in frame) and (hosts := digest_mdns_or_llmnr_packet(frame)):
        results.put(hosts)


sniffer = scapy.sendrecv.AsyncSniffer(
    prn=digest_packet,
    filter="udp port 67 or udp port 68 or udp port 5353 or udp port 5355",
    store=False)
sniffer.start()

db = manage_hosts.MacDatabase("stagingscan.json")

while True:
    # Exhaust the buffer
    try:
        for host in results.get_nowait():
            print("Got packet")
            try:
                items = host.to_items()
                # Save the host
                for line in items:
                    # Filter out bogus
                    if line[0].is_unspecified:
                        break
                    if line[1] == "00:00:00:00:00:00":
                        break
                    if line[1] == "ff:ff:ff:ff:ff:ff":
                        break
                    if line[0] in ip_network("169.254.0.0/16"):
                        break
                else:
                    # Not rejected, save
                    # Print the line
                    print(
                        *(f"{i[0]}\t{i[1]}\t{i[2]} # {' '.join(i[3])}" for i in items), sep="\n")
                    for line in items:
                        db.add(line[0], line[2], line[1], line[3])
            except NoMACError as err:
                print(err, file=sys.stderr)
        db.save()
    except queue.Empty:
        time.sleep(1)
