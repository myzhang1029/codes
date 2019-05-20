#!/usr/bin/env python3
# Print numbers that their nth root, n_2th root,... are all an interger
# like 4096, 262144, 729 for 2 and 3
#
#
# cnsroot.py
# Copyright (C) 2017 Zhang Maiyun <myzhang1029@163.com>
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
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

from getopt import gnu_getopt, GetoptError
from sys import argv, exit
from sbl import lcm

multiple = 1
bmax = 3
pmax = 3


def usage(returncode):
    print("options:\n")
    print("-b, --base-max arg: maximum of the base\n")
    print("-p, --power-max arg: maximum of power/multiple\n")
    print("-m, --add-multiple arg: add a multiple\n")
    print("-h, --help: show this\n")
    exit(int(returncode))


try:
    opts, args = gnu_getopt(argv[1:], "b:p:m:h",
                            ["base-max=", "power-max=", "add-multiple=", "help"])
except GetoptError as err:
    print(err)

for o, a in opts:
    if o in ("-b", "--base-max"):
        bmax = long(a)
    elif o in ("-p", "--power-max"):
        pmax = long(a)
    elif o in ("-m", "--add-multiple"):
        multiple = lcm(multiple, long(a))
    elif o in ("-h", "--help"):
        usage(0)
    else:
        print("unknown option", o)
        usage(1)

for base in range(0, bmax+1):
    for power in range(0, pmax+1):
        print(base**(multiple * power))
