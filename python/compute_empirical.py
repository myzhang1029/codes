#!/usr/bin/env python
"""Compute organic composition with combustion."""

from fractions import Fraction as Fr
import sys

M_O = Fr("16.00")
M_C = Fr("12.01")
M_H = Fr("1.01")


def compute(m_original: Fr, m_co2: Fr, m_h2o: Fr):
    """Compute composition of carbon, hydrogen, and oxygen."""
    m_o2 = m_co2 + m_h2o - m_original
    print(f"mass of oxygen: {float(m_o2)}")
    n_o2 = m_o2 / (2 * M_O)
    print(f"moles of oxygen: {float(n_o2)}")
    n_co2 = m_co2 / (M_C+2*M_O)
    print(f"moles of CO2: {float(n_co2)}")
    n_h2o = m_h2o/(2*M_H+M_O)
    print(f"moles of water: {float(n_h2o)}")
    print(f"moles of C: {float(n_co2)}")
    print(f"moles of H: {float(2*n_h2o)}")
    print(f"moles of O: {float(n_h2o+2*n_co2-2*n_o2)}")
    return (n_co2, 2*n_h2o, n_h2o+2*n_co2-2*n_o2)


if __name__ == "__main__":
    compute(Fr(sys.argv[1]), Fr(sys.argv[2]), Fr(sys.argv[3]))
