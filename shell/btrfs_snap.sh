#!/bin/sh
# Take a snapshot of all btrfs subvolumes

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

