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
    printf("Temperature:\n");
    printf("\tCPU Proximity\t%0.1f\t°C\n", SMCGetTemperature("TC0P"));
//    printf("\tCPU Die\t%0.1f\t°C\n", SMCGetTemperature("TC0D"));
//    printf("\tCPU Heatsink\t%0.1f\t°C\n", SMCGetTemperature("TC0H"));
    printf("\tCPU Core 1\t%0.1f\t°C\n", SMCGetTemperature("TC0c"));
    printf("\tCPU Core 2\t%0.1f\t°C\n", SMCGetTemperature("TC1c"));
    printf("\tCPU Core 3\t%0.1f\t°C\n", SMCGetTemperature("TC2c"));
    printf("\tCPU Core 4\t%0.1f\t°C\n", SMCGetTemperature("TC3c"));
    printf("\tGPU Proximity\t%0.1f\t°C\n", SMCGetTemperature("TG0P"));
//    printf("\tGPU Heatsink\t%0.1f\t°C\n", SMCGetTemperature("TG0H"));
    printf("\tGPU Die\t%0.1f\t°C\n", SMCGetTemperature("TG0D"));
//    printf("\tNorthbridge\t%0.1f\t°C\n", SMCGetTemperature("TN0H"));
//    printf("\tNorthbridge Proximity\t%0.1f\t°C\n", SMCGetTemperature("TN0D"));
//    printf("\tNorthbridge Die\t%0.1f\t°C\n", SMCGetTemperature("TN0D"));
    printf("\tHDD 0 \t%0.1f\t°C\n", SMCGetTemperature("TH0P"));
    printf("\tMemory Proximity \t%0.1f\t°C\n", SMCGetTemperature("TM0P"));
    printf("Fan Num\t%i\n", fans);
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
    printf("CPU Voltage\t%0.1f\tV\n", SMCGetVoltageCurrent("VC0C"));
    printf("GPU Voltage\t%0.1f\tV\n", SMCGetVoltageCurrent("VG0C"));
    printf("CPU Current\t%0.1f\tA\n", SMCGetVoltageCurrent("IC0C"));
    printf("GPU Current\t%0.1f\tA\n", SMCGetVoltageCurrent("IG0C"));
//    printf("CPU Frequency\t%0.1f\tGHz\n", SMCGetVoltageCurrent("CC0C"));
//    printf("GPU Frequency\t%0.1f\tGHz\n", SMCGetVoltageCurrent("CG0C"));
    SMCClose();
    return 0;
}
