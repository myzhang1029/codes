/*
 *  wwvb_simul.h
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

#ifndef _WWVB_SIMUL_H
#define _WWVB_SIMUL_H

#include "pico/cyw43_arch.h"

#include "hardware/clocks.h"

#include "lwip/ip_addr.h"

#define WIFI_NETIF (cyw43_state.netif[CYW43_ITF_STA])

struct ntp_client {
    struct ntp_client_current_request {
        bool in_progress;
        ip_addr_t server_address;
        struct udp_pcb *pcb;
        alarm_id_t resend_alarm;
    } req;
};

struct wifi_config_entry {
    const char *ssid;
    const char *password;
    uint32_t auth;
};

/*
 * Pins:
 * GP21 (carrier): 60kHz square wave
 *   It must be one of GP21, GP23, GP24, GP25 for clock output.
 * GP16 (modulation): high when transmitting at full power,
 *   low when reduced by 17dB
 * */
static const uint CARRIER_PIN = 21;
// Pin 21
static const uint MODULATION_PIN = 16;

// NTP and DNS config
static const char NTP_SERVER[] = "pool.ntp.org";
static const uint16_t NTP_PORT = 123;
// 10 minutes between syncs
static const uint32_t NTP_INTERVAL_MS = 600 * 1000;
static const char DEFAULT_DNS[] = "1.1.1.1";

// Define
// - `static const struct wifi_config_entry wifi_config[]`
// - `static const uint32_t TIME_CORR_SEC`
#include "private_config.h"

bool ntp_client_init(struct ntp_client *state);
void ntp_client_check_run(struct ntp_client *state);

#endif
