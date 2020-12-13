#!/usr/bin/env python3

"""Find missing data in the Beijing Air Quality Dataset."""

import os

# Months with 31 days
mo32 = {1, 3, 5, 7, 8, 10, 12}

# Iter Beijing PM2.5 data
for yr in range(2015, 2020):
    for mo in range(1, 13):
        for d in range(1, 32 if mo in mo32
                       else (30 if yr == 2016 else 29) if mo == 2
                       else 31):
            date = f"{yr}{mo:02}{d:02}"
            try:
                os.stat(f"beijing_{yr}0101-{yr}1231/beijing_all_{date}.csv")
            except FileNotFoundError:
                print("all", date)
            try:
                os.stat(f"beijing_{yr}0101-{yr}1231/beijing_extra_{date}.csv")
            except FileNotFoundError:
                print("extra", date)
