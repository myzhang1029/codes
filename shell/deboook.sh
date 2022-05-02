#!/bin/sh
#
#  debook - Convert double-sided book style PDFs to single paged PDFs
#  Copyright (C) 2021-2022 Zhang Maiyun <myzhang1029@hotmail.com>
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

set -e

usage()
{
    echo "Convert double-sided book style PDF with esoteric page numbers to single page PDF."
    echo "This program can correctly deal with files with varying page sizes."
    echo "Requires coreutils, grep, awk, pdfinfo and gs."
    echo
    echo "Usage: $1 [OPTIONS] INPUT OUTPUT"
    echo
    echo "Options:"
    echo "  -f: Assume the first page is the cover pages (default assumes center page)."
    echo "  -v: Assume the layout is vertical (i.e. Two pages stack on each other)."
    echo "  -s: Assume the original document is scanned according to the correct order,"
    echo "      and only split each page into two. Ignores -f if specified."
    echo "  -q: Prevent additional information from being printed."
    echo "  -h: Print this help."
    echo
    echo "Report bugs to Zhang Maiyun<myzhang1029@hotmail.com>."
    if [ "$2" != "" ]
    then
        echo
        echo "$2"
    fi
}

# Output a page
# $1: target page number
# $2: original page number
# $3: cropbox
output_page()
{
    "$logger" "Page $1. Original WxH is ${width}x$height."
    gs \
        -dNOPAUSE \
        -dBATCH \
        -dQUIET \
        -o "$output_dir/$1.pdf" \
        -sDEVICE=pdfwrite \
        -dFirstPage="$2" \
        -dLastPage="$2" \
        -c "[/CropBox [$3]" \
        -c " /PAGES pdfmark" \
        -f "$input"
}

# Get page dimensions
get_dim()
{
    size="$(pdfinfo -f "$idx" -l "$idx" "$input" | grep "$idx size")"
    rot="$(pdfinfo -f "$idx" -l "$idx" "$input" | grep "$idx rot" | awk '{print $4}')"
    if [ "$((rot % 180))" -eq 90 ]
    then
        height="$(echo "$size" | awk '{print $4}')"
        width="$(echo "$size" | awk '{print $6}')"
    else
        height="$(echo "$size" | awk '{print $6}')"
        width="$(echo "$size" | awk '{print $4}')"
    fi
}

# No options given
if [ "$#" -eq 0 ]
then
    usage "$1"
    exit 1
fi

# Options
fopt=0
vopt=0
sopt=0
logger="echo"

# Parse dashed arguments
while [ "$(echo "x$1" | cut -c2)" = "-" ]
do
    opt="$1"
    while true
    do
        # Take the next option
        opt="$(echo "x$opt" | cut -c3-)"
        # No more options
        if [ "y$opt" = "y" ]
        then
            break
        fi
        # Parse this option
        thisopt="$(echo "x$opt" | cut -c2)"
        case "$thisopt" in
            f) fopt=1;;
            v) vopt=1;;
            s) sopt=1;;
            q) logger="true";;
            h)
                usage "$1"
                exit 0
                ;;
            # Stop processing
            -)
                shift
                break 2
                ;;
            # Undefined options
            *)
                usage "$1" "Unrecognized option -$thisopt." >&2
                exit 1
                ;;
        esac
    done
    shift
done

# No file given
if [ "$#" -ne 2 ]
then
    usage "$1" "Wrong number of position arguments." >&2
    exit 1
fi

input="$1"
output="$2"

npages="$(pdfinfo "$input" | grep "Pages:" | awk '{print $2}')"

"$logger" "$npages pages to be processed."

output_dir="$(mktemp -d)"

"$logger" "Outputting into $output_dir"

for idx in $(seq 1 "$npages")
do
    # All page numbers start from 1
    if [ "$sopt" -eq 1 ]
    then
        # Simple mode, just split two pages
        lopage="$((2 * idx - 1))"
        hipage="$((2 * idx))"
    elif [ "$fopt" -eq 0 ]
    then
        # Processing from the center
        lopage="$((npages - idx + 1))"
        hipage="$((npages + idx))"
    else
        # Processing from the cover
        lopage="$idx"
        hipage="$((2 * npages - idx + 1))"
    fi
    # Get page geometry
    get_dim
    # If this page is odd-numbered (from the first page),
    # then the smaller-numbered page is on the left
    # else, it is on the right
    # For simple mode, the smaller-numbered page is always on the left.
    if [ "$sopt" -eq 1 ] || [ "$((idx % 2))" -eq 1 ]
    then
        # Odd-numbered page
        if [ "$vopt" -eq 0 ]
        then
            # Left, smaller
            cropbox="0 0 $(echo "$width/2" | bc) $height"
            output_page "$lopage" "$idx" "$cropbox"
            # Right, larger
            cropbox="$(echo "$width/2" | bc) 0 $width $height"
            output_page "$hipage" "$idx" "$cropbox"
        else
            # Top, smaller
            cropbox="0 $(echo "$height/2" | bc) $width $height"
            output_page "$lopage" "$idx" "$cropbox"
            # Bottom, larger
            cropbox="0 0 $width $(echo "$height/2" | bc)"
            output_page "$hipage" "$idx" "$cropbox"
        fi
    else
        # Even-numbered page
        if [ "$vopt" -eq 0 ]
        then
            # Left, larger
            cropbox="0 0 $(echo "$width/2" | bc) $height"
            output_page "$hipage" "$idx" "$cropbox"
            # Right, smaller
            cropbox="$(echo "$width/2" | bc) 0 $width $height"
            output_page "$lopage" "$idx" "$cropbox"
        else
            # Top, larger
            cropbox="0 $(echo "$height/2" | bc) $width $height"
            output_page "$hipage" "$idx" "$cropbox"
            # Bottom, smaller
            cropbox="0 0 $width $(echo "$height/2" | bc)"
            output_page "$lopage" "$idx" "$cropbox"
        fi
    fi
done

files="$(seq 1 $((npages * 2)) | sed "s,\$,.pdf," | tr '\n' ' ')"

# Convert to absolute path
if [ "$(echo "x$output" | cut -c2)" != "/" ]
then
    output="$PWD/$output"
fi

# Not prepending path to avoid over-lengthed command line
cd "$output_dir"
# files must be splitted and there will be no globs since filenames are generated
# shellcheck disable=SC2086
gs -dBATCH -dNOPAUSE -dQUIET -sDEVICE=pdfwrite -sOutputFile="$output" $files

rm -rf "$output_dir"
exit 0
