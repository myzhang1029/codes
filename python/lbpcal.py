#!/usr/bin/env python3
#
#  lbpcal.py
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

"""Make iCalendar files from iSAMS exports."""

import sys
from pathlib import Path

import pandas as pd
from ics import Calendar, Event


def isams_to_ics(excel_file, tz) -> Calendar:
    """Convert a LBPC iSAMS export to ics.

    Parameters
    ----------
    excel_file : FileLike
        iSAMS-formatted Excel file of events.
    tz : str, pytz.timezone, dateutil.tz.tzfile or None
        Time zone in which `excel_file` is local.

    Returns
    -------
    Calendar
        Decoded calendar object.
    """
    # Read Excel
    df = pd.read_excel(excel_file, parse_dates=[
                       ["Start Date", "Start Time"], ["Start Date", "End Time"]])
    # Create cal
    cal = Calendar()
    for event_desc in df.iloc:
        event = Event()
        event.name = event_desc["Description"]
        event.begin = event_desc["Start Date_Start Time"].tz_localize(tz)
        event.end = event_desc["Start Date_End Time"].tz_localize(tz)
        if event_desc["All Day Event"]:
            event.make_all_day()
        cal.events.add(event)
    return cal


def main():
    """Do the conversion."""
    # Simple argument parser
    if len(sys.argv) == 2:
        infile, outfile = sys.argv[1], sys.argv[1] + ".ics"
    elif len(sys.argv) == 3:
        infile, outfile = sys.argv[1], sys.argv[2]
    else:
        print(
            f"Usage: {sys.argv[0]} <Input.xlsx> [Output.ics]", file=sys.stderr)
        sys.exit(1)
    # Pearson time
    cal = isams_to_ics(infile, "America/Vancouver")
    outfile = Path(outfile)
    if outfile.exists():
        if input(f"{outfile} exists, overwrite? [y/N] ").lower() != "y":
            sys.exit(2)
    outfile.open("w").write(str(cal))


if __name__ == "__main__":
    main()
