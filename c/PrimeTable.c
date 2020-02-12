/* Generate a CSV number table with LINES lines,
 * 2,3,5,7,11,... elements each line,
 * mark prime numbers with P postfix,
 * otherwise N postfix*/
/*
 * Sample:(four lines)
 * 1N, 2P
 * 3P, 4N, 5P
 * 6N, 7P, 8N, 9N, 10N
 * 11P, 12N, 13P, 14N, 15N, 16N, 17P
 */
/*
 *  PrimeTable.c
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

#include <stdio.h>
#include <stdlib.h>
#include <slib/math.h>

#define LINES lines /* Replace to a constant value if no VLA support*/
#define USE_NUMBERS /* Whether have the first row and the first column empty   \
                     */
#undef _NC          /* ncurses support */

#ifdef _NC
#include <ncurses.h>
#include <string.h>
#endif

int main()
{
    unsigned int lines, p = 1, countn = 0;
    FILE *fp;
#ifdef _NC
    int row, col;
    initscr();
    getmaxyx(stdscr, row, col);
#endif

#ifdef _NC
    {
        char mesg[] = "Type the amount of lines you need:",
             mesg2[] = "Prime table generator";
        mvprintw(1, (col - strlen(mesg2)) / 2, "%s", mesg2);
        mvprintw(row / 2, (col - strlen(mesg)) / 2, "%s", mesg);
        refresh();
    }
#else
    printf("Type the amount of lines you need:");
#endif
#ifdef _NC
    scanw("%u", &lines);
#else
    scanf("%u", &lines);
#endif
    unsigned int jmpt[LINES];
    if ((fp = fopen("output.csv", "w")) == NULL)
    {
#ifdef _NC
        endwin();
#endif
        fprintf(stderr, "Fopen call failed, please try again!");
        exit(1);
    }
    {
        unsigned int curp = 0, addp = 0, count = 0;
        for (; count < LINES; addp++)
        {
            if (slib_ispn(addp))
            {
                curp += addp;
                jmpt[count] = curp;
                count++;
            }
        }
    }
#ifdef USE_NUMBERS
    fprintf(fp, "\n,");
#endif
    for (; countn < LINES && p <= jmpt[LINES - 1]; p++)
    {
        if (slib_ispn(p))
            fprintf(fp, "%dP", p);
        else
            fprintf(fp, "%dN", p);
        if (jmpt[countn] == p)
        {
            fprintf(fp, "\n");
#ifdef USE_NUMBERS
            fprintf(fp, ",");
#endif
            countn++;
        }
        else
            fprintf(fp, ",");
    }

#ifdef _NC
    endwin();
#endif

    printf("\n\n\n\nGenerating done.\nPlease send out.csv to "
           "Numbers/Excel\nThen color cells ends with 'P'.\n\n\n");
    fclose(fp);
    return 0;
}
