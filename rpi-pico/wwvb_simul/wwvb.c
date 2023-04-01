/*
 *  wwvb.c
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

#include <stdio.h>

#include "wwvb.h"

#include "pico/divider.h"
#include "pico/stdlib.h"

#include "hardware/clocks.h"

void wwvb_modulation_init(const uint modulation_pin) {
    gpio_init(modulation_pin);
    gpio_set_dir(modulation_pin, GPIO_OUT);
    // High amplification on initial state
    gpio_put(modulation_pin, 1);
}

void wwvb_carrier_init(const uint carrier_pin) {
#if 0
    // Configure a PWM at the correct frequency
    // (four times the desired carrier frequency)
    pwm_config config = pwm_get_default_config();
    uint f_clk_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    // In kHz
    float clkdiv = f_clk_sys / 4.0f / CARRIER_FREQ;
    pwm_config_set_clkdiv(&config, clkdiv);
    // Set CARRIER_PIN to PWM and kick off the PWM
    gpio_set_function(CARRIER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(CARRIER_PIN);
    pwm_init(slice_num, &config, true);
    pwm_set_wrap(slice_num, 4);
    // 50% duty cycle
    pwm_set_gpio_level(CARRIER_PIN, 2);
    pwm_set_enabled(slice_num, true);
#endif

    uint f_clk_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_USB);
    // Tie the carrier pin to the USB clock
    uint32_t div_int = f_clk_usb / CARRIER_FREQ;
    clock_gpio_init_int_frac(carrier_pin, CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLK_USB, div_int, 0);
}

static inline void send_one(const uint modulation_pin) {
    // Reduced strength for 0.5s, then back to full strength
    gpio_put(modulation_pin, 0);
    sleep_ms(500);
    gpio_put(modulation_pin, 1);
    sleep_ms(500);
    puts("1");
}

static inline void send_zero(const uint modulation_pin) {
    // Reduced strength for 0.2s, then back to full strength
    gpio_put(modulation_pin, 0);
    sleep_ms(200);
    gpio_put(modulation_pin, 1);
    sleep_ms(800);
    puts("0");
}

static inline void send_marker(const uint modulation_pin) {
    // Reduced strength for 0.8s, then back to full strength
    gpio_put(modulation_pin, 0);
    sleep_ms(800);
    gpio_put(modulation_pin, 1);
    sleep_ms(200);
    puts("M");
}

/// Send bits [1, 10).
static inline void send_minute(const uint modulation_pin, const uint32_t minute) {
    uint32_t ones;
    uint32_t tens = divmod_u32u32_rem(minute, 10, &ones);
    // MSB = 40
    tens & 4 ? send_one(modulation_pin) : send_zero(modulation_pin);
    tens & 2 ? send_one(modulation_pin) : send_zero(modulation_pin);
    tens & 1 ? send_one(modulation_pin) : send_zero(modulation_pin);
    // Zero @ bit 4
    send_zero(modulation_pin);
    // MSB = 8
    ones & 8 ? send_one(modulation_pin) : send_zero(modulation_pin);
    ones & 4 ? send_one(modulation_pin) : send_zero(modulation_pin);
    ones & 2 ? send_one(modulation_pin) : send_zero(modulation_pin);
    ones & 1 ? send_one(modulation_pin) : send_zero(modulation_pin);
    // Marker @ bit 9
    send_marker(modulation_pin);
}

/// Send bits [10, 20).
static inline void send_hour(const uint modulation_pin, const uint32_t hour) {
    uint32_t ones;
    uint32_t tens = divmod_u32u32_rem(hour, 10, &ones);
    // Zero @ bit 10, 11
    send_zero(modulation_pin);
    send_zero(modulation_pin);
    // MSB = 20
    tens & 2 ? send_one(modulation_pin) : send_zero(modulation_pin);
    tens & 1 ? send_one(modulation_pin) : send_zero(modulation_pin);
    // Zero @ bit 14
    send_zero(modulation_pin);
    // MSB = 8
    ones & 8 ? send_one(modulation_pin) : send_zero(modulation_pin);
    ones & 4 ? send_one(modulation_pin) : send_zero(modulation_pin);
    ones & 2 ? send_one(modulation_pin) : send_zero(modulation_pin);
    ones & 1 ? send_one(modulation_pin) : send_zero(modulation_pin);
    // Marker @ bit 19
    send_marker(modulation_pin);
}

/// Send bits [20, 34).
static inline void send_doty(const uint modulation_pin, const uint32_t doty) {
    uint32_t ones, tens;
    uint32_t hundreds = divmod_u32u32_rem(doty, 100, &tens);
    tens = divmod_u32u32_rem(tens, 10, &ones);
    // Zero @ bit 20, 21
    send_zero(modulation_pin);
    send_zero(modulation_pin);
    // MSB = 200
    hundreds & 2 ? send_one(modulation_pin) : send_zero(modulation_pin);
    hundreds & 1 ? send_one(modulation_pin) : send_zero(modulation_pin);
    // Zero @ bit 24
    send_zero(modulation_pin);
    // MSB = 80
    tens & 8 ? send_one(modulation_pin) : send_zero(modulation_pin);
    tens & 4 ? send_one(modulation_pin) : send_zero(modulation_pin);
    tens & 2 ? send_one(modulation_pin) : send_zero(modulation_pin);
    tens & 1 ? send_one(modulation_pin) : send_zero(modulation_pin);
    // Marker @ bit 29
    send_marker(modulation_pin);
    // MSB = 8
    ones & 8 ? send_one(modulation_pin) : send_zero(modulation_pin);
    ones & 4 ? send_one(modulation_pin) : send_zero(modulation_pin);
    ones & 2 ? send_one(modulation_pin) : send_zero(modulation_pin);
    ones & 1 ? send_one(modulation_pin) : send_zero(modulation_pin);
}

/// Send bits [34, 44).
/// DUT1 is a signed 8-bit integer where 1 = 0.1s
static inline void send_dut1(const uint modulation_pin, int8_t dut1) {
    // Zero @ bit 34, 35
    send_zero(modulation_pin);
    send_zero(modulation_pin);
    // Sign
    if (dut1 < 0) {
        send_zero(modulation_pin);
        send_one(modulation_pin);
        send_zero(modulation_pin);
        dut1 = -dut1;
    } else {
        send_one(modulation_pin);
        send_zero(modulation_pin);
        send_one(modulation_pin);
    }
    // Marker @ bit 39
    send_marker(modulation_pin);
    // MSB = 0.8
    dut1 & 8 ? send_one(modulation_pin) : send_zero(modulation_pin);
    dut1 & 4 ? send_one(modulation_pin) : send_zero(modulation_pin);
    dut1 & 2 ? send_one(modulation_pin) : send_zero(modulation_pin);
    dut1 & 1 ? send_one(modulation_pin) : send_zero(modulation_pin);
}

/// Send bits [44, 56).
/// Also sends the leap year bit.
static inline void send_year(const uint modulation_pin, const uint32_t year) {
    uint32_t ones, tens;
    uint32_t hundreds = divmod_u32u32_rem(year, 100, &tens);
    tens = divmod_u32u32_rem(tens, 10, &ones);
    // Zero @ bit 44
    send_zero(modulation_pin);
    // Only send last two digits
    // MSB = 80
    tens & 8 ? send_one(modulation_pin) : send_zero(modulation_pin);
    tens & 4 ? send_one(modulation_pin) : send_zero(modulation_pin);
    tens & 2 ? send_one(modulation_pin) : send_zero(modulation_pin);
    tens & 1 ? send_one(modulation_pin) : send_zero(modulation_pin);
    // Marker @ bit 49
    send_marker(modulation_pin);
    // MSB = 8
    ones & 8 ? send_one(modulation_pin) : send_zero(modulation_pin);
    ones & 4 ? send_one(modulation_pin) : send_zero(modulation_pin);
    ones & 2 ? send_one(modulation_pin) : send_zero(modulation_pin);
    ones & 1 ? send_one(modulation_pin) : send_zero(modulation_pin);
    // Zero @ bit 54
    send_zero(modulation_pin);
    // Leap year indicator
    if (year & 3) {
        // Not a leap year
        send_zero(modulation_pin);
    } else if (ones == 0 && tens == 0) {
        if (hundreds & 3) {
            // Leap year (divisible by 400)
            send_one(modulation_pin);
        } else {
            // Not a leap year
            send_zero(modulation_pin);
        }
    } else {
        // Leap year
        send_one(modulation_pin);
    }
}

/// Send bit 56.
static inline void send_leap_second(const uint modulation_pin, const uint32_t leap_second) {
    // Leap second indicator
    if (leap_second) {
        send_one(modulation_pin);
    } else {
        send_zero(modulation_pin);
    }
}

/// Send bits [57, 60).
static inline void send_dst(const uint modulation_pin, const uint32_t dst_bits) {
    if (dst_bits & 2) {
        send_one(modulation_pin);
    } else {
        send_zero(modulation_pin);
    }
    if (dst_bits & 1) {
        send_one(modulation_pin);
    } else {
        send_zero(modulation_pin);
    }
    // Marker @ bit 59
    send_marker(modulation_pin);
}

void wwvb_send_once(const uint modulation_pin, struct tm *current) {
    uint32_t minute = current->tm_min;
    uint32_t hour = current->tm_hour;
    uint32_t doty = current->tm_yday + 1;
    // TODO: Handle UT1, leap seconds, and DST
    int8_t dut1 = 0;
    uint32_t year = current->tm_year + 1900;
    uint32_t leap_second = 0;
    uint32_t dst_bits = 0;

    send_marker(modulation_pin);
    send_minute(modulation_pin, minute);
    send_hour(modulation_pin, hour);
    send_doty(modulation_pin, doty);
    send_dut1(modulation_pin, dut1);
    send_year(modulation_pin, year);
    send_leap_second(modulation_pin, leap_second);
    send_dst(modulation_pin, dst_bits);
}
