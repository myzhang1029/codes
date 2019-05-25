#!/usr/bin/env python3
# Print numbers that their nth root, n_2th root,... are all intergers
# like 4096, 262144, 729 for 2 and 3
#
#
#  cnsroot.py
#  Copyright (C) 2018-present Zhang Maiyun <myzhang1029@163.com>
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

import argparse
from sys import exit
from sbl import lcm

class LcmAction(argparse.Action):
    def __call__(self, parser, namespace, values, option_string=None):
        old = namespace.add_multiple
        namespace.add_multiple=lcm(values, old) if old is not None else values

parser = argparse.ArgumentParser(description="Print numbers that their nth roots are all integers")
parser.add_argument("-b", "--base-max", help="maximum of the base[3]", type=int, default=3)
parser.add_argument("-p", "--power-max", help="maximum of the power[3]", type=int, default=3)
parser.add_argument("-m", "--add-multiple", help="add a multiple", type=int, action=LcmAction, default=1)
args = parser.parse_args()

for base in range(0, args.base_max + 1):
    for power in range(0, args.power_max + 1):
        print(base ** (args.add_multiple * power))
