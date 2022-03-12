#!/usr/bin/env python3
#  wave_interference.py
#  Copyright (C) 2022 Zhang Maiyun <myzhang1029@hotmail.com>
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

"""Plot 2-D point wave interference."""

from typing import Tuple

import matplotlib
import numpy as np
from matplotlib import pyplot as plt


def plot_interference(
    wl: float,
    dis: float,
    *,
    phase_diff: float = 0.0,
    x_width: float = 0.0,
    y_width: float = 0.0,
    n_points: int = 250
) -> Tuple[matplotlib.contour.ContourSet, matplotlib.colorbar.Colorbar]:
    """Plot 2-D point interference pattern.

    Parameters
    ---------
    wl : float
        Wavelength of the two waves.
    dis : float
        Distance between the two sources.
    phase_diff : float
        Phase difference between the two sources in radians.
    x_width : float, optional
        Length of the horizontal axis. Defalus to `y_width`.
    y_width : float, optional
        Length of the vertical axis.
        Defaults to 10 times the maximum of `wl` and `dis`.
    n_points : int, optional
        Number of points along both axes. Defaults to 250.
    """
    if y_width == 0.0:
        y_width = 10 * max(wl, dis)
    if x_width == 0.0:
        x_width = y_width
    y = np.linspace(-y_width / 2, y_width / 2, n_points)
    x = np.linspace(-x_width / 2, x_width / 2, n_points)
    [xx, yy] = np.meshgrid(x, y)
    zh = np.sin(np.sqrt(xx**2 + (yy - dis / 2) ** 2) * 2 * np.pi / wl)
    zl = np.sin(np.sqrt(xx**2 + (yy + dis / 2) ** 2)
                * 2 * np.pi / wl - phase_diff)
    contour = plt.contour(x, y, zh + zl,
                          levels=512,
                          cmap=plt.cm.PiYG_r)
    cbar = plt.colorbar(contour, label="Amplitude")
    return contour, cbar


if __name__ == "__main__":
    plot_interference(10, 10, n_points=200)
    plt.show()
