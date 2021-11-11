/* Program to bootstrap prime list between MINPRIME and MAXPRIME with
 * Sieve of Eratosthenes algorithm */
/* Reads a smaller prime list from stdin */
/*
 * prime5.c
 * Copyright (C) 2020 Zhang Maiyun <myzhang1029@hotmail.com>
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
#define MINPRIME 0ULL
/* Exclusive */
#define MAXPRIME 1000ULL
/* base-10 length of MAXPRIME (plus one if its not a power of ten) */
#define LENGTH 4
int main(void)
{
    unsigned long long
        /* Prime to multiply */
        i,
        /* Number to exclude */
        j = 0,
        /* sqrt(max) */
        sq = (unsigned long long) sqrt(MAXPRIME);
    /* Line read */
    char *line = malloc(LENGTH);
    /* Reversed bool, true is false, false is true */
    bool *primes = calloc(MAXPRIME - MINPRIME, sizeof(bool));
    if (!primes)
        return fprintf(stderr, "malloc failed\n"); /* 15 */
    /* No number here */
    if (MAXPRIME - MINPRIME <= 0)
        return 0;
    /* Exclude the blind ones */
    if (MINPRIME < 2)
        for (i = 0; i < 2 - MINPRIME; ++i)
            primes[i] = true;

    while (1)
    {
        fgets(line, LENGTH, stdin);
        if (feof(stdin))
        {
            fprintf(stderr, "Unexpected EOF\n");
            free(primes);
            free(line);
            return 1;
        }
        i = atoll(line);
        /* Print to stderr */
        fprintf(stderr, "%llu\n", i);
#pragma omp parallel for
        for (j = i * i; j < MAXPRIME; j += i)
        {
            if (j < MINPRIME)
                continue;
            primes[j - MINPRIME] = true;
        }
	/* Put this code here so that no extra line is read */
        if (i >= sq)
            break;
    }

    for (i = 0; i < MAXPRIME - MINPRIME; ++i)
        if (primes[i] == false) /* is prime */
            printf("%llu\n", i + MINPRIME);

    free(line);
    free(primes);
    return 0;
}
