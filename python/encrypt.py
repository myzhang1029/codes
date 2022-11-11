#!/usr/bin/env python3
#
#  encrypt.py
#
#  Copyright (C) 2020 Zhang Maiyun <me@myzhangll.xyz>
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

"""Demonstrate a simple encryption method without key exchange, unsafe."""

from math import ceil, log


def key2num(key):
    return int.from_bytes(key.encode(), "big")


def encode1(s, key):
    p = key2num(key)
    return int.from_bytes(s.encode(), "big")*p


def encode2(s, key):
    p = key2num(key)
    return s*p


def decode1(s, key):
    p = key2num(key)
    return s//p


def decode2(s, key):
    p = key2num(key)
    value = s//p
    maxlen = ceil(log(s, 0xff))
    return value.to_bytes(maxlen, "big").decode().split("\x00")[-1]
