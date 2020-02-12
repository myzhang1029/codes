/* Print numbers that their nth root, n_2th root,... are all an integer
 * like 4096, 262144, 729 for 2 and 3
 */
/*
 *  cnsroot.c
 *  Copyright (C) 2017 Zhang Maiyun <myzhang1029@163.com>
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
#include <stdlib.h>
#include <slib/getopt.h>
#include <slib/math.h>
int main(int argc, char **argv)
{
    struct optionGS options[] = {{"base-max", 1, NULL, 'b'},
                                 {"power-max", 1, NULL, 'p'},
                                 {"add-multiple", 1, NULL, 'm'},
                                 {"help", 0, NULL, 'h'},
                                 {NULL, 0, NULL, 0}};
    char *sopts = ":b:p:m:h";
    int opt;
    unsigned long multiple = 1, bmax = 3, pmax = 3;
    while ((opt = getopt_longGS(argc, argv, sopts, options, NULL)) != -1)
    {
        switch (opt)
        {
            case 'b':
                bmax = atol(optargGS);
                break;
            case 'p':
                pmax = atol(optargGS);
                break;
            case 'm':
                multiple = slib_lcm(multiple, atol(optargGS));
                break;
            case 'h':
                printf("Options:\n"
                       "-b, --base-max ARG: maximum of the base\n"
                       "-p, --power-max ARG: maximum of power/multiple\n"
                       "-m, --add-multiple ARG: add a multiple\n"
                       "-h, --help: show this\n");
                exit(0);
            case ':':
                printf("missing arg for -%c\n", optoptGS);
                exit(1);
            default:
                printf("not an arg: -%c\n", optoptGS);
                exit(1);
        }
    }
    unsigned long base, power;
    for (base = 0; base <= bmax; ++base)
        for (power = 0; power <= pmax; ++power)
            printf("%ld\n", (long)pow(base, multiple * power));
    return 0;
}
