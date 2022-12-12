/*
 *  dimmer.h
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

#ifndef _DIMMER_H
#define _DIMMER_H

#include "lwip/ip_addr.h"

#define WIFI_NETIF (cyw43_state.netif[CYW43_ITF_STA])
#define has_wifi (netif_is_up(&WIFI_NETIF))

typedef struct NTP_T_ {
    ip_addr_t ntp_server_address;
    bool dns_request_sent;
    struct udp_pcb *ntp_pcb;
    absolute_time_t next_sync_time;
    alarm_id_t ntp_resend_alarm;
} NTP_T;

typedef struct HTTP_SERVER_T_ {
    struct tcp_pcb *server_pcb;
    struct tcp_pcb *client_pcb;
    enum {
        HTTP_OTHER = 0,
        HTTP_ACCEPTED,
        HTTP_RECEIVING
    } conn_state;
    // len = allocated_len + 1
    char *received;
    size_t allocated_len;
} HTTP_SERVER_T;

typedef struct LIGHT_SCHED_ENTRY_T_ {
    int8_t hour;
    int8_t min;
    bool on;
} LIGHT_SCHED_ENTRY_T;

typedef struct WIFI_CONFIG_T_ {
    const char *ssid;
    const char *password;
    uint32_t auth;
} WIFI_CONFIG_T;

extern bool has_cyw43;
extern bool time_in_sync;

void temperature_init(void);
float temperature_measure(void);

void light_init(void);
void light_dim(float intensity);
bool light_register_next_alarm(void);

bool wifi_connect(void);
void print_ip(void);

bool ntp_init(NTP_T *state);
void ntp_check_run(NTP_T *state);

bool http_server_open(HTTP_SERVER_T *state);
void http_server_close(void *arg);

bool trigger_tasks(void);
bool register_tasks(void);

#endif
