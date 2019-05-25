/* Program to print prime numbers less than MAXPRIME with Sieve of Eratosthenes algorithm */
/*
 * prime3.c
 * Copyright (C) 2018 Zhang Maiyun <myzhang1029@163.com>
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
	unsigned long i, j;
	bool *primes = malloc(MAXPRIME);
	if (primes == NULL)
		exit((fprintf(stderr, "malloc failed\n"), 1));
	for (i = 2; i < MAXPRIME; i++)
		primes[i] = true;
	primes[0] = primes[1] = false;

#pragma omp for
	for (i = 2; i < sqrt(MAXPRIME); i++)
		for (j = i * i; j < MAXPRIME; j += i)
			primes[j] = false;

	for (i = 0; i < MAXPRIME; i++)
		if (primes[i] == true)
			printf("%lu\n", i);

	free(primes);
	return 0;
}
