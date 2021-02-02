#!/usr/bin/env python
"""Compute organic composition with combustion."""

import sys

M_O = 16.00
M_C = 12.01
M_H = 1.01


def compute(m_original: float, m_co2: float, m_h2o: float):
    """Compute composition of carbon, hydrogen, and oxygen."""
    m_o2 = m_co2 + m_h2o - m_original
    print(f"mass of oxygen: {m_o2}")
    n_o2 = m_o2 / (2 * M_O)
    print(f"moles of oxygen: {n_o2}")
    n_co2 = m_co2 / (M_C+2*M_O)
    print(f"moles of CO2: {n_co2}")
    n_h2o = m_h2o/(2*M_H+M_O)
    print(f"moles of water: {n_h2o}")
    print(f"moles of C: {n_co2}")
    print(f"moles of H: {2*n_h2o}")
    print(f"moles of O: {n_h2o+2*n_co2-2*n_o2}")
    return (n_co2, 2*n_h2o, n_h2o+2*n_co2-2*n_o2)


if __name__ == "__main__":
    compute(float(sys.argv[1]), float(sys.argv[2]), float(sys.argv[3]))
