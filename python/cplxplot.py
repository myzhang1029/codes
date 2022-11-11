#!/usr/bin/env python3
#
#  cplxplot.py
#
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
# pylint: disable=invalid-name

"""Complex plotter on a 4-D plane."""

from typing import Tuple

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.axes import Axes
from matplotlib.colors import Normalize
from matplotlib.figure import Figure


def cplxspace(rad: float, *, num: int = 50, polar: bool = True):
    """Create a complex grid.

    Parameters
    ----------
    rad : float
        Radius of both sides of the grid.
    num : int, optional
        Number of points per component. Default = 50.
    polar : bool, optional
        Whether to use polar coordinates. Default = True.

    Returns
    -------
    NDArray[shape=(num, num), dtype="complex128"]
        2-D array containing points.
    """
    if polar:
        r = np.linspace(0, rad, num=num).reshape(-1, 1)
        theta = np.pi * np.linspace(0, 2, num=num)
        return r * np.exp(theta * 1j)
    real = np.linspace(-rad, rad, num=num).reshape(-1, 1)
    imag = np.linspace(-rad, rad, num=num) * 1j
    return real + imag


def cplxplot(xy, func) -> Tuple[Figure, Axes]:
    """Plot a complex function w.r.t a complex variable on a 4-D plane.

    Parameters
    ----------
    xy : NDArray[dim=2]
        Complex independent variable.
    cfun : NDArray[dim=2]
        Complex dependent variable with the same shape as `xy`.

    Returns
    -------
    tuple[Figure, Axes]
        Plotted 3-D figure.
    """
    realpart = np.real(xy)
    imagpart = np.imag(xy)
    # Function value real part
    realval = np.real(func)
    # Fourth dimension as color
    ival = np.imag(func)
    # Normalize to [0, 1]
    norm = Normalize(vmin=np.min(ival), vmax=np.max(ival))
    imagval = norm(ival)
    # Create fugure
    fig = plt.figure()
    ax = plt.axes(projection="3d")
    # Function surface
    colors = plt.cm.hsv(imagval)
    # pylint: disable=no-member
    ax.plot_surface(realpart, imagpart, realval, facecolors=colors)
    # Imaginary legend
    cbarmap = plt.cm.ScalarMappable(norm=norm, cmap=plt.cm.hsv)
    plt.colorbar(cbarmap, label="Im(z)")
    ax.set_xlabel("Re")
    ax.set_ylabel("Im")
    ax.set_zlabel("Re(z)")
    return fig, ax


if __name__ == "__main__":
    z = cplxspace(10)
    _, axis = cplxplot(z, z ** 2 - 1)
    axis.set_title("Complex Plot for $z = x^2 - 1$")
    plt.show(block=True)
