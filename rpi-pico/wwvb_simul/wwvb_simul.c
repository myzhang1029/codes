/* WWVB transmitter. */
/*
 *  wwvb_simul.c
 *  Copyright (C) 2023 Zhang Maiyun <me@maiyun.me>
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

#include "wwvb_simul.h"
#include "wwvb.h"

#include <stdio.h>

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "pico/time.h"

#include "hardware/clocks.h"
#include "hardware/rtc.h"

#include "lwip/dns.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"

static struct ntp_client ntp_state;

/// Connect to Wi-Fi
static bool wifi_connect(void) {
    int n_configs = sizeof(wifi_config) / sizeof(struct wifi_config_entry);
    for (int i = 0; i < n_configs; ++i) {
        printf("Attempting Wi-Fi %s\n", wifi_config[i].ssid);
        int result = cyw43_arch_wifi_connect_timeout_ms(
            wifi_config[i].ssid,
            wifi_config[i].password,
            wifi_config[i].auth,
            5000
        );
        if (result == 0) {
            printf("IP Address: %s\n", ipaddr_ntoa(&WIFI_NETIF.ip_addr));
            // Print the current DNS server and reset it to the predefined value
            const ip_addr_t *pdns = dns_getserver(0);
            printf("DNS Server: %s\n", ipaddr_ntoa(pdns));
            ip_addr_t default_dns;
            printf("Reconfiguing DNS server to %s\n", DEFAULT_DNS);
            ipaddr_aton(DEFAULT_DNS, &default_dns);
            dns_setserver(0, &default_dns);
            // Good
            return true;
        }
        printf("Failed with status %d\n", result);
    }
    puts("WARNING: Cannot connect to Wi-Fi");
    return false;
}

static void wwvb_init() {
    // Prepare CARRIER_PIN
    wwvb_carrier_init(CARRIER_PIN);
    // Prepare MODULATION_PIN for output and set it high
    wwvb_modulation_init(MODULATION_PIN);

    // Log frequency
    uint f_clk_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    uint f_clk_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_USB);
    uint f_clk_rtc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_RTC);
    printf("clk_sys  = %dkHz\n", f_clk_sys);
    printf("clk_usb  = %dkHz\n", f_clk_usb);
    printf("clk_rtc  = %dkHz\n", f_clk_rtc);
}

static void datetime_to_tm(const datetime_t *dt, struct tm *tm) {
    struct tm intermediate = {
        .tm_sec = dt->sec,
        .tm_min = dt->min,
        .tm_hour = dt->hour,
        .tm_mday = dt->day,
        .tm_mon = dt->month - 1,
        .tm_year = dt->year - 1900,
        .tm_wday = 0,
        .tm_yday = 0,
        .tm_isdst = -1,
    };
    // Retrieve the yearday
    time_t time = mktime(&intermediate);
    localtime_r(&time, tm);
}

static void init() {
    stdio_init_all();
    if (cyw43_arch_init() != 0) {
        puts("WARNING: Cannot init CYW43");
        return;
    }
    rtc_init();
    wwvb_init();
    cyw43_arch_enable_sta_mode();
    wifi_connect();
    if (!ntp_client_init(&ntp_state))
        puts("WARNING: Cannot init NTP client");
    puts("Successfully initialized everything");
}

int main() {
    init();

    while (1) {
        int wifi_state = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
        extern volatile uint8_t ntp_stratum;
        if (wifi_state != CYW43_LINK_JOIN) {
            printf("Wi-Fi link status is %d, reconnecting\n", wifi_state);
            wifi_connect();
        }
        ntp_client_check_run(&ntp_state);
#if PICO_CYW43_ARCH_POLL
        cyw43_arch_poll();
#endif
        sleep_ms(1);
        if (ntp_stratum != 16) {
            datetime_t dt;
            struct tm tm;
            // A valid time is available
            // Make sure rtc is synced
            sleep_us(100);
            // Get the current time
            rtc_get_datetime(&dt);
            printf("Preparing transmission at %04d-%02d-%02d %d:%02d:%02d\n",
                    dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec);
            int8_t current_minute = dt.min;
            // Wait until the next start of minute
            while (dt.min == current_minute) {
                sleep_ms(1);
                rtc_get_datetime(&dt);
            }
            puts("Starting transmission");
            // Send five consecutive minutes
            for (int i = 0; i < 5; ++i) {
                rtc_get_datetime(&dt);
                datetime_to_tm(&dt, &tm);
                wwvb_send_once(MODULATION_PIN, &tm);
            }
        }
    }
    cyw43_arch_deinit();
}
