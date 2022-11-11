/* third.c - Calculate n/3 by recursively calculate n/4,2n/4 */
/*
 *  third.c
 *  Copyright (C) 2017 Zhang Maiyun <me@myzhangll.xyz>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
int main(int d, char **e)
{
    double a, b, ebp, esp;
    int f = 32;
    ebp = 0.0;
    esp = 9000.0;
    while (f--)
    {
        a = esp;
        ebp = ebp + (b = (a - ebp) / 4.0);
        esp = ebp + b;
    }
    printf("%.10f\n", ebp);
    return 0;
}
