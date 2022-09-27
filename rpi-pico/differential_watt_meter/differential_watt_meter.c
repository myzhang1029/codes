/* Use a Raspberry Pi Pico as a Voltage-Current meter */
/*
 *  differential_watt_meter.c
 *  Copyright (C) 2022 Zhang Maiyun <myzhang1029@hotmail.com>
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
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

#define PS_PIN 23
// Connect a LM4040@3V to ADC_VREF
const float TO_VOLTAGE = 3.f / (1 << 12);
// Number of milliseconds to sleep
const uint32_t INTV = 1000;

int main() {
    stdio_init_all();
    // Set the SWPS to PWM mode
    gpio_init(PS_PIN);
    gpio_set_dir(PS_PIN, GPIO_OUT);
    gpio_put(PS_PIN, true);
    adc_init();
    // 26(A0) is the shunt voltage
    adc_gpio_init(26);
    // 27(A1) is the source (shunt + load) voltage
    adc_gpio_init(27);
    // 28(A2) is tied to AGND
    adc_gpio_init(28);
    adc_set_temp_sensor_enabled(false);

    while (1) {
        adc_select_input(2);
        uint16_t bias = adc_read();
        adc_select_input(0);
        uint16_t shunt = adc_read();
        adc_select_input(1);
        uint16_t source = adc_read();
        // Receive a timestamp for the measurement
        absolute_time_t abstime = get_absolute_time();
        uint32_t tstamp = to_ms_since_boot(abstime);

        printf(
            "%05d\t" "%f\t" "%05d\t" "%f\t" "%05d\t" "%ld\n",
            shunt, (shunt - bias) * TO_VOLTAGE,
            source, (source - bias) * TO_VOLTAGE,
            bias, tstamp
        );
        sleep_ms(INTV);
    }
}
