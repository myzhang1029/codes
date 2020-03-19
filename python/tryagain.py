#!/usr/bin/env python3
#
#  tryagain.py
#  Copyright (C) 2019 Zhang Maiyun <myzhang1029@hotmail.com>
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

"""Run something until it succeeds."""

import subprocess
import sys

ntries = 1

print("Try 1")

while subprocess.run(sys.argv[1:]).returncode != 0:
    ntries += 1
    print(f"Try {ntries}")
