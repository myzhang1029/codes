#!/bin/sh

# Auto generate prime2_n.c for multi-process calculating

# p2gen.sh
# Copyright (C) 2018 Zhang Maiyun <myzhang1029@163.com>
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
base=`dirname $0`
while true
do
	read -p "Enter the minimum value:" min
	if [ "$(echo $min | bc)" = "$min" ]; then
		break
	else
		echo Must be an integer
	fi
done
while true
do
	read -p "Enter the maximum value (this value will not be included):" max
	if [ "$(echo $max | bc)" = "$max" ]; then
		break
	else
		echo Must be an integer
	fi
done
diff=`echo ${max}-${min} | bc`
while true
do
	read -p "Enter increase value:" incr
	if [ "$(echo $incr | bc)" = "$incr" ]; then
		if [ "$(echo ${incr}%4 | bc)" = "0" ]; then
			if [ $incr -le $diff ]; then
				if [ "$(echo ${diff}%${incr} | bc)" = "0" ]; then
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
for i in `seq $min $incr $(($max-$incr))`
do
    cp ${base}/prime2.c prime2_${count}.c
	gsed -i "s/@p2gen_min @/$i/" prime2_${count}.c
	gsed -i "s/@p2gen_max @/$(($i+$incr))/" prime2_${count}.c
	((count+=1))
done
