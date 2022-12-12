/*
 *  tasks.c
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

#include "lwip/apps/http_client.h"

#include "config.h"
#include "thekit4_pico_w.h"

repeating_timer_t tasks_timer;

static void http_recv_callback(void *arg, struct altcp_pcb *conn, struct pbuf *p, err_t err) {
    // I'll simply discard the data
    if (err != ERR_OK)
        printf("HTTP request for %s received error %d\n", (const char *)arg, err);
}

static bool send_ddns(void) {
    char uri[256] = {0};
    snprintf(uri, 256, DDNS_URI, DDNS_HOSTNAME, DDNS_KEY, ipaddr_ntoa(&WIFI_NETIF.ip_addr));
    puts("Sending DDNS");
    return httpc_get_file_dns(
        DDNS_HOST,
        HTTP_DEFAULT_PORT,
        uri,
        NULL,
        (altcp_recv_fn)&http_recv_callback,
        (void *)DDNS_URI,
        NULL
    ) == ERR_OK;
}

static bool send_temperature(void) {
    float temperature = temperature_measure();
    char uri[64] = {0};
    snprintf(uri, 64, WOLFRAM_URI, WOLFRAM_DATABIN_ID, temperature);
    puts("Sending temperature");
    return httpc_get_file_dns(
        WOLFRAM_HOST,
        HTTP_DEFAULT_PORT,
        uri,
        NULL,
        (altcp_recv_fn)&http_recv_callback,
        (void *)WOLFRAM_URI,
        NULL
    ) == ERR_OK;
}

static bool do_run_tasks(struct repeating_timer *t) {
    bool result = true;
    result &= send_ddns();
    result &= send_temperature();
    if (!result)
        puts("Tasks failed");
    return result;
}

bool trigger_tasks(void) {
    return do_run_tasks(NULL);
}

bool register_tasks(void) {
    puts("Registering repeating tasks");
    return add_repeating_timer_ms(TASKS_INTERVAL_MS, do_run_tasks, NULL, &tasks_timer);
}
