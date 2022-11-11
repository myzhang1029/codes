/* Program to print prime numbers between MINPRIME and MAXPRIME with
 * Sieve of Eratosthenes algorithm */
/* This involves the same amount of calculation as using prime3, but less memory */
/*
 * prime4.c
 * Copyright (C) 2020 Zhang Maiyun <me@myzhangll.xyz>
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

/* Inclusive */
#define MINPRIME 10000000000ULL
/* Exclusive */
#define MAXPRIME 10000100000ULL

int main(void)
{
    unsigned long long i, j, sq = (unsigned long long) sqrt(MAXPRIME);
    /* Reversed bool, true is false, false is true */
    bool *primes = calloc(MAXPRIME - MINPRIME, sizeof(bool));
    if (!primes)
        return fprintf(stderr, "malloc failed\n"); /* 15 */
    if (MAXPRIME - MINPRIME <= 0)
        return 0;

#pragma omp parallel for
    for (i = 2; i < sq; ++i)
    {
        for (j = i * i; j < MAXPRIME; j += i)
        {
            if (j < MINPRIME)
                continue;
            primes[j - MINPRIME] = true;
        }
    }

    for (i = 0; i < MAXPRIME - MINPRIME; ++i)
        if (primes[i] == false) /* is prime */
            printf("%llu\n", i + MINPRIME);

    free(primes);
    return 0;
}
