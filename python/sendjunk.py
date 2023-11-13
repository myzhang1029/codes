#!/usr/bin/env python3
#
#  sendjunk.py
#
#  Copyright (C) 2020 Zhang Maiyun <me@maiyun.me>
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

"""Generate junk and send through AppleScript.
Make sure to select the appropriate persion."""

import random
import subprocess as sp
import time

tk = """
tell application "WeChat"
\tactivate
end tell
set the clipboard to "{}"
tell application "System Events" to keystroke "v" using command down
tell application "System Events" to keystroke return
"""
mp = {
    0: "zero",
    1: "one",
    2: "two",
    3: "three",
    4: "four",
    5: "five",
    6: "six",
    7: "seven",
    8: "eight",
    9: "nine",
    10: "ten",
    11: "eleven",
    12: "twelve",
    13: "thirteen",
    14: "fourteen",
    15: "fifteen",
    16: "sixteen",
    17: "seventeen",
    18: "eighteen",
    19: "nineteen",
    20: "twenty",
    30: "thirty",
    40: "forty",
    50: "fifty",
    60: "sixty",
    70: "seventy",
    80: "eighty",
    90: "ninety",
}


def number2words(num: int) -> str:
    string = ""
    if num == 0:
        return mp[0]
    if num >= 1000000000000:
        under = num//1000000000000
        string += small2words(under) + " trillion"
        num %= 1000000000
        if num != 0:
            string += ","
    if num >= 1000000000:
        under = num//1000000000
        string += small2words(under) + " billion"
        num %= 1000000000
        if num != 0:
            string += ","
    if num >= 1000000:
        under = num//1000000
        string += small2words(under) + " million"
        num %= 1000000
        if num != 0:
            string += ","
    if num >= 1000:
        under = num//1000
        string += small2words(under) + " thousand"
        num %= 1000
        if num != 0:
            string += ","
    if num > 0:
        string += small2words(num)
    # Remove extra space
    return string[1:]


def small2words(num: int) -> str:
    string = ""
    addand = False
    if num >= 100:
        addand = True
        hundreds = num//100
        string += " " + mp[hundreds] + " hundred"
        num %= 100
    if num > 0:
        if addand:
            string += " and"
        if 0 < num <= 20:
            string += " " + mp[num]
        else:
            tens = num//10
            string += " " + mp[tens * 10]
            ones = num % 10
            if ones > 0:
                string += "-" + mp[ones]
    return string


if __name__ == "__main__":
    for j in range(100):
        s = number2words(random.randint(0, 1000000000000000))
        print(s)
        cmd = tk.format(s)
        sp.Popen(["osascript"], stdin=sp.PIPE).communicate(cmd.encode())
        time.sleep(5)
