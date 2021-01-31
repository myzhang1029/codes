#!/usr/bin/env python3
#
#  searchw_py.py
#
#  Copyright (C) 2020 Zhang Maiyun <myzhang1029@hotmail.com>
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

"""Search /usr/share/dict/words to brute-force word puzzles."""

import sys


def searchw(listd, lista):
    """Search for a word in a list.

    Args:
        listd: the word separated alphabet by alphabet, None for any.
        lista: all words.

    Return:
        A generator of valid words.
    """
    for w in lista:
        if len(w) != len(listd):
            continue
        for n, a in enumerate(listd):
            if a != w[n] and a is not None:
                break
        else:
            yield w


def search():
    l = [w.strip().lower() for w in open("/usr/share/dict/words").readlines()]
    l5w23s = [w for w in l if len(w) == 5 and w[2] == w[3]]

    o, s, i, e, q, h, a, t, n, k, u, m, y = (None,)*13
    for w in l5w23s:
        o = w[0]
        s = w[1]
        i = w[2]
        e = w[4]
        # Test inequality
        if len(set((o, s, i, e))) != 4:
            continue
        for w1 in searchw([e, None, e, None, s], l):
            q = w1[1]
            h = w1[3]
            # Test inequality
            if len(set((o, s, i, e, q, h))) != 6:
                continue
            for w2 in searchw([o, None, None, i, e], l):
                a = w2[1]
                t = w2[2]
                # Test inequality
                if len(set((o, s, i, e, q, h, a, t))) != 8:
                    continue
                for w3 in searchw([e, None, t, None], l):
                    n = w3[1]
                    k = w3[3]
                    # Test inequality
                    if len(set((o, s, i, e, q, h, a, t, n, k))) != 10:
                        continue
                    for w4 in searchw([None, e, a, None], l):
                        u = w4[0]
                        m = w4[3]
                        # Test inequality
                        if len(set((o, s, i, e, q, h, a, t, n, k, u, m))) != 12:
                            continue
                        for w5 in searchw([u, t, None, o], l):
                            y = w5[2]
                            # Test inequality
                            if len(set((o, s, i, e, q, h, a, t, n, k, u, m, y))) != 13:
                                continue
                            sent = w[0].upper()+w[1:]+' '+w1+' ' + \
                                w2+' '+w3+' '+w4+' '+w5
                            print(sent, sep='\n')


if __name__ == "__main__":
    search()
