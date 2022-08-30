#!/usr/bin/env python3

"""Automatically or manually deal with clashes."""

import re
from datetime import timedelta
from pprint import pformat

import IPython
import portalocker
import requests
from pygments import highlight
from pygments.formatters import Terminal256Formatter
from pygments.lexers import PythonLexer
from ratelimit import limits, sleep_and_retry

import manage_hosts

can_merge_mac_hns = [re.compile("^[a-z0-9]{32}$")]
can_merge_hn_macs = [re.compile("^lzkPH42CJ7Y3jg$")]
ignore_hns = ("Android", "iPhone", "iPad", "Samsung", "<Ubiquiti AP>", "<unknown>", "Pixel-4", "Pixel-4a",
              "Google-Home-Mini", "XboxOne", "MacBook-Pro", "MacBook-Air", "DW10", "Google-Nest-Mini", "Pixel-4a")


def pprint(obj):
    """Pretty-print and highlight an object."""
    formatted_object = pformat(obj)
    print(highlight(formatted_object, PythonLexer(), Terminal256Formatter()))


def match_hostname(res, entry):
    for re in res:
        for hn in entry["hostnames"]:
            match = re.match(hn)
            if match:
                return match
    return None


def match_mac(res, entry):
    for re in res:
        for mac in entry["macs"]:
            match = re.match(mac)
            if match:
                return match
    return None


def cm(x):
    """Prompt and merge."""
    pprint(m[m.find_indices_by_mac(x)])
    if input() == "y":
        m.merge(m.find_indices_by_mac(x))


@sleep_and_retry
@limits(calls=1, period=1.0)
def get_mac_vendor(mac):
    """Get the vendor of a MAC address."""
    response = requests.get("https://api.macvendors.com/" + mac)
    return response.text


with portalocker.Lock("/home/ubuntu/mdns/database.json.lock", timeout=0):
    m = manage_hosts.MacDatabase("/home/ubuntu/mdns/database.json")

    print("MAC clashes:")
    have = set()
    clash = set()
    for n, i in enumerate(m):
        for mac in i["macs"]:
            if mac in have:
                print(n, i)
                clash.add(mac)
            have.add(mac)
    print()

    for mac in clash:
        indices_to_merge = set()
        auto = True
        print(f"Dealing with {mac}...", end=" ")
        for ind in m.find_indices_by_mac(mac):
            print(ind, end=", ")
            if "<mac merge exception>" not in m[ind]["comments"]:
                indices_to_merge.add(ind)
            if not match_hostname(can_merge_mac_hns, m[ind]):
                # Disable auto if any host does not conform
                auto = False
        print()
        pprint(m[indices_to_merge])
        if not indices_to_merge:
            continue
        if auto:
            m.merge(indices_to_merge)
            if "<unknown>" in m[-1]["hostnames"] and len(m[-1]["hostnames"]) != 1:
                m[-1]["hostnames"].remove("<unknown>")
        else:
            response = input("Merge those [N/y/s] ?").lower()
            if response == "y":
                m.merge(indices_to_merge)
                if "<unknown>" in m[-1]["hostnames"] and len(m[-1]["hostnames"]) != 1:
                    m[-1]["hostnames"].remove("<unknown>")
            elif response == "s":
                IPython.embed(colors="neutral")
    m.save()

    print("Hostname clashes:")
    have = set()
    clash = set()
    for n, i in enumerate(m):
        for hn in i["hostnames"]:
            if hn.startswith("Android-") or hn.startswith("iPhone-") or hn.startswith("iPad-") or hn.startswith("Samsung-") or hn.startswith("Galaxy-"):
                continue
            if any(manage_hosts.hostname_fuzz(i, hn) for i in ignore_hns):
                continue
            if hn in have:
                print(n, i)
                clash.add(hn)
                break
            have.add(hn)
    print()

    for hn in clash:
        indices_to_merge = set()
        auto = True
        print(f"Dealing with {hn}...", end=" ")
        for ind in m.find_indices_by_hostname(hn):
            print(ind, end=", ")
            if "<hostname merge exception>" not in m[ind]["comments"]:
                indices_to_merge.add(ind)
            if not match_mac(can_merge_hn_macs, m[ind]):
                # Disable auto if any host does not conform
                auto = False
        print()
        pprint(m[indices_to_merge])
        if not indices_to_merge:
            continue
        if auto:
            m.merge(indices_to_merge)
        else:
            for host in m[indices_to_merge]:
                for mac in host["macs"]:
                    print(mac, get_mac_vendor(mac))
            response = input("Merge those [N/y/s] ?").lower()
            if response == "y":
                m.merge(indices_to_merge)
            elif response == "s":
                IPython.embed(colors="neutral")
        if "<unknown>" in m[-1]["hostnames"] and len(m[-1]["hostnames"]) != 1:
            m[-1]["hostnames"].remove("<unknown>")
    m.save()
