#!/usr/bin/env python3
#
#  manage_hosts.py
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


"""Manage MAC and IP databases."""

import json
from ipaddress import IPv4Address, IPv6Address, ip_address
from pathlib import Path
from typing import (TYPE_CHECKING, Any, Dict, Iterable, List, Set, Union, cast,
                    overload)

if TYPE_CHECKING:
    from os import PathLike

# Type for a host record
JsonRecordType = Dict[str, Union[List[str]]]
RecordType = Dict[str, Union[Set[Union[IPv4Address, IPv6Address, str]]]]


class JSONEncoder(json.JSONEncoder):
    """Encode JSON with IP and set support."""

    def default(self, o: Any) -> Union[List[Any], str, Any]:
        """JSON-serialize an object."""
        if isinstance(o, set):
            return list(o)
        if isinstance(o, IPv4Address):
            return "IP4/" + o.compressed
        if isinstance(o, IPv6Address):
            return "IP6/" + o.compressed
        return super().default(o)


class MacDatabase:
    """The database for storing MAC, IP, and hostname relationships.

    Data structure:
        - macs: Set[str]
          A list of media access control addresses associated with this host.
        - ips: Set[Union[IPv4Address, IPv6Address]]
          A list of IP addresses associated with this host.
        - hostnames: Set[str]
          A list of hostnames used on this host.
        - comments: Set[str]
          A list of comments.

    Serialization structure:
        `IPAddress`es are converted to `str`;
        `set`s are converted to `list`s.

    When adding new hosts, if the MAC and the hostname match, or if the
    hostname only differs by the -n suffix, they are considered to be the
    same host. Otherwise, entries can be merged with `merge()`.
    """

    def __init__(self, db_path: Union[str, "PathLike[str]"]):
        self._db_path = Path(db_path)
        if self._db_path.exists():
            self.open()
        else:
            self._db: List[RecordType] = []
        # Cache for reverse lookup
        self._mac_cache: Dict[str, List[int]] = {}

    def __len__(self) -> int:
        """Get the length of the database."""
        return len(self._db)

    @overload
    def __getitem__(self, index: int) -> RecordType: ...

    @overload
    def __getitem__(
        self,
        index: Union[slice, Iterable[Union[int, slice]]]
    ) -> List[RecordType]: ...

    def __getitem__(
            self,
            index: Union[int, slice, Iterable[Union[int, slice]]]
    ) -> Union[RecordType, List[RecordType]]:
        """Get items from the database by index, slice, or multiple indices."""
        if isinstance(index, (int, slice)):
            return self._db[index]
        # Unify slice and int to a 2-D array
        nes = ((self._db[i],) if isinstance(i, int)
               else self._db[i] for i in index)
        # Flatten the 2-D array
        return [itm for sli in nes for itm in sli]

    def __iter__(self) -> Iterable[RecordType]:
        return iter(self._db)

    @staticmethod
    def _jsonip_to_ip(
            jsonip: Union[str, IPv4Address, IPv6Address]
    ) -> Union[IPv4Address, IPv6Address]:
        """Convert a serialized IP back to `IPvxAddress`."""
        if isinstance(jsonip, (IPv4Address, IPv6Address)):
            return jsonip
        if jsonip[0:4] == "IP4/":
            return IPv4Address(jsonip[4:])
        if jsonip[0:4] == "IP6/":
            return IPv6Address(jsonip[4:])
        raise ValueError("Expecting an IP address with a IPx/ prefix")

    def _deserialize_and_check(
            self,
            record: Union[JsonRecordType, RecordType]) -> RecordType:
        """Deserialize a single host record and check for its integrity."""
        if {"ips", "macs", "hostnames", "comments"} != set(record):
            raise ValueError("Invalid record")
        new_record: RecordType = {}
        ips = cast(List[str], record["ips"])
        new_record["ips"] = {self._jsonip_to_ip(ip) for ip in ips}
        new_record["macs"] = set(record["macs"])
        new_record["hostnames"] = set(record["hostnames"])
        new_record["comments"] = set(record["comments"])
        return new_record

    def open(self) -> None:
        """Open and load the storage.

        Can also be used to reload the file.
        """
        with open(self._db_path, encoding="utf-8") as database:
            data = json.load(database)
        self._db = [self._deserialize_and_check(host) for host in data]

    def save(self) -> None:
        """Save the database to the storage file."""
        with open(self._db_path, "w", encoding="utf-8") as database:
            json.dump(self._db, database, cls=JSONEncoder)

    @staticmethod
    def _hostname_fuzz(one: str, two: str) -> bool:
        """See if two hostnames only differ in the part after the last -."""
        dash1 = one.rfind("-")
        dash2 = two.rfind("-")
        # Only accept numeric suffixes, otherwise it is a part of the hostname
        try:
            _ = int(one[dash1:])
            base1 = one[:dash1] if dash1 != -1 else one
        except ValueError:
            base1 = one
        try:
            _ = int(two[dash2:])
            base2 = two[:dash2] if dash2 != -1 else two
        except ValueError:
            base2 = two
        return base1 == base2

    def find_indices_by_hostname(
            self,
            hostname: str,
            fuzz: bool = True) -> List[int]:
        """Look up hosts with `hostname`.

        If `fuzz` is `True`, hosts that only the "-n" suffixes are different
        are treat as a match.
        """
        results: List[int] = []
        for index, entry in enumerate(self._db):
            if hostname in cast(List[str], entry["hostnames"]):
                results.append(index)
            elif fuzz:
                for entry_hn in entry["hostnames"]:
                    if self._hostname_fuzz(hostname, cast(str, entry_hn)):
                        results.append(index)
        return results

    def find_indices_by_mac(self, mac: str) -> List[int]:
        """Look up hosts with `mac`."""
        if mac in self._mac_cache:
            return self._mac_cache[mac]
        results: List[int] = []
        for index, entry in enumerate(self._db):
            if mac in cast(List[str], entry["macs"]):
                results.append(index)
        self._mac_cache[mac] = results
        return results

    def add(
        self,
        ipaddr: Union[IPv4Address, IPv6Address],
        hostname: str,
        mac: str,
        comments: Iterable[str]
    ) -> None:
        """Add a new entry, potentially merging it."""
        # Remove the mDNS .local suffix
        if hostname[-6:] == ".local":
            hostname = hostname[:-6]
        # Try to find an existing one to merge
        existing_records = self.find_indices_by_mac(mac)
        for index in existing_records:
            for this_hostname in self._db[index]["hostnames"]:
                if self._hostname_fuzz(hostname, cast(str, this_hostname)):
                    break
            else:
                # Not found/matched
                continue
            # Matched
            self._db[index]["ips"].add(ipaddr)
            self._db[index]["hostnames"].add(hostname)
            self._db[index]["macs"].add(mac)
            self._db[index]["comments"].update(comments)
            # We assume previous records are all unique and correct
            break
        else:
            # No existing
            self._db.append({
                "ips": {ipaddr},
                "hostnames": {hostname},
                "macs": {mac},
                "comments": set(comments)
            })
        # Invalidate cache
        if mac in self._mac_cache:
            del self._mac_cache[mac]
        _ = [self._deserialize_and_check(entry) for entry in self._db]

    def merge(self, entries: Iterable[int]) -> None:
        """Merge the entries with the given indices."""
        merged_entry: RecordType = {
            "ips": set(),
            "hostnames": set(),
            "macs": set(),
            "comments": set()
        }
        # Start from the highest-index to avoid recalculating index
        for index in sorted(entries, reverse=True):
            for key in ("ips", "hostnames", "macs", "comments"):
                merged_entry[key].update(self._db[index][key])
            del self._db[index]
        self._db.append(merged_entry)
        # Invalidate cache
        self._mac_cache = {}

    def add_from_tsv_line(self, tsv_line: str) -> None:
        """Parse an old-style TSV line and add it."""
        fields = tsv_line.strip().split("\t")
        ipaddr = ip_address(fields[0])
        hostname = fields[1]
        comment_pos = fields[-1].find("#")
        comment = []
        if comment_pos != -1:
            comment = [fields[-1][comment_pos+1:].strip()]
            fields[-1] = fields[-1][:comment_pos-1].strip()
        for mac in fields[2:]:
            self.add(ipaddr, hostname, mac, comment)

    def get_db(self) -> List[RecordType]:
        """Get the underlying database."""
        return self._db
