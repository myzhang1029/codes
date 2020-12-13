#!/usr/bin/env python3
"""Select a point within a given lat/lon."""
import sys

delta = float(sys.argv[1])
for i in open("data").readlines():
    line = i.strip().split(',')
    if (116.585-delta <= float(line[2]) <= 116.585+delta
            and 40.08-delta <= float(line[3]) <= 40.08+delta):
        print(i)
