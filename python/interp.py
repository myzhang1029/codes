#!/usr/bin/env python3
#
#  interp.py
#
#  Copyright (C) 2021 Zhang Maiyun <myzhang1029@hotmail.com>
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


"""Simple script to translate UTF-8 encoded Chinese to Pinyin on-the-fly.
Dataset:
    ftp://ftp.cuhk.hk/pub/chinese/ifcss/software/data/Uni2Pinyin.gz
"""

import os.path as op
import sys

dbfile = op.dirname(op.abspath(__file__)) + "/Uni2Pinyin"
db = open(dbfile)
entries = {}
for line in db.readlines():
    if line[0] == '#':
        continue
    pair = line.split()
    entries[pair[0]] = ':'.join(pair[1:])

data = sys.stdin.read()
for c in data:
    code = hex(ord(c))[2:].upper()
    if code in entries:
        print(entries[code], end='')
    else:
        print(c, end='')
