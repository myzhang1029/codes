#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
#
#  healthrep.py
#  Copyright (C) 2020 Zhang Maiyun <myzhang1029@hotmail.com>
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

"""Generate health report based on wjx.com report spreadsheet."""

import sys
import tempfile

import requests

import xlrd


def geturl(url):
    """Read a URL and save its content to a temporary file.
    return the file
    """
    temp = tempfile.NamedTemporaryFile()
    r = requests.get(url)
    if r.status_code != 200:
        r.raise_for_status()
    temp.write(r.content)
    return temp


def getresult(xlsfile):
    """Read an xls report sheet.
    return the output list
    """
    xls = xlrd.open_workbook(xlsfile)
    ws = xls.sheet_by_index(0)
    lines = []
    for i in range(1, ws.nrows):
        number = ("00" + ws.cell(i, 6).value)[-2:]  # Pad zeroes
        name = ws.cell(i, 7).value
        yesorno = ws.cell(i, 9).value
        healthstat = ws.cell(i, 8).value
        curline = f"{number} {name} {healthstat} {yesorno}"
        lines.append(curline)
    return lines


def kickrep(healthlist):
    """Kick out repeated reports."""
    healthlist.sort()
    oldnum = 0
    for idx in reversed(range(len(healthlist))):
        rep = healthlist[idx]
        thisnum = rep[0:2]
        thisline = rep
        oldline = ''
        if thisnum == oldnum:
            # Skip verification for identical lines
            if thisline == oldline:
                del healthlist[idx]
            else:
                print(f"""发现重复项:
1: {oldline}
2: {thisline}""")
                while True:
                    keep = input("保留哪一项: ")
                    # Delete the opposing one
                    if keep == "2":
                        del healthlist[idx+1]
                    elif keep == "1":
                        del healthlist[idx]
                    else:
                        continue
                    break
        oldnum = thisnum
        oldline = thisline
    return healthlist


def main():
    template = """各位家长2020届初三年级各班学生从1月10日以来前往过湖北和武汉的情况报送表，请在交流群中接龙完成，谢谢～
如：
学号  姓名  健康状况 是否到过湖北或接触湖北人
{}"""
    try:
        tmpfile = geturl(sys.argv[1])
        filename = tmpfile.name
        healthlist = getresult(filename)
        tmpfile.close()
    except requests.exceptions.MissingSchema:
        healthlist = getresult(sys.argv[1])
    healthlist = kickrep(healthlist)
    result = '\n'.join(healthlist)
    # Output to stderr so that piping is easier
    print(template.format(result), file=sys.stderr)


if __name__ == "__main__":
    main()
