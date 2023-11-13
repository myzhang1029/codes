/*
 *  uprec.c - Replacement for tuptime or uptimed for my own needs.
 *
 *  Copyright (C) 2021 Zhang Maiyun <me@maiyun.me>
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

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Maximum base-16 length of `time_t`, including a sign but not NUL
 *   ceil(log_16(2 ** (sizeof * 8))) + 2
 * = ceil(sizeof * 8 * 0.25) + 2
 * = 2 * sizeof + 2
 */
#define MAX_B16_TIME_LEN (2 * sizeof(time_t) + 1)
/* Plus a separator, newline, and NUL */
#define RECORD_SIZE (2 * MAX_B16_TIME_LEN + 3)
/* Separator */
const char SEP = '|';
/* Format of a date */
#define DATE_LEN 25
const char *DATE_FMT = "%F %T %z";
/* Max difference between boot times to consider them as identical.
 * Usually the only source of error is rounding error, so 1 should be enough
 */
#define TIME_EPSILON 1

#ifdef __APPLE__
#include <sys/sysctl.h>

/* Get epoch of the last boot */
time_t get_boot_time(void)
{
    struct timeval boottime;
    size_t len = sizeof(boottime);
    int mib[2] = {CTL_KERN, KERN_BOOTTIME};
    if (sysctl(mib, 2, &boottime, &len, NULL, 0) < 0)
        return 0;
    return boottime.tv_sec;
}
#elif defined(linux)
#include <sys/sysinfo.h>

/* Get epoch of the last boot */
time_t get_boot_time(void)
{
    struct sysinfo sinfo;
    if (sysinfo(&sinfo) < 0)
        return 0;
    return time(NULL) - sinfo.uptime;
}
#elif defined(_WIN32)
#include <sysinfoapi.h>

/* Get epoch of the last boot */
time_t get_boot_time(void) { return time(NULL) - GetTickCount64() / 1000; }
#endif

/* Create or update the entry in the database.
 *
 * This function is not concurrent-safe.
 * Database format: <UPTIME><SEP><UPSINCE>
 * - <UPTIME> is the time for which the system is up in seconds.
 * - <SEP> Defined.
 * - <UPSINCE> is the epoch timestamp of that boot.
 * Both fields has a fixed column width to ensure a complete rewrite when
 * they are updated.
 * Those seconds are stored in hex because it is easier to calculate their
 * expected length.
 *
 * `db_filename`: File name of the database.
 *
 * Return 0 on success; 1 on OS failure; 2 on other failures.
 */
int update(const char *db_filename)
{
    FILE *db;
    char linebuf[RECORD_SIZE];
    /* Uptime */
    time_t diff;
    /* Real boot time */
    time_t boot_time = 0;
    /* Position of the last newline */
    fpos_t last_newline;

    /* Get the boot time of the running boot */
    boot_time = get_boot_time();
    if (boot_time == 0)
        return 2;

    /* First try read write mode */
    errno = 0;
    db = fopen(db_filename, "r+");
    if (db == NULL && errno == ENOENT)
    {
        /* Then create it */
        db = fopen(db_filename, "a+");
        rewind(db);
    }
    if (db == NULL)
        /* Fail */
        return 1;

    /* Initialize this variable with the start of the file */
    if (fgetpos(db, &last_newline) != 0)
        return 1;

    /* Try to see if a record is about the current boot */
    while (fgets(linebuf, RECORD_SIZE, db) != NULL)
    {
        char *separator_pos;
        long last_uptime = strtol(linebuf, &separator_pos, 16);
        time_t last_upsince = (time_t)strtol(separator_pos + 1, NULL, 16);
        if (labs(last_upsince - boot_time) <= TIME_EPSILON ||
            last_upsince + last_uptime >= boot_time)
        {
            /* In the same boot, or
             * the system seems to have booted before it was last shutdown:
             * assuming a time sync problem in the previous record.
             *
             * Let's rewrite the current entry.
             */
            if (fsetpos(db, &last_newline) != 0)
                return 1;
            break;
        }
        /* Save this newline */
        if (fgetpos(db, &last_newline) != 0)
            return 1;
    }

    /* Compute uptime */
    diff = time(NULL) - boot_time;
    /* Write output */
    fprintf(db, "%*" PRIxMAX "%c%*" PRIxMAX "\n", (int)MAX_B16_TIME_LEN,
            (intmax_t)diff, SEP, (int)MAX_B16_TIME_LEN, (intmax_t)boot_time);
    fclose(db);
    return 0;
}

/* Pretty-print the database for human */
int show(const char *db_filename)
{
    FILE *db;
    char linebuf[RECORD_SIZE];
    unsigned int idx = 0;

    db = fopen(db_filename, "r");
    if (db == NULL)
        /* Fail */
        return 1;

    while (fgets(linebuf, RECORD_SIZE, db) != NULL)
    {
        char *separator_pos;
        /* +1 for NUL */
        char upbuf[DATE_LEN + 1] = {0}, downbuf[DATE_LEN + 1] = {0};
        long uptime = strtol(linebuf, &separator_pos, 16);
        time_t upsince = (time_t)strtol(separator_pos + 1, NULL, 16);
        /* BEWARE: localtime() overwrites the buffer from previous calls */
        struct tm *buf;
        /* Print up since */
        buf = localtime(&upsince);
        strftime(upbuf, DATE_LEN, DATE_FMT, buf);
        /* Now print down since */
        upsince += uptime;
        buf = localtime(&upsince);
        strftime(downbuf, DATE_LEN, DATE_FMT, buf);

        /* Keep this less than 79 chars */
        printf("%d: Up from %s to %s, %ld sec\n", idx, upbuf, downbuf, uptime);
        ++idx;
    }
    /* TODO: STATs */
    fclose(db);
    return 0;
}

int usage(const char *name)
{
    printf("Usage: %s <-u|-s> <DBFILE>\n", name);
    puts("Options:");
    puts("\t-s: show uptime records.");
    puts("\t-u: update uptime records.");
    puts("");
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 3)
        return usage(argv[0]);
    if (strcmp(argv[1], "-s") == 0)
    {
        int stat = show(argv[2]);
        if (stat != 0)
            fprintf(stderr, "%s\n", strerror(errno));
        return stat;
    }
    if (strcmp(argv[1], "-u") == 0)
    {
        int stat = update(argv[2]);
        if (stat != 0)
            fprintf(stderr, "%s\n", strerror(errno));
        return stat;
    }
    return usage(argv[0]);
}
