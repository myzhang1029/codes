/*
 *  wifi.c
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

#include "config.h"
#include "thekit4_pico_w.h"

#include "pico/cyw43_arch.h"

#if ENABLE_WATCHDOG
#include "hardware/watchdog.h"
#endif

#include "lwip/netif.h"
#include "mdns_fix.h"
#include "lwip/apps/mdns.h"

extern cyw43_t cyw43_state;

static void register_mdns(void) {
    cyw43_arch_lwip_begin();
    mdns_resp_init();
    mdns_resp_add_netif(&WIFI_NETIF, HOSTNAME);
    cyw43_arch_lwip_end();
}

/// Connect to Wi-Fi
bool wifi_connect(void) {
    int n_configs = sizeof(wifi_config) / sizeof(struct wifi_config_entry);
    for (int i = 0; i < n_configs; ++i) {
        printf("Attempting Wi-Fi %s\n", wifi_config[i].ssid);
        int result = cyw43_arch_wifi_connect_timeout_ms(
            wifi_config[i].ssid,
            wifi_config[i].password,
            wifi_config[i].auth,
            30000
        );
#if ENABLE_WATCHDOG
        watchdog_update();
#endif
        if (result == 0)
            goto succeed;
        printf("Failed with status %d\n", result);
    }
    puts("WARNING: Cannot connect to Wi-Fi");
    return false;
succeed:
    printf("Online\n");
    register_mdns();
    return 0;
}

void print_ip(void) {
    printf("IP Address: %s\n", ipaddr_ntoa(&WIFI_NETIF.ip_addr));
}
