/* Solve a quadratic equation */
/*
 *  autoip.c
 *  Copyright (C) 2017, 2018 Zhang Maiyun <myzhang1029@163.com>
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

#include <math.h>
#include <stdio.h>

int main()
{
    double a, b, c, delta, x1, x2, real, imag;

    printf("输入三个系数 a, b, c: ");
    scanf("%lf %lf %lf", &a, &b, &c);

    delta = b * b - 4 * a * c;

    /* 两个不相等的实数根 */
    if (delta > 0)
    {
        x1 = (-b + sqrt(delta)) / (2 * a);
        x2 = (-b - sqrt(delta)) / (2 * a);
        printf("x1 = %.10lf, x2 = %.10lf\n", x1, x2);
    }
    /* 两个相等的实数根 */
    else if (delta == 0)
    {
        x1 = -b / (2 * a);
        printf("x1 = x2 = %.10lf\n", x1);
    }
    /* 复数根 */
    else
    {
        real = -b / (2 * a);
        imag = sqrt(-delta) / (2 * a);
        printf("x1 = %.10lf+%.10lfi, x2 = %.10f-%.10fi\n", real, imag, real,
               imag);
    }
    return 0;
}
