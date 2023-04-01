/*
 *  wwvb.h
 *  Copyright (C) 2023 Zhang Maiyun <me@myzhangll.xyz>
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

#ifndef _WWVB_H
#define _WWVB_H

#include <time.h>

#include "pico/stdlib.h"

// WWVB is 60kHz. Verify that the algorithms in `wwvb_*_init` work when changing this
static const uint CARRIER_FREQ = 60;

void wwvb_modulation_init(const uint modulation_pin);
void wwvb_carrier_init(const uint carrier_pin);
void wwvb_send_once(const uint modulation_pin, struct tm *current);

#endif
