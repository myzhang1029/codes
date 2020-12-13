#!/usr/bin/env python3

"""Filter out the stations in Beijing, by hand with assistant."""

import glob

# Load the info of all stations, leading and trailing quotation marks removed
stations_info = [[cell[1:-1] for cell in line.strip().split(',')]
                 for line in open("instruction/isd-history.csv").readlines()]

# Load all station IDs in this database, uniquified by Python set
stations_have = list({i.split('/')[1].split('-')[0]
                      for i in glob.glob("china_isd_lite_201*/*")})

bjstations = []

for n, station_id in enumerate(stations_have):
    for b in stations_info:
        if b[0] == station_id:
            if b[3] != "CH":
                # Skip stations in HK, Macau, Taiwan
                continue
            if not (39.2 <= float(b[6]) <= 41.1
                    and 115.2 <= float(b[7]) <= 117.9):
                # Skip stations outside of this raw bounding box of BJ (WGS-84)
                continue
            yorn = input(
                f"[{n}/{len(stations_have)}] state: {b[4]}, "
                f"name: {b[2]}, lat: {b[6]}, lon: {b[7]}\n[N/y]> ")
            if yorn.lower() == "y":
                bjstations.append(station_id+'\n')

open("output-ids_keep.txt", "w").writelines(bjstations)
