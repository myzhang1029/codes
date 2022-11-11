#!/usr/bin/env python3
#
#  admkcsv.py
#
#  Copyright (C) 2019-2020 Zhang Maiyun <me@myzhangll.xyz>
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

"""Format the administrative division code page from website of the Ministry
of Civil Affairs of the People's Republic of China into CSV files."""

from pathlib import Path
from sys import argv
from urllib.parse import urlparse

from bs4 import BeautifulSoup as bs
from requests import get


def gencsv(lines, name):
    """ Generate a CSV file from the list. """
    nl = []
    for i, l in enumerate(lines):
        if i % 2 == 0:
            nl.append(l[0:-1]+",")
        else:
            nl.append(l)
    open(name + ".csv", "w").writelines(nl)


def readhtml(html):
    """ Read an HTML string and format it to a list.
    lines: The HTML content.
    The content should be from mca.gov or have the same structure.
    """
    out = []
    isfirstline = True
    soup = bs(html, "html.parser")
    for child in soup.body.children:  # remove extra tags
        if child.name == 'script':
            child.decompose()
    for line in soup.body.get_text().split("\n"):  # extract text
        stripped = line.strip()
        if stripped == "":  # skip empty lines
            continue
        if isfirstline:  # skip the title line
            isfirstline = False
            continue
        if stripped.find("注") != -1:  # end reached
            break
        out.append(stripped + "\n")
    return out


def start():
    for i in argv[1:]:
        o = urlparse(i)
        if o.scheme:  # a URL entered
            outname = Path(o.path).stem
            lines = get(i).content.decode()
        else:  # a normal path
            outname = i[:-5]
            lines = open(i).read()
        lines = readhtml(lines)
        gencsv(lines, outname)


start()
