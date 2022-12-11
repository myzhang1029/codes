/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Adopted by Zhang Maiyun
 */

#include <string.h>
#include <time.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "hardware/rtc.h"

#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"

#include "config.h"
#include "thekit4_pico_w.h"

#define NTP_MSG_LEN 48
// Seconds between 1 Jan 1900 and 1 Jan 1970
#define NTP_DELTA 2208988800

// Called with results of operation
static void ntp_result(NTP_T *state, int status, time_t *result) {
    if (status == 0 && result) {
        time_t lresult = *result + TZ_DIFF_SEC;
        struct tm *lt = gmtime(&lresult);
        printf("Got NTP response: %02d/%02d/%04d %02d:%02d:%02d\n",
               lt->tm_mday, lt->tm_mon + 1, lt->tm_year + 1900,
               lt->tm_hour, lt->tm_min, lt->tm_sec);
        datetime_t dt = {
            .year  = lt->tm_year + 1900,
            .month = lt->tm_mon + 1,
            .day = lt->tm_mday,
            .dotw = lt->tm_wday,
            .hour = lt->tm_hour,
            .min = lt->tm_min,
            .sec = lt->tm_sec
        };
        if (rtc_set_datetime(&dt)) {
            time_in_sync = true;
            puts("RTC set");
        }
    }

    if (state->ntp_resend_alarm > 0) {
        cancel_alarm(state->ntp_resend_alarm);
        state->ntp_resend_alarm = 0;
    }
    state->next_sync_time = make_timeout_time_ms(NTP_INTERVAL_MS);
    state->dns_request_sent = false;
}

static int64_t ntp_failed_handler(alarm_id_t id, void *user_data)
{
    NTP_T *state = (NTP_T*)user_data;
    puts("NTP request failed");
    ntp_result(state, -1, NULL);
    return 0;
}

// Make an NTP request
static void ntp_request(NTP_T *state) {
    // cyw43_arch_lwip_begin/end should be used around calls into lwIP to ensure correct locking.
    // You can omit them if you are in a callback from lwIP. Note that when using pico_cyw_arch_poll
    // these calls are a no-op and can be omitted, but it is a good practice to use them in
    // case you switch the cyw43_arch type later.
    cyw43_arch_lwip_begin();
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, NTP_MSG_LEN, PBUF_RAM);
    uint8_t *req = (uint8_t *) p->payload;
    memset(req, 0, NTP_MSG_LEN);
    req[0] = 0x1b;
    udp_sendto(state->ntp_pcb, p, &state->ntp_server_address, NTP_PORT);
    pbuf_free(p);
    cyw43_arch_lwip_end();
}

// Call back with a DNS result
static void ntp_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg) {
    NTP_T *state = (NTP_T*)arg;
    if (ipaddr) {
        state->ntp_server_address = *ipaddr;
        printf("NTP address %s\n", ip4addr_ntoa(ipaddr));
        ntp_request(state);
    } else {
        puts("NTP DNS request failed");
        ntp_result(state, -1, NULL);
    }
}

// NTP data received
static void ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) {
    NTP_T *state = (NTP_T*)arg;
    uint8_t mode = pbuf_get_at(p, 0) & 0x7;
    uint8_t stratum = pbuf_get_at(p, 1);

    // Check the result
    if (ip_addr_cmp(addr, &state->ntp_server_address) && port == NTP_PORT && p->tot_len == NTP_MSG_LEN &&
        mode == 0x4 && stratum != 0) {
        uint8_t seconds_buf[4] = {0};
        pbuf_copy_partial(p, seconds_buf, sizeof(seconds_buf), 40);
        uint32_t seconds_since_1900 = seconds_buf[0] << 24 | seconds_buf[1] << 16 | seconds_buf[2] << 8 | seconds_buf[3];
        uint32_t seconds_since_1970 = seconds_since_1900 - NTP_DELTA;
        time_t epoch = seconds_since_1970;
        ntp_result(state, 0, &epoch);
    } else {
        puts("Invalid NTP response");
        ntp_result(state, -1, NULL);
    }
    pbuf_free(p);
}

// Perform initialisation
bool ntp_init(NTP_T *state) {
    if (!state)
        return false;
    state->ntp_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
    if (!state->ntp_pcb) {
        puts("Failed to create pcb");
        return false;
    }
    udp_recv(state->ntp_pcb, ntp_recv, state);
    return true;
}

/// Check and see if the time should be synchronized
void ntp_check_run(NTP_T *state) {
    if (!state)
        return;
    // `state` is zero-inited by `calloc` so it will always fire on the first time
    if (absolute_time_diff_us(get_absolute_time(), state->next_sync_time) < 0 && !state->dns_request_sent) {

        // Set alarm in case udp requests are lost
        state->ntp_resend_alarm = add_alarm_in_ms(NTP_RESEND_TIME_MS, ntp_failed_handler, state, true);

        // cyw43_arch_lwip_begin/end should be used around calls into lwIP to ensure correct locking.
        // You can omit them if you are in a callback from lwIP. Note that when using pico_cyw_arch_poll
        // these calls are a no-op and can be omitted, but it is a good practice to use them in
        // case you switch the cyw43_arch type later.
        cyw43_arch_lwip_begin();
        int err = dns_gethostbyname(NTP_SERVER, &state->ntp_server_address, ntp_dns_found, state);
        cyw43_arch_lwip_end();

        state->dns_request_sent = true;
        if (err == ERR_OK) {
            ntp_request(state); // Cached result
        } else if (err != ERR_INPROGRESS) { // ERR_INPROGRESS means expect a callback
            puts("DNS request failed");
            ntp_result(state, -1, NULL);
        }
    }
}
