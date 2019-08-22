/*
 * Apple SMC reader
 * Copyright (C) 2019 Zhang Maiyun
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 USA.
 */

#include <stdio.h>
#include <IOKit/IOKitLib.h>

#include "smc.h"

int main(int argc, char *argv[])
{
    UInt32Char_t key;
    SMCOpen();
    int i = 0, fans = SMCGetFanNumber("FNum");
    printf("CPU Proximity\t%0.1f\t°C\n", SMCGetTemperature("TC0P"));
    printf("CPU Core 1\t%0.1f\t°C\n", SMCGetTemperature("TC0c"));
    printf("CPU Core 2\t%0.1f\t°C\n", SMCGetTemperature("TC1c"));
    printf("CPU Core 3\t%0.1f\t°C\n", SMCGetTemperature("TC2c"));
    printf("CPU Core 4\t%0.1f\t°C\n", SMCGetTemperature("TC3c"));
    printf("GPU Proximity\t%0.1f\t°C\n", SMCGetTemperature("TG0P"));
    printf("GPU Die\t%0.1f\t°C\n", SMCGetTemperature("TG0D"));
    printf("FAN NUM\t%i\n", fans);
    for (i = 0; i < fans; i++)
    {
        float act, target;
        sprintf(key, "F%dAc", i);
        act = SMCGetFanSpeed(key);
        printf("FAN %d\t%0.0f\tRPM\n", i + 1, act);
        sprintf(key, "F%dTg", i);
        target = SMCGetFanSpeed(key);
        printf("    Target speed : %.0f\n", target);
    }
    SMCClose();
    return 0;
}
