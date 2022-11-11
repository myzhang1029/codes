/* Discharge a battery; turn the transistor off at a target voltage. */
/*
 *  discharger.c
 *  Copyright (C) 2022 Zhang Maiyun <me@myzhangll.xyz>
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
#define TRANSISTOR_PIN 1

// Connect a LM4040@3V to ADC_VREF
const float TO_VOLTAGE = 3.f / (1 << 12);
// Number of milliseconds to sleep
const uint32_t INTV = 1000;
// Target voltage
const float TARGET_VOLTAGE = 0.8f;

int main() {
    bool state = true;
    stdio_init_all();
    // Set the SWPS to PWM mode
    gpio_init(PS_PIN);
    gpio_set_dir(PS_PIN, GPIO_OUT);
    gpio_init(TRANSISTOR_PIN);
    gpio_set_dir(TRANSISTOR_PIN, GPIO_OUT);
    gpio_put(PS_PIN, true);
    gpio_put(TRANSISTOR_PIN, true);
    adc_init();
    // 27(A1) is the source voltage
    adc_gpio_init(27);
    // 28(A2) is tied to AGND
    adc_gpio_init(28);
    adc_set_temp_sensor_enabled(false);

    while (1) {
        adc_select_input(2);
        uint16_t bias = adc_read();
        adc_select_input(1);
        uint16_t source = adc_read();
        // Receive a timestamp for the measurement
        absolute_time_t abstime = get_absolute_time();
        uint32_t tstamp = to_ms_since_boot(abstime);
        float voltage = (source - bias) * TO_VOLTAGE;

        fputs(state ? "true" : "false", stdout);
        printf(
            "\t%05d\t" "%f\t" "%05d\t" "%ld\n",
            source, voltage,
            bias, tstamp
        );
        if (voltage <= TARGET_VOLTAGE)
            gpio_put(TRANSISTOR_PIN, (state = false));
        else
            gpio_put(TRANSISTOR_PIN, (state = true));
        sleep_ms(INTV);
    }
}
