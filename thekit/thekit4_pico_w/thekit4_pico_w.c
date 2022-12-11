/* The entire The Kit on Raspberry Pi Pico W */
/*
 *  thekit4_pico_w.c
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

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "pico/time.h"

#include "hardware/rtc.h"

#include "thekit4_pico_w.h"

bool has_cyw43 = false;
bool time_in_sync = false;
NTP_T ntp_state = {};
HTTP_SERVER_T http_state = {NULL, NULL, 0, NULL, 0};

int init() {
    stdio_init_all();

    rtc_init();
    light_init();
    temperature_init();

    if (cyw43_arch_init() != 0) {
        puts("WARNING: Cannot init CYW43");
        return -1;
    }
    has_cyw43 = true;
    wifi_connect();
    // Depends on cyw43
    if (!ntp_init(&ntp_state)) {
        puts("WARNING: Cannot init NTP client");
        return -1;
    }
    // Start HTTP server
    if (!http_server_open(&http_state)) {
        puts("WARNING: Cannot open HTTP server");
        return -1;
    }

    puts("Successfully initialized everything");

    print_ip();
    printf("Temperature: %f\n", temperature_measure());

    return 0;
}

int main() {
    bool alarm_first_register_done = false;

    init();

    while (1) {
        if (has_cyw43 && !has_wifi)
            // Reconnect Wi-Fi
            wifi_connect();
        ntp_check_run(&ntp_state);
#if PICO_CYW43_ARCH_POLL
        if (has_cyw43)
            cyw43_arch_poll();
#endif
        if (!alarm_first_register_done) {
            // It waits for NTC to be up
            alarm_first_register_done = light_register_next_alarm();
            puts("Alarm waiting for RTC");
        }
        sleep_ms(10);
    }
    http_server_close(&http_state);
    if (has_cyw43)
        cyw43_arch_deinit();
}
