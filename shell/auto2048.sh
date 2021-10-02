#!/bin/sh
#
#  auto2048.sh - Play the famous 2048 game automatically, and see how
#  smart (or silly) your computer is!
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

# Prereq: download https://github.com/mevdschee/2048.c and compile it to ./2048

# Number of iterations that the game will run
ITERS=1
# 2048.c scheme
SCHEME=
TRUNCATE_SCORE=true

while [ $# -ne 0 ]
do
    # Augument parse
    if [ "$1" = "-h" ]
    then
        echo "$0" "[-hn] [SCHEME] [LOOPS]"
        echo ""
        echo "Options:"
        echo "-h: Print this message."
        echo "-n: Don't truncate the existing score file."
        echo ""
        echo "SCHEME:"
        echo "  none: default scheme."
        echo "  blackwhite: black-to-white color scheme (requires 256 colors)."
        echo "  bluered: blue-to-red color scheme (requires 256 colors)."
        echo ""
        echo "LOOPS:"
        echo "  Number of iterations that the game will be run. Defaults to 1."
        echo "  You can specify any number of any of those arguments. The order"
        echo "  does not matter, but only the last one will be effective."
        echo "  i.e., the last numeral will be LOOPS, and the last non-numeral"
        echo "  will be SCHEME."
        echo ""
        exit 0
    elif [ "$1" = "-n" ]
    then
        TRUNCATE_SCORE=false
    elif [ "$1" -eq "$1" ] 2> /dev/null # is a number, should be LOOPS
    then
        ITERS="$1"
    else # not a number, should be SCHEME
        SCHEME="$1"
    fi
    shift
done

# Generate strings like "wasdwasd" for 2048
gen_input()
{
    while true
    do
        n="$(awk 'BEGIN { srand(); print int(rand()*4) }' /dev/null)"
        case $n in
            0) printf w;; # up
            1) printf a;; # left
            2) printf s;; # down
            3) printf d;; # right
        esac
    done
}

# Truncate score file
if "$TRUNCATE_SCORE"
then
    : > score
fi

# Base directory of this program
basedir="$(dirname "$0")"
if [ "$basedir" = "" ] # PATH search needed
then
    cmd2048=2048
else
    cmd2048="$basedir/2048"
fi

# Main loop
for _ in $(seq 1 "$ITERS")
do
    gen_input | "$cmd2048" "$SCHEME" 2>> score
    sleep 3
done

exit 0
