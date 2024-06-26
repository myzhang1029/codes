#!/usr/bin/env python3
#
#  manage_hosts.py
#
#  Copyright (C) 2022 Zhang Maiyun <me@maiyun.me>
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
import os
import stat
import tempfile
from ipaddress import IPv4Address, IPv6Address, ip_address
from pathlib import Path
from typing import TYPE_CHECKING, Any, Iterable, Union, cast, overload

if TYPE_CHECKING:
    from os import PathLike

# Type for a host record
JsonRecordType = dict[str, list[str]]
RecordType = dict[str, set[Union[IPv4Address, IPv6Address, str]]]


class JSONEncoder(json.JSONEncoder):
    """Encode JSON with IP and set support."""

    def __init__(self, sort: bool = False, **kwargs: Any):
        super().__init__(**kwargs)
        self.sort = sort

    def default(self, o: Any) -> Union[list[Any], str, Any]:
        """JSON-serialize an object."""
        if isinstance(o, set):
            if self.sort:
                try:
                    return sorted(o)
                except TypeError:
                    # Cannot sort IPv4 and IPv6 addresses
                    v4s = {x for x in o if isinstance(x, IPv4Address)}
                    v6s = o - v4s
                    return sorted(v4s) + sorted(v6s)
            else:
                return list(o)
        if isinstance(o, IPv4Address):
            return "IP4/" + o.compressed
        if isinstance(o, IPv6Address):
            return "IP6/" + o.compressed
        return super().default(o)


