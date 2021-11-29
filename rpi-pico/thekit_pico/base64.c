/* Simple base64 decoder */
/*
 *  base64.c
 *  Copyright (C) 2021 Zhang Maiyun <myzhang1029@hotmail.com>
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

#include <stdint.h>

#include "base64.h"

// Corresponding base64 index from '+' to 'z'
const uint8_t DECODE_TABLE[] = {
    62, 0, 0, 0, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
    0, 0, 0, 0, 0, 0, 0,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
    0, 0, 0, 0, 0, 0,
    26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51
};

const uint16_t IEXP2[] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096};

int base64_feed(struct base64decoder *decoder, int m) {
    if (m < '+' || m > 'z')
        return 1;
    decoder->buf <<= 6;
    decoder->buf += DECODE_TABLE[m - '+'];
    decoder->count += 6;
    return 0;
}

uint8_t base64_read(struct base64decoder *decoder) {
    uint8_t ch;
    uint16_t mask = IEXP2[decoder->count];
    decoder->count -= 8;
    mask -= IEXP2[decoder->count];
    ch = (decoder->buf & mask) >> decoder->count;
    decoder->buf &= ~mask;
    return ch;
}

#ifdef BASE64_EXAMPLE
#include <stdio.h>

int main() {
    struct base64decoder decoder = BASE64_INITIALIZER;
    while (1) {
        int m = getc(stdin);
        if (m == '\n')
            continue;
        if (m == '=' || m == EOF || m == 0)
            break;
        base64_feed(&decoder, m);
        if (decoder.count >= 8)
            putchar(base64_read(&decoder));
    }
}
#endif
