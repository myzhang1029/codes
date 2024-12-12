# -*- coding: utf-8 -*-
#
#  expfit_guess.py
#
#  Copyright (C) 2024 Zhang Maiyun <maz005@ucsd.edu>
#   As a project developed during Maiyun's employment at the University of
#   California, The University of California may have claims to this work.
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

"""Make a guess for the parameters of an exponential decay function."""

import numpy as np
import numpy.typing as npt


def make_expfit_guess(
    t_input: npt.ArrayLike, v_input: npt.ArrayLike
) -> tuple[float, float, float, float, float, float]:
    """Make a guess for the parameters of an exponential fit of the form
    v(t) = amplitude * exp(-t / tau) + asymptote

    Parameters
    ----------
    t_input : array_like
        Time values.
    v_input : array_like
        Voltage values.

    Returns
    -------
    amplitude : float
        Guess for v(t=0) - v(infinity).
    tau : float
        Guess for the time constant.
    asymptote : float
        Guess for v(infinity).
    amplitude_ew : float
        Estimate of the error in the amplitude guess.
    tau_ew : float
        Estimate of the error in the tau guess.
    asymptote_ew : float
        Estimate of the error in the asymptote guess.

    Notes
    -----
    This function estimates the asymptote of the exponential decay by taking the
    average of the last five points. The difference between this average and the
    average of the five points immediately before that is taken as the error in
    the asymptote guess.

    The amplitude is estimated as the difference between the first point and the
    asymptote guess, with the error in the amplitude guess being twice the error
    in the asymptote guess.

    The time constant is estimated by finding the longest continuous window of
    points where the voltage is about the value that corresponds to one time
    constant (amplitude / e + asymptote), taking into account the errors in the
    amplitude and asymptote guesses.
    The time constant guess is then taken as the time value in this window that
    is closest to this value, with the error in the time constant guess being
    the maximum deviation from this value in the chosen window.

    This method should work well for data with a clear exponential decay,
    non-pathological noise, and a sufficient number of points.
    """
    # First cast to NumPy arrays because our indexing don't work with lists or pandas Series
    t = np.asanyarray(t_input)
    v = np.asanyarray(v_input)
    # Let's hope that the inputs have the same non-zero length
    if not len(t) or not len(v) or len(t) != len(v):
        raise ValueError("Time and voltage arrays must have the same non-zero length")
    # Estimate the asymptote of the exponential decay
    asymptote = np.mean(v[-5:])
    ahead = v[-10:-5]
    # Rare case that there are not enough points
    if len(ahead) == 0:
        ahead = v[0:1]
    asymptote_ew = np.abs(np.mean(ahead) - asymptote)
    # Estimate the amplitude
    amplitude = v[0] - asymptote
    amplitude_ew = asymptote_ew * 2
    # Estimate the time constant
    INVE = np.exp(-1)
    # The v value that corresponds to one time constant is amplitude / e + asymptote
    v_at_tau_good = amplitude * INVE + asymptote
    v_at_tau_3 = amplitude / 3 + asymptote
    v_at_tau_unc = np.abs(v_at_tau_good - v_at_tau_3) + \
        amplitude_ew * INVE + asymptote_ew
    indices_in_window = np.nonzero((v_at_tau_good - v_at_tau_unc < v) &
                                   (v < v_at_tau_good + v_at_tau_unc))[0]
    # Find the biggest window of times where the voltage is within the window
    discontinuities = np.nonzero(np.diff(indices_in_window) != 1)[0]
    discontinuities = np.concatenate(
        ([0], discontinuities, [len(indices_in_window)]))
    # Use the biggest window
    group_lengths = np.diff(discontinuities)
    max_group = np.argmax(group_lengths)
    start_didx = discontinuities[max_group]
    end_didx = discontinuities[max_group + 1]
    best_group_indices = indices_in_window[start_didx:end_didx]
    # Return the closest time to one_time_constant_value_good plus minus edge
    voltages_in_window = v[best_group_indices]
    if not len(voltages_in_window):
        # There are no points in the window
        # Hopefully we can get away with choosing the closest point
        # If there is significant noise, this will be a terrible guess
        # To somewhat account for the possibility of a very noisy signal, take
        # the next two closest points and use the maximum time difference
        # as the error in the guess
        closest_indices = np.argsort(np.abs(v - v_at_tau_good))[0:3]
        tau = t[closest_indices[0]]
        tau_ew = np.max(np.abs(t[closest_indices] - tau))
    else:
        closest_index = np.argmin(np.abs(voltages_in_window - v_at_tau_good))
        tau = t[best_group_indices][closest_index]
        tau_ew = np.max((t[best_group_indices[-1]] - tau,
                         tau - t[best_group_indices[0]]))
    return amplitude, tau, asymptote, amplitude_ew, tau_ew, asymptote_ew