def jsonip_to_ip(
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


def hostname_fuzz(one: str, two: str) -> bool:
    """See if two hostnames only differ in the part after the last -.

    See also: RFC6762 Section 9
    (https://www.rfc-editor.org/rfc/rfc6762.html#page-31)
    """
    dash1 = one.rfind("-")
    # Only accept numeric suffixes, otherwise it is a part of the hostname
    if dash1 == -1:
        base1 = one
    else:
        try:
            _ = int(one[dash1:])
            base1 = one[:dash1]
        except ValueError:
            base1 = one
    dash2 = two.rfind("-")
    if dash2 == -1:
        base2 = two
    else:
        try:
            _ = int(two[dash2:])
            base2 = two[:dash2]
        except ValueError:
            base2 = two
    return base1.lower() == base2.lower()


def canonicalize_mac(mac: str) -> str:
    """Convert MAC addresses to colon-separated lowercase."""
    # Error message
    ERROR = f"'{mac}' does not appear to be a MAC address"
    # Keep alphanumeric characters
    alnums = ''.join(ch for ch in mac if ch.isalnum())
    # Sanity check
    if len(alnums) != 12:
        # Fall back to detecting the separator and filling in zeros
        seps = [x for x in mac if not x.isalnum()]
        nseps = len(set(seps))
        # If there are mixed separators, it is not possible to
        # reliably know the length of the segments
        if nseps != 1:
            raise ValueError(ERROR)
        # Detect the length of the segments
        nseg = len(seps) + 1
        seglen = 12 // nseg
        if seglen * nseg != 12:
            raise ValueError(ERROR)
        sep = seps[0]
        # Fill missing characters in each segment with zeros
        alnums = ''.join(seg.zfill(seglen) for seg in mac.split(sep))
        if len(alnums) != 12:
            raise ValueError(ERROR)
    try:
        _ = int(alnums, 16)
    except ValueError:
        raise ValueError(ERROR)
    return ''.join(
        ch.lower() + ("" if idx & 1 == 0 or idx == 11 else ":")
        for idx, ch in enumerate(alnums)
    )


class MacDatabase:
    """The database for storing MAC, IP, and hostname relationships.

    Data structure:
        - macs: set[str]
          A list of media access control addresses associated with this host.
        - ips: set[Union[IPv4Address, IPv6Address]]
          A list of IP addresses associated with this host.
        - hostnames: set[str]
          A list of hostnames used on this host.
        - comments: set[str]
          A list of comments.

    Serialization structure:
        `IPAddress`es are converted to `str`;
        `set`s are converted to `list`s.

    When adding new hosts, if the MAC and the hostname match, or if the
    hostname only differs by the -n suffix, they are considered to be the
    same host. Otherwise, entries can be merged with `merge()`.
    """

    __slots__ = ("_db", "_db_path", "_mac_cache")

    def __init__(self, db_path: Union[str, "PathLike[str]"]):
        self._db_path = Path(db_path)
        if self._db_path.exists():
            self.open()
        else:
            self._db: list[RecordType] = []
        # Cache for reverse lookup
        self._mac_cache: dict[str, tuple[int, ...]] = {}

    def __len__(self) -> int:
        """Get the length of the database."""
        return len(self._db)

    @overload
    def __getitem__(self, index: int) -> RecordType: ...

    @overload
    def __getitem__(
        self,
        index: Union[slice, str, Iterable[Union[int, slice, str]]]
    ) -> list[RecordType]: ...

    def __getitem__(
            self,
            index: Union[int, slice, str, Iterable[Union[int, slice, str]]]
    ) -> Union[RecordType, list[RecordType]]:
        """Get items from the database by index, slice, MAC, or hostnames."""
        if isinstance(index, (int, slice)):
            return self._db[index]
        # MAC or hostname
        # Find matching indices and return the corresponding items
        if isinstance(index, (str, IPv4Address, IPv6Address)):
            matches = self.find_indices_by_hostname(index, fuzz=False)
            try:
                matches += self.find_indices_by_mac(index)
            except ValueError:
                pass
            try:
                matches += self.find_indices_by_ipaddr(index)
            except ValueError:
                pass
            return self[matches]
        # Unify slice and int to a 2-D array
        nes = (
            # Is an int, should give only one match
            (self._db[i],) if isinstance(i, int) else
            # Is a slice, a MAC, a hostname, or an IP
            self[i] for i in index
        )
        # Flatten the 2-D array
        return [itm for sli in nes for itm in sli]

    def __delitem__(
        self,
        index: Union[int, slice, Iterable[Union[int, slice]]]
    ) -> None:
        """Delete items by index, slice, or multiple indices and slices."""
        if isinstance(index, (int, slice)):
            del self._db[index]
            return
        indices = []
        for i in index:
            if isinstance(i, int):
                indices.append(i)
            else:
                # isinstance(i, slice)
                indices += list(range(*i.indices(len(self._db))))
        indices_unique = sorted(set(indices), reverse=True)
        for i in indices_unique:
            del self._db[i]

    def __iter__(self) -> Iterable[RecordType]:
        return iter(self._db)

    @staticmethod
    def _deserialize_and_check(
            record: Union[JsonRecordType, RecordType]) -> RecordType:
        """Deserialize a single host record and check for its integrity."""
        if {"ips", "macs", "hostnames", "comments"} != set(record):
            raise ValueError("Invalid record")
        new_record: RecordType = {}
        ips = cast(list[str], record["ips"])
        new_record["ips"] = {jsonip_to_ip(ip) for ip in ips}
        new_record["macs"] = set(record["macs"])
        new_record["hostnames"] = set(record["hostnames"])
        new_record["comments"] = set(record["comments"])
        return new_record

    def open(self) -> None:
        """Open and load the storage.

        Can also be used to reload the file.
        (WARNING: the original in-memory content is discarded!)
        """
        with open(self._db_path, encoding="utf-8") as database:
            data = json.load(database)
        self._db = [self._deserialize_and_check(host) for host in data]
        self._mac_cache = {}

    def save(
            self,
            sort: bool = False,
            db_path: Union[None, str, "PathLike[str]"] = None) -> None:
        """Save the database to the storage file.

        If `sort` is `True`, sets are sorted before serialized. This operation
        makes saving slower but may help when comparing two databases.

        If `db_path` is not None, the current db_path is overriden and
        subsequent saves will also go to the new path.
        """
        if db_path:
            self._db_path = Path(db_path)
        # Make sure they are in the same filesystem
        dirname = os.path.dirname(self._db_path)
        # Atomic writes to prevent clearing the database
        fd, path = tempfile.mkstemp(dir=dirname, text=True)
        with os.fdopen(fd, "w", encoding="utf-8") as database:
            database.write(JSONEncoder(sort).encode(self._db))
        oldstat = os.stat(self._db_path)
        try:
            os.replace(path, self._db_path)
            os.chown(self._db_path, oldstat.st_uid, oldstat.st_gid)
            os.chmod(self._db_path, stat.S_IMODE(oldstat.st_mode))
        finally:
            # Remove the temporary file if it failed
            try:
                os.remove(path)
            except FileNotFoundError:
                pass

    def find_indices_by_hostname(
            self,
            hostname: str,
            fuzz: bool = True) -> tuple[int, ...]:
        """Look up hosts with `hostname`.

        If `fuzz` is `True`, hosts that only the "-n" suffixes are different
        are treat as a match.
        """
        results: list[int] = []
        for index, entry in enumerate(self._db):
            for entry_hn in cast(set[str], entry["hostnames"]):
                if hostname.lower() == entry_hn.lower() or (
                        fuzz and hostname_fuzz(hostname, entry_hn)):
                    results.append(index)
        return tuple(results)

    def find_indices_by_ipaddr(
        self,
        ipaddr: Union[IPv4Address, IPv6Address, str]
    ) -> tuple[int, ...]:
        """Look up hosts that have `ipaddr` in its IP addresses."""
        # Convert to `IPvXAddress` if we have a `str`
        if isinstance(ipaddr, str):
            ipaddr = ip_address(ipaddr)
        results: list[int] = []
        for index, entry in enumerate(self._db):
            if ipaddr in entry["ips"]:
                results.append(index)
        return tuple(results)

    def find_indices_by_mac(self, mac: str) -> tuple[int, ...]:
        """Look up hosts with `mac`."""
        mac = canonicalize_mac(mac)
        if mac not in self._mac_cache:
            self._mac_cache[mac] = tuple(n for n, e in enumerate(
                self._db) if mac in cast(set[str], e["macs"]))
        return self._mac_cache[mac]

    def add(
        self,
        ipaddr: Union[IPv4Address, IPv6Address, str],
        hostname: str,
        mac: str,
        comments: Iterable[str]
    ) -> None:
        """Add a new entry, potentially merging it."""
        # Convert to `IPvXAddress` if we have a `str`
        if isinstance(ipaddr, str):
            ipaddr = ip_address(ipaddr)
        # Remove the mDNS .local suffix
        if hostname[-6:] == ".local":
            hostname = hostname[:-6]
        # Try to find an existing one to merge
        existing_records = self.find_indices_by_mac(mac)
        for index in existing_records:
            for this_hostname in self._db[index]["hostnames"]:
                if hostname_fuzz(hostname, cast(str, this_hostname)):
                    break
            else:
                # Not found/matched
                continue
            # Matched
            self._db[index]["ips"].add(ipaddr)
            self._db[index]["hostnames"].add(hostname)
            self._db[index]["macs"].add(canonicalize_mac(mac))
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
            self.add(ipaddr, hostname, canonicalize_mac(mac), comment)

    def get_db(self) -> list[RecordType]:
        """Get the underlying database."""
        return self._db


# Unit tests

def test_json_encoder() -> None:
    # Note that `set` is not ordered but comparison works
    assert json.loads(JSONEncoder().encode([
        {IPv4Address("10.0.4.2"), IPv6Address("2001:db8::")},
        "test",
        {1, 2, 4}
    ])) == [
        ["IP4/10.0.4.2", "IP6/2001:db8::"],
        "test",
        [1, 2, 4]
    ]


def test_jsonip_to_ip() -> None:
    """Test for `jsonip_to_ip`."""
    assert jsonip_to_ip("IP4/10.0.4.2") == IPv4Address("10.0.4.2")
    assert jsonip_to_ip("IP4/0.0.0.0") == IPv4Address("0.0.0.0")
    assert jsonip_to_ip(IPv4Address("0.0.0.0")) == IPv4Address("0.0.0.0")
    assert jsonip_to_ip("IP6/::ffff:0.0.0.0") == IPv6Address("::ffff:0:0")
    assert jsonip_to_ip("IP6/2001:db8::") == IPv6Address("2001:db8::")
    assert jsonip_to_ip(IPv6Address("2001:db8::")) == IPv6Address("2001:db8::")
    try:
        jsonip_to_ip("0.0.0.0")
    except ValueError:
        pass
    else:
        assert True == False, "Should raise"


def test_hostname_fuzz() -> None:
    """Test for `hostname_fuzz`."""
    assert hostname_fuzz("Android", "Android-10")
    assert hostname_fuzz("Android-20", "ANDROID-10")
    assert not hostname_fuzz("Android-abc", "Andriod-10")


def test_canonicalize_mac() -> None:
    """Test for `canonicalize_mac`."""
    test_cases = [
        ("6F-A1-34-98-69-DF", "6f:a1:34:98:69:df"),
        ("cf-d8-a5-b4-ae-95", "cf:d8:a5:b4:ae:95"),
        ("ff:aa:86-69-40-53", "ff:aa:86:69:40:53"),
        ("8B:A4:88:F7:2D:BC", "8b:a4:88:f7:2d:bc"),
        ("3BF20D-a9dfc7", "3b:f2:0d:a9:df:c7"),
        ("3-F3-e7-e9-39-73", "03:f3:e7:e9:39:73"),
        ("F7A02DAD14E0", "f7:a0:2d:ad:14:e0"),
        ("7B58.2cbb.12f9", "7b:58:2c:bb:12:f9"),
        ("6:51:64:e:b2:8e", "06:51:64:0e:b2:8e"),
    ]
    for n, (question, correct) in enumerate(test_cases):
        assert canonicalize_mac(question) == correct, \
            f"Incorrect MAC canonicalization of {question} at position {n}"
