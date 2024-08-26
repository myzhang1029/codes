#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
#  validate_detection.py
#
#  Copyright (C) 2024 Zhang Maiyun <maz005@ucsd.edu>
#   As a project developed during Maiyun's participation in the UCSD SRIP program,
#   The University of California may have claims to this work.
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


"""Validate detection results when the source event is expected to be periodic."""


import glob
import json
import re
from datetime import datetime as dt

import numpy as np
from numpy.typing import NDArray

# Parameters
# Averaged from the best results manually
# Theoretically includes the transmission time, wait time, and detection algorithm runtime
expected_delta = np.timedelta64(2072207, "us")
# Heuristic value
tolerance = np.timedelta64(50, "ms")


def find_t(p: str) -> NDArray[np.datetime64]:
    """Extract timestamps from detection results.

    The files should be in `p` directory and named as `*.dat`, where the first
    few characters of the filename are the timestamp in the given format.
    """
    dfmt = "%Y-%m-%d_%H%M%S.%f"
    dfmtlen = len(dt.strftime(dt.now(), dfmt))
    files = glob.glob("*.dat", root_dir=p)

    def parse_filename(fn: str) -> np.datetime64 | None:
        try:
            return np.datetime64(dt.strptime(fn[:dfmtlen], dfmt))
        except ValueError:
            return None
    times = np.array([d for n in files if (
        d := parse_filename(n)) is not None])
    times.sort()
    return times


def find_longest_chain(
    times: NDArray[np.datetime64],
    expected_delta: np.timedelta64,
    tolerance: np.timedelta64
) -> list[int]:
    """`times` is a sorted array of timestamps expected to be equally spaced.

    There might be some false positive detections in the timestamps, so this
    function assumes that the false positives are more or less uniformly
    distributed and the real detections will statistically stand out as the
    longest chain detected.

    The algorithm works by iterating over the timestamps ("the start") and
    finding the longest chain from that start. If the number of remaining
    elements is less than the longest chain found so far, the function can
    return early.

    Differences are calculated from the previous "confirmed" timestamp so that
    errors do not accumulate. This mimics the behavior of a transmitter that
    sleeps between transmissions instead of using an interrupt-based timer.

    Returns a list of indices of the longest chain found.
    """
    length = len(times)
    longest = 0
    longest_chain = []

    def find_chains_from(index: int, past_indices: list[int]) -> list[list[int]]:
        # `index` should be the last of `past_indices`
        assert past_indices[-1] == index
        diffs = times - times[index]
        max_diff = diffs[-1]
        for order in range(1, round(max_diff / expected_delta) + 1):
            candidates = np.abs(
                diffs - order * expected_delta) < tolerance * order
            cand_idxs = np.flatnonzero(candidates)
            # Prefer lower-order candidates (since the rest will be the next
            # candidate's responsibility)
            if len(cand_idxs):
                break
        else:
            # Nothing found
            return [past_indices]
        subchains = [find_chains_from(idx, past_indices + [idx])
                     for idx in cand_idxs]
        # Flatten once
        return [chain for sublist in subchains for chain in sublist]
    for start_idx in range(length):
        from_here_results = find_chains_from(start_idx, [start_idx])
        for chain in from_here_results:
            if len(chain) > longest:
                longest = len(chain)
                longest_chain = chain
        if length - start_idx < longest:
            # Early stop if the remaining timestamps are fewer than the longest
            break
    return longest_chain


ARGRE = re.compile(r"^(.*)_detect([0-9]+)-([0-9]+)$")

for p in sorted(glob.glob("*_detect*")):
    basename, reqchirp, synlen = ARGRE.match(p).groups()
    times = find_t(p)
    tdiffs = np.diff(times).astype(np.float64)/1e6
    filelen = np.fromfile(p[:-10] + ".sigmf-data", dtype=np.complex64).size
    sample_rate = json.load(
        open(p[:-10] + ".sigmf-meta"))["global"]["core:sample_rate"]
    elapsed = np.timedelta64(int(filelen / sample_rate * 1e6), "us")
    detected_elapsed = times[-1] - \
        times[0] if len(times) else np.timedelta64(0, "us")
    if detected_elapsed > elapsed:
        print(f"Error in {p}: {detected_elapsed.astype('timedelta64[s]')}")
    longest_chain = find_longest_chain(times, expected_delta, tolerance)
    if len(times):
        expected_n = int(np.ceil(elapsed / expected_delta))
        # Now there are two (largely equivalent) ways to find the detection rate:
        # 1. Find all the "holes", including the holes at the start and end
        # 2. Just find the expected N and the actual N and divide
        # Method 1:
        # longest_times = times[longest_chain]
        # nholes = (np.diff(longest_times)/expected_delta - 1).round().astype(int).sum()
        # start_nholes = round((longest_times[0] - times[0])/expected_delta)
        # end_nholes = round((times[-1] - longest_times[-1])/expected_delta)
        # nholes += start_nholes + end_nholes
        # rate1 = 1 - nholes / expected_n
        # Method 2:
        rate = len(longest_chain) / expected_n
        false_positives = len(times) - len(longest_chain)
        false_positive_rate = false_positives / len(times)
    else:
        rate = 0
        false_positives = 0
        false_positive_rate = 0
    tagline = f"({basename} R={reqchirp} L={synlen}):".ljust(40)
    print(
        f"{tagline}"
        f"expected N = {expected_n:>3}, "
        f"likely N = {len(longest_chain):>3}, "
        f"rate = {rate:>9.4%}, "
        f"FP = {false_positives:>3}, "
        f"FP rate = {false_positive_rate:>8.4%}, "
        f"elapsed = {elapsed}"
    )
