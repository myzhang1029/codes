#!/usr/bin/env python3
#
#  sendjunk2.py
#
#  Copyright (C) 2020-2021 Zhang Maiyun <myzhang1029@hotmail.com>
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

"""Send an arbitrary Unicode block through AppleScript.
Make sure to select the appropriate persion."""

import subprocess as sp
import time
from itertools import zip_longest

tk = """
tell application "WeChat"
\tactivate
end tell
set the clipboard to "{}"
tell application "System Events" to keystroke "v" using command down
tell application "System Events" to keystroke return
"""

# From https://stackoverflow.com/a/434411/9347959


def grouper(iterable, n):
    args = [iter(iterable)] * n
    return zip_longest(*args)


if __name__ == "__main__":
    chars = (chr(x) for x in range(0x9FFF - 1, 0x4E00 - 1, -1))
    lines = grouper(chars, 7)
    poems = grouper(lines, 8)
    for poem in poems:
        s = '\n'.join(''.join(c if c else '' for c in line)
                      for line in poem if line)
        print(s)
        cmd = tk.format(s)
        sp.Popen(["osascript"], stdin=sp.PIPE).communicate(cmd.encode())
        time.sleep(2)
