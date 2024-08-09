#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sun May 26 13:39:42 2024

@author: zmy
"""

import math

def trig(θ: float, N: int = 10000) -> (float, float):
    """Calculates (cos θ, sin θ)."""
    # IEEE754 Product[Cos[ArcTan[2^-n]], {n, 0, \[Infinity]}]
    x = 6.0725293500888122277814318295E-1
    y = 0
    φ = θ
    n = 0
    ss = []
    while n < N and φ:
        d = φ/abs(φ) if φ else 0
        ch = 2 ** -n
        x1 = x - d * y * ch
        y1 = y + d * x * ch
        x,y = x1, y1
        φ = φ - d * math.atan(ch)
        n += 1
    return x,y,n

print(trig(math.pi/3))
