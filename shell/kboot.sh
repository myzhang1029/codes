#!/bin/sh
# kboot
# kboot [kernel] [params] [append params ...]

# The first arg (if any) specifies the kernel, where a '-' means use the current kernel.
# If the 2nd arg is '-' or is omitted, then the existing kernel parameters are appended.
# Any remaining args are also appended to the kernel parameters.

if [ $# -eq 0 ]; then
    reuse=--reuse-cmdline
else
    if ! [ "$1" = "-" ]; then
        kernel="$1"
    fi
    shift
fi
if [ $# -eq 0 ]; then
    reuse=--reuse-cmdline
elif [ "$1" = "-" ]; then
    reuse=--reuse-cmdline
    shift
fi
kernel="${kernel:-$(uname -r)}"
kargs="/boot/vmlinuz-$kernel --initrd=/boot/initrd.img-$kernel"

kexec -l -t bzImage $kargs $reuse --append="$*" && systemctl kexec
