/*
 *  light.c
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

#include <math.h>

#include "pico/stdlib.h"
#include "pico/util/datetime.h"
#include "hardware/pwm.h"
#include "hardware/rtc.h"

#include "config.h"
#include "thekit4_pico_w.h"

/// Initialize everything we need
uint16_t current_pwm_level = 0;

uint32_t last_irq_timestamp = 0;

// For rtc alarm
static void light_on(void) {
    current_pwm_level = WRAP;
    pwm_set_gpio_level(LIGHT_PIN, current_pwm_level);
    puts("Turning on");
    light_register_next_alarm();
}

// For rtc alarm
static void light_off(void) {
    current_pwm_level = 0;
    pwm_set_gpio_level(LIGHT_PIN, current_pwm_level);
    puts("Turning off");
    light_register_next_alarm();
}

// For gpio irq
static void light_toggle(uint gpio, uint32_t events) {
    if (gpio != BUTTON1_PIN)
        return;
    // Debounce
    uint32_t irq_timestamp = time_us_32();
    if (irq_timestamp - last_irq_timestamp < 4000)
        return;
#if 0
    busy_wait_us(1000);
    if (events & GPIO_IRQ_EDGE_RISE && gpio_get(gpio)
            || events & GPIO_IRQ_EDGE_RISE && !gpio_get(gpio))
        // Event not sustained
        return;
#endif
    last_irq_timestamp = irq_timestamp;
    current_pwm_level = current_pwm_level ? 0 : WRAP;
    pwm_set_gpio_level(LIGHT_PIN, current_pwm_level);
    puts("Toggling");
}

void light_init(void) {
    // IO
    gpio_set_function(LIGHT_PIN, GPIO_FUNC_PWM);
    gpio_set_irq_enabled_with_callback(BUTTON1_PIN, GPIO_IRQ_EDGE_FALL, true, light_toggle);
    gpio_pull_up(BUTTON1_PIN);

    // PWM
    uint light_slice_num = pwm_gpio_to_slice_num(LIGHT_PIN);

    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, clockdiv);
    pwm_config_set_wrap(&config, WRAP - 1);
    pwm_init(light_slice_num, &config, true);
    pwm_set_gpio_level(LIGHT_PIN, current_pwm_level);
    pwm_set_enabled(light_slice_num, true);
}

static int intensity_to_dcycle(float intensity) {
    float real_intensity = exp(intensity * log(101.) / 100.) - 1.;
    float voltage = real_intensity * (19.2 - 7.845) / 100. + 7.845;
    if (7.845 < voltage && voltage <= 9.275)
        return (int)((-7.664 + voltage) * 2819.70);
    if (9.275 < voltage && voltage <= 13.75)
        return (int)((6.959 + voltage) * 265.20);
    if (13.75 < voltage && voltage <= 16.88)
        return (int)((-2.529 + voltage) * 494.85);
    if (16.88 < voltage)
    {
        int r = (int)((26.90 + voltage) * 216.92);
        return r > 10000 ? 10000 : r;
    }
    return 0;
}

/// Takes a percentage perceived intensity and dim the light
void light_dim(float intensity) {
    current_pwm_level = intensity_to_dcycle(intensity);
    pwm_set_gpio_level(LIGHT_PIN, current_pwm_level);
}

static void do_register_alarm(const datetime_t *current, int index) {
    datetime_t alarm = *current;
    alarm.hour = light_sched[index].hour;
    alarm.min = light_sched[index].min;
    alarm.sec = 0;
    rtc_enable_alarm();
    if (light_sched[index].on)
        rtc_set_alarm(&alarm, light_on);
    else
        rtc_set_alarm(&alarm, light_off);
    printf("Registered alarm to turn %s the light at %04d-%02d-%02d %d:%02d\n",
           light_sched[index].on ? "on" : "off", alarm.year, alarm.month,
           alarm.day, alarm.hour, alarm.min);
}

/// Move a datetime to the next day
static void next_day(datetime_t *dt) {
    if (dt->day < 28) {
        dt->day += 1;
        return;
    }
    switch (dt->month) {
        case 1:
        case 3:
        case 5:
        case 7:
        case 8:
        case 10:
        case 12:
            if (dt->day == 31) {
                dt->day = 1;
                dt->month += 1;
                return;
            }
            dt->day += 1;
            return;
        case 2:
            // We already know it is day 28
            dt->day = 1;
            dt->month += 1;
            return;
        case 4:
        case 6:
        case 9:
        case 11:
            if (dt->day == 30) {
                dt->day = 1;
                dt->month += 1;
                return;
            }
            dt->day += 1;
            return;
        default:
            // ???
            return;
    }
}

/// Returns `false` if NTC is not successful
bool light_register_next_alarm(void) {
    if (!time_in_sync)
        return false;
    datetime_t current;
    rtc_get_datetime(&current);
    int n_alarms = sizeof(light_sched) / sizeof(LIGHT_SCHED_ENTRY_T);
    // Find the next alarm
    for (int i = 0; i < n_alarms; ++i) {
        if (light_sched[i].hour > current.hour) {
            do_register_alarm(&current, i);
            return true;
        }
        if (light_sched[i].hour == current.hour
                && light_sched[i].min > current.min) {
            do_register_alarm(&current, i);
            return true;
        }
        // Go to the next one
    }
    // We past the last one. Register the first alarm after incrementing `day`
    current.day += 1;
    next_day(&current);
    do_register_alarm(&current, 0);
    return true;
}