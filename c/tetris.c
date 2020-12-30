/*
 * Tetris by Zhang Maiyun<myzhang1029@hotmail.com>
 * Screen at least 27rows@15columns
 * Version: Compileable-Runable-Useless v0.1.0
 */
/*
 *  tetris.c
 *  Copyright (C) 2017-present Zhang Maiyun <myzhang1029@hotmail.com>
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

#include <slib.h>
#include <slib/fileopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

/* The very traditional size of the map */
#define ROW 20
#define COL 10

/* Screen roller */
#ifdef _WIN32
#define clscr() system("cls")
#else
#define clscr() system("clear")
#endif

#define oneblock(color) colorprintf(blue, color, "  ")
#define twoblock(color) colorprintf(blue, color, "	")
#define blkblock() printf("  ")
#define tbkblock() printf("	")
#define ret() printf("\n")

#define ONE_TYPES (shape == 6)
#define TWO_TYPES (shape == 1 || shape == 2 || shape == 5)
#define FOUR_TYPES (shape == 3 || shape == 4 || shape == 7)

/*This should be greater than ROW and lesser than max of int, or a negative
 * value*/
#define STAGE_FLOOR -1

/*Level: seconds delay before dropping*/
/*TODO: add conf file or interactive and/or take arguments*/
#define LVL 1

/*
Basic values for HUMAN

SHAPE_S_SIZE1 3  //_00
SHAPE_S_SIZE2 2  //00_
SHAPE_Z_SIZE1 3  //00_
SHAPE_Z_SIZE2 2  //_00
SHAPE_L_SIZE1 2  //__0
SHAPE_L_SIZE2 4  //000
SHAPE_J_SIZE1 2  //000
SHAPE_J_SIZE2 4  //__0
SHAPE_I_SIZE1 1  //0000
SHAPE_I_SIZE2 4  //____
SHAPE_O_SIZE1 2  //00__
SHAPE_O_SIZE2 2  //00__
SHAPE_T_SIZE1 3  //000_
SHAPE_T_SIZE2 2  //_0__

All shapes are turned clockwise

enum
{
    s_init,s_1,
    z_init,z_1,
    l_init,l_1,l_2,l_3,
    j_init,j_1,j_2,j_3,
    i_init,i_1,
    o_init,
    t_init,t_1,t_2,t_3
}
*/

void refreshscr();
void displays(int shape, int pnt, int stage, int pos);
void deletes(int shape, int pnt, int stage, int pos);
int getchsleep(int seconds);
bool atfloor(int shape, int pnt, int stage, int pos);

int scrbuf[ROW][COL] = {{0}};

/* TODO: Game over/win */

int main()
{
    int n = 0, type, pos, stage, pnt, next, score = 0, level = LVL;
#if PLAT
    COORD sz = GetLargestConsoleWindowSize(GetStdHandle(STD_OUTPUT_HANDLE));
    if (sz.X < 15 || sz.Y < 27)
    {
        fprintf(stderr, "Window needs at least 27 rows and 15 columns\n");
        exit(1);
    }
#else
    struct winsize sz;
    ioctl(STDIN_FILENO, TIOCGWINSZ, &sz);
    if (sz.ws_col < 15 || sz.ws_row < 27)
    {
        fprintf(stderr, "Window needs at least 27 rows and 15 columns\n");
        exit(1);
    }
#endif
    srand(time(NULL));
    type = 1 + rand() % 7;
    while (1)
    {
        next = 1 + rand() % 7;
        pos = COL / 2;
        stage = 1;
        pnt = 1;
        n++;
        displays(type, pnt, stage, pos);
        refreshscr(next, score, level);
        while (!atfloor(type, pnt, stage, pos))
        {
            switch (getchsleep(LVL))
            {
                case 'w':
                    deletes(type, pnt, stage, pos);
                    displays(type, ++pnt, ++stage, pos);
                    break;
                case 's':
                    deletes(type, pnt, stage, pos);
                    displays(type, pnt, STAGE_FLOOR, pos);
                    break;
                case 'a':
                    deletes(type, pnt, stage, pos);
                    displays(type, pnt, ++stage, --pos);
                    break;
                case 'd':
                    deletes(type, pnt, stage, pos);
                    displays(type, pnt, ++stage, ++pos);
                    break;
                case 'q':
                    exit(0);
                default:
                    clscr();
                    printf("Press w, a, s or d");
                    getch();
                    refreshscr(next, score, level);
            }
            refreshscr(next, score, level);
        }
        type = next;
    }
    return 0;
}

/*
 * refreshscr: Write scrbuf to screen with score, level and next block
 * information Finished but not tested Parameters: next: Next block number
 * score: Current score level: Current level
 */
void refreshscr(int next, int score, int level)
{
    int row = 0, col = 0;
    clscr();
    for (; row < ROW; ++row)
    {
        for (; col < COL; ++col)
        {
            if (scrbuf[row][col] == 0)
                oneblock(blue);
            else
                blkblock();
        }
        printf("\n");
    }
    printf("Now score: %d\n", score);
    printf("Now level: %d\n", level);
    printf("Next:\n");
    switch (next)
    {
        case 1:
            tbkblock();
            twoblock(red);
            ret();
            twoblock(red);
            ret();
            break;
        case 2:
            twoblock(green);
            ret();
            tbkblock();
            twoblock(green);
            ret();
            break;
        case 3:
            tbkblock();
            oneblock(blue);
            ret();
            twoblock(blue);
            oneblock(blue);
            ret();
            break;
        case 4:
            twoblock(blue);
            oneblock(blue);
            ret();
            tbkblock();
            oneblock(blue);
            ret();
            break;
        case 5:
            twoblock(yellow);
            twoblock(yellow);
            ret();
            ret();
            break;
        case 6:
            twoblock(magenta);
            ret();
            twoblock(magenta);
            ret();
            break;
        case 7:
            twoblock(cyan);
            oneblock(cyan);
            ret();
            blkblock();
            oneblock(cyan);
            break;
        default:
            fprintf(stderr,
                    "Check your source code"); /*it should be impossible*/
            exit(1);
    }
}

/*
 * getchsleep: Get a char or sleep some seconds
 * Finished but not tested
 */
int getchsleep(int seconds)
{
    clock_t t = clock();
    int ret = -1;
    while (1)
    {
        if (kbhit())
        {
            ret = getch();
            break;
        }
        if ((int)((clock() - t) / CLOCKS_PER_SEC) >= seconds)
            break;
    }
    return ret;
}

void displays(int shape, int pnt, int stage, int pos) {}
void deletes(int shape, int pnt, int stage, int pos) {}
bool atfloor(int shape, int pnt, int stage, int pos) { return 0; }
