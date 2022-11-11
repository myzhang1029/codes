#!/bin/sh
#
#  btrfs_snap - Take a snapshot of all btrfs subvolumes
#  Copyright (C) 2021 Zhang Maiyun <me@myzhangll.xyz>
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


# Go over the subvolumes
for subvol in / $(btrfs subvolume list / --sort=-path | cut -f9 -d' ')
do
    # Suggest a name based on folder name
    guessed_name="$(basename -- "$subvol" | sed s,/,rootfs,)"
    echo "What is the name for '$subvol' [$guessed_name]?"
    read -r name
    # Use guessed name if empty
    if  [ "$name" = "" ]
    then
        name="$guessed_name"
    elif  [ "$name" = "skip" ]
    then
        # Allow user to skip subvolumes like /swap
        continue
    fi
    # Add date and context to the name
    bkname="/btrfs_snapshots/$name-$(date +%Y-%m-%dT%H:%M%z)"
    # Do the snapshot
    btrfs subvolume snapshot "/$subvol" "$bkname"
done

exit 0
