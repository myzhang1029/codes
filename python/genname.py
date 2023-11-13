#!/usr/bin/env python3
#
#  genname.py
#
#  Copyright (C) 2021 Zhang Maiyun <me@maiyun.me>
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

"""Generate team name based on member names."""

import itertools
import sys

def printnames(names):
    """Print all permutations."""
    # Length of all names
    lengths=[len(name) for name in names]
    # Length of an output
    maxlen=min(min(lengths), len(names))
    indices=list(range(maxlen))
    for i in itertools.permutations(indices):
        for n,j in enumerate(i):
            print(names[j][n], end="")
        print()

if __name__ == "__main__":
    printnames(sys.argv[1:])
