/* Program to print prime numbers less than MAXPRIME with Sieve of Eratosthenes
 * algorithm */
/*
 * prime3.c
 * Copyright (C) 2018,2020 Zhang Maiyun <me@myzhangll.xyz>
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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
/* This number will be the number of bytes the process will use */
#define MAXPRIME 100000000UL

int main(void)
{
    unsigned long i, j, sq = sqrt(MAXPRIME);
    /* Reversed bool, true is false, false is true */
    bool *primes = calloc(MAXPRIME, sizeof(bool));
    if (!primes)
        return fprintf(stderr, "malloc failed\n"); /* 15 */

#pragma omp parallel for
    for (i = 2; i < sq; ++i)
        for (j = i * i; j < MAXPRIME; j += i)
            primes[j] = true;

    for (i = 2; i < MAXPRIME; ++i)
        if (primes[i] == false) /* is prime */
            printf("%lu\n", i);

    free(primes);
    return 0;
}
