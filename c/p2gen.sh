#!/bin/bash

# Auto generate prime2_n.c for multi-process calculating

# p2gen.sh
# Copyright (C) 2018 Zhang Maiyun <me@maiyun.me>
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
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

base="$(dirname "$0")"
while true
do
	echo -n "Enter the minimum value:"
    read -r min
    if [ "$((min))" = "$min" ]; then
		break
	else
		echo Must be an integer
	fi
done
while true
do
	echo -n "Enter the maximum value (this value will not be included):"
    read -r max
    if [ "$((max))" = "$max" ]; then
		break
	else
		echo Must be an integer
	fi
done
diff="$((max-min))"
while true
do
    echo -n "Enter increase value:"
    read -r incr
    if [ "$((incr))" = "$incr" ]; then
        if [ "$((incr%4))" = "0" ]; then
			if [ "$incr" -le "$diff" ]; then
                if [ "$((diff%incr))" = "0" ]; then
					break
				else
					echo Must be a factor of max-min
				fi
			else
				echo Must be lower than max-min
			fi
		else
			echo Must be a multiple of 4
		fi
	else
		echo Must be an integer
	fi
done
count=0
for i in $(seq "$min" "$incr" $((max-incr)))
do
    cp "${base}"/prime2.c prime2_"${count}".c
	gsed -i "s/@p2gen_min @/$i/" prime2_${count}.c
	gsed -i "s/@p2gen_max @/$((i+incr))/" prime2_${count}.c
	count="$((count+1))"
done
