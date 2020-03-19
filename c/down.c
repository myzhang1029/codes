/* Multi thread map tiles downloader using OpenMP and libcurl */
/* Parameters:
 * MAXZOOM: maximum zoom level
 * MINZOOM: minimum zoom level
 * BASEDIR: directory for the tiles
 * URLBASE: the url without /{z}/{x}/{y}.png part
 * URLARGS: GET arguments for things like apikey
 * USRAGNT: User-Agent
 * Maximum zoom level is 21
 * Only URLs like http://example.com/{z}/{x}/{y}.png are supported
 */
/*
 *  down.c
 *  Copyright (C) 2018 Zhang Maiyun <myzhang1029@hotmail.com>
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

#include <curl/curl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXZOOM 1
#define MINZOOM 0
#define BASEDIR "map"
#define URLBASE "https://tile.com"
#define URLARGS "?apikey=123456789abcdef"
#define USRAGNT                                                                \
    "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:58.0) Gecko/20100101 "         \
    "Firefox/58.0"

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

int main(void)
{
    long long maxtile;
    char *URI = malloc(1 + strlen(URLBASE "/XX/XXXXXXX/"
                                          "XXXXXXX.png" URLARGS));
    char *DEST =
        malloc(sizeof(char) * (1 + strlen(BASEDIR "/XX/XXXXXXX/XXXXXXX.png")));
    char *firstdir = malloc(1 + strlen(BASEDIR "/XX"));
    char *secdir = malloc(1 + strlen(BASEDIR "/XX/XXXXXXX"));
    CURL *curl;
    FILE *out;
    CURLcode res;
    curl = curl_easy_init();
    if (curl == NULL)
    {
        fprintf(stderr, "Curl init error\n");
        free(URI);
        free(DEST);
        free(firstdir);
        free(secdir);
        exit(1);
    }
    mkdir(BASEDIR, 0755);
#pragma omp parallel for
    for (int i = MINZOOM; i <= MAXZOOM; ++i)
    {
        sprintf(firstdir, BASEDIR "/%d", i);
        mkdir(firstdir, 0755);
        maxtile = pow(2, i);
        for (int long j = 0; j < maxtile; ++j)
        {
            sprintf(secdir, BASEDIR "/%d/%ld", i, j);
            mkdir(secdir, 0755);
            for (long int k = 0; k < maxtile; ++k)
            {
                printf("%d/%ld/%ld\n", i, j, k);
                sprintf(URI,
                        URLBASE "/%d/%ld/"
                                "%ld.png" URLARGS,
                        i, j, k);
                sprintf(DEST, BASEDIR "/%d/%ld/%ld.png", i, j, k);
                out = fopen(DEST, "wb");
                if (out == NULL)
                {
                    fprintf(stderr, "fopen error at %d/%ld/%ld\n", i, j, k);
                    free(URI);
                    free(DEST);
                    free(firstdir);
                    free(secdir);
                    curl_easy_cleanup(curl);
                    exit(1);
                }
                curl_easy_setopt(curl, CURLOPT_URL, URI);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, out);
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
                curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
                curl_easy_setopt(curl, CURLOPT_USERAGENT, USRAGNT);
                res = curl_easy_perform(curl);
                fclose(out);
            }
        }
    }
    curl_easy_cleanup(curl);
    free(URI);
    free(DEST);
    free(firstdir);
    free(secdir);
    return 0;
}
