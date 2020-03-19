#!/usr/bin/env python3
#
#
#  generation.py
#  Copyright (C) 2018 Zhang Maiyun <myzhang1029@hotmail.com>
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
"""One man has n children, his children have n-1 children each,
they have n-2 children each, how many people are there in the family
after n generations? The mother side not included.
n specified in argv[1], defaults to 1.
This program actually calculates OEIS:A000522"""

from sys import argv

try:
    men = int(argv[1])
except IndexError:
    men = 1

count = men  # count for generations
total = 1  # People in total
total += men
while count - 1:
    count -= 1
    men *= count
    total += men

print(total)
