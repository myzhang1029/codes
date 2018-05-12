/* Program to count prime numbers greater than MINPRIME, less than MAXPRIME with THREADS threads */
/*
 * prime2.c
 * Copyright (C) 2017 Zhang Maiyun <myzhang1029@163.com>
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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

#define MINPRIME 100000000
#define MAXPRIME 1000000000

#define THREADS 4

atomic_int count = 0;

void *thrd_fct(void *arg)
{
	unsigned long long min = MINPRIME + ((unsigned long long)arg) * ((MAXPRIME - MINPRIME) / THREADS),
			   max = MINPRIME + ((unsigned long long)arg + 1L) * ((MAXPRIME - MINPRIME) / THREADS) - 1;
	for (; min < max; ++min)
	{
		unsigned long long k = sqrtl(min), i = 3;
		int add = 1;

		if (!(min == 1 || (!(min & 1) && min != 2)))
		{
			for (; i <= k; i++)
				if (min % i == 0)
				{
					add = 0;
					break;
				}
		}
		else
			add = 0;
		if (add)
		{
#ifdef PRINT
			printf("%llu\n", min);
#endif
			++count;
		}
	}
	return NULL;
}

int main(void)
{
	int i;
	pthread_t threads[THREADS];
	for (i = 0; i < THREADS; ++i)
		pthread_create(&(threads[i]), NULL, thrd_fct, (void *)i);
	for (i = 0; i < THREADS; ++i)
		pthread_join(threads[i], NULL);
	printf("\n\033[31m%d\033[0m\n", count);
	return 0;
}
