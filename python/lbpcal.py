#!/usr/bin/env python3
#
#  lbpcal.py
#
#  Copyright (C) 2021-2022 Zhang Maiyun <me@myzhangll.xyz>
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
from typing import List, Tuple

import pandas as pd
from icalendar import Calendar, Event

# Pearson time
TZ = "America/Vancouver"


def isams_to_ics(excel_file: str, tz: str) -> Calendar:
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
        event.add("summary", event_desc["Description"])
        start = event_desc["Start Date_Start Time"].tz_localize(tz)
        end = event_desc["Start Date_End Time"].tz_localize(tz)
        event.add("dtstart", start)
        if event_desc["All Day Event"]:
            event.allday = True
            event.add("dtstart", start.date())
            event.add("dtend", start.date() + pd.Timedelta(days=1))
        else:
            event.add("dtstart", start)
            event.add("dtend", end)
        cal.add_component(event)
    return cal


def web_to_ics(url: str, tz: str) -> Calendar:
    """Convert a pearsoncollege.com website to ics.

    Parameters
    ----------
    url: str
        URL to the data.
    tz : str, pytz.timezone, dateutil.tz.tzfile or None
        Time zone in which `excel_file` is local.

    Returns
    -------
    Calendar
        Decoded calendar object.
    """
    df = pd.read_xml(url, xpath=".//iSAMS_CALENDARMANAGER/*")
    # Create cal
    cal = Calendar()
    for event_desc in df.iloc:
        event = Event()
        event.add("summary", event_desc["description"])
        if event_desc["notes"]:
            event.add("description", event_desc["notes"])
        if event_desc["location"]:
            event.add("location", event_desc["location"])
        if event_desc["submitdate"]:
            event.add("dtstamp", pd.to_datetime(
                event_desc["submitdate"],
                format="%d/%m/%Y"
            ).tz_localize(tz))
        if event_desc["starttime"]:
            start = pd.to_datetime(
                event_desc["startdate"] + "-" + event_desc["starttime"],
                format="%d/%m/%Y-%H:%M"
            ).tz_localize(tz)
            end = pd.to_datetime(
                event_desc["startdate"] + "-" + event_desc["endtime"],
                format="%d/%m/%Y-%H:%M"
            ).tz_localize(tz)
        else:
            assert event_desc["alldayevent"]
            event.allday = True
            start = pd.to_datetime(
                event_desc["startdate"],
                format="%d/%m/%Y"
            ).tz_localize(tz).date()
            end = start + pd.Timedelta(days=1)
        event.add("dtstart", start)
        event.add("dtend", end)
        cal.add_component(event)
    return cal


def main_xml(argv: List[str]) -> Tuple[str, Calendar]:
    """Convert from Pearson website XML to ics, driver."""
    if len(argv) != 4:
        print(f"Usage: {argv[0]} <Start> <End> <Output.ics>", file=sys.stderr)
        print("Specify dates in the form `dd/mm/yyyy'", file=sys.stderr)
        sys.exit(1)
    # Parser stuff
    start, end = argv[1], argv[2]
    outfile = argv[3]
    url = f"https://pearsoncollegeproxy.azurewebsites.net/isams-calendar?date={start}&endDate={end}"
    cal = web_to_ics(url, TZ)
    return outfile, cal


def main_xlsx(argv: List[str]) -> Tuple[str, Calendar]:
    """Convert from xlsx to ics, driver."""
    # Simple argument parser
    if len(argv) == 2:
        infile, outfile = argv[1], argv[1] + ".ics"
    elif len(argv) == 3:
        infile, outfile = argv[1], argv[2]
    else:
        print(f"Usage: {argv[0]} <Input.xlsx> [Output.ics]", file=sys.stderr)
        sys.exit(1)
    cal = isams_to_ics(infile, TZ)
    return outfile, cal


def main() -> None:
    """Dispatcher."""
    mode = "xlsx"
    # Not the best way to do functional programming. Don't do this

    def filter_function(arg: str) -> bool:
        """Helper to filter and update `sys.argv`."""
        nonlocal mode
        # Parse and dispose options
        if arg in ("--xml", "--web"):
            mode = "xml"
            return False
        if arg in ("--xlsx", "--excel"):
            return False
        if arg[:2] in ("--", "-h"):
            # Therefore don't name the program something like `--lbpcal.py'(UB)
            print("--xml to parse Pearson website,"
                  "--xlsx (default) to parse iSAMS export.", file=sys.stderr)
            sys.exit(1)
        # Keep non-options
        return True
    argv = list(filter(filter_function, sys.argv))
    outfile, cal = main_xlsx(argv) if mode == "xlsx" else main_xml(argv)
    if Path(outfile).exists():
        if input(f"{outfile} exists, overwrite? [y/N] ").lower() != "y":
            sys.exit(2)
    open(outfile, "wb").write(cal.to_ical())


if __name__ == "__main__":
    main()
