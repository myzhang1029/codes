/* Program to count prime numbers greater than or equal to MINPRIME, less than
 * MAXPRIME with THREADS threads */
/* Prepared for p2gen.sh */
/*
 * prime2.c
 * Copyright (C) 2017-2018 Zhang Maiyun <me@maiyun.me>
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

#include <inttypes.h>
#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define MINPRIME @p2gen_min @
#define MAXPRIME @p2gen_max @
#define PRINT

#define THREADS 4

_Atomic uint32_t count = 0;

void *thrd_fct(void *arg)
{
    uint64_t min =
                 MINPRIME + ((uint64_t)arg) * ((MAXPRIME - MINPRIME) / THREADS),
             max = MINPRIME +
                   ((uint64_t)arg + 1L) * ((MAXPRIME - MINPRIME) / THREADS) - 1;
    for (; min <= max; ++min)
    {
        uint64_t k = sqrt(min), i = 3;
        bool add = true;

        if (!(min == 1 || (!(min & 1) && min != 2)))
        {
            for (; i <= k; i++)
                if (min % i == 0)
                {
                    add = false;
                    break;
                }
        }
        else
            add = false;
        if (add)
        {
#ifdef PRINT
            printf("%" PRIu64 "\n", min);
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
    printf("\n\033[31m%" PRIu32 "\033[0m\n", count);
    return 0;
}
