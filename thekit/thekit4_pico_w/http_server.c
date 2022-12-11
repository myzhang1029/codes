/*
 *  http_server.c
 *  Based on BSD-3-Clause picow_tcp_server.c
 *  Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
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

/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <string.h>

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#include "config.h"
#include "thekit4_pico_w.h"

#define TCP_PORT 80

static const char resp_common[] = "\r\nContent-Type: application/json\r\n"
                                  "Connection: close\r\n"
                                  "Server: zmy/0.1\r\n"
                                  "Content-Length: ";

static const char resp_200_pre[] = "HTTP/1.0 200 OK";
static const char resp_400_pre[] = "HTTP/1.0 400 BAD REQUEST";
static const char resp_400_post[] = "24\r\n\r\n"
                                    "{\"error\": \"bad request\"}";
static const char resp_404_pre[] = "HTTP/1.0 404 NOT FOUND";
static const char resp_404_post[] = "22\r\n\r\n"
                                    "{\"error\": \"not found\"}";
static const char resp_405_pre[] = "HTTP/1.0 405 METHOD NOT ALLOWED";
static const char resp_405_post[] = "31\r\n\r\n"
                                    "{\"error\": \"method not allowed\"}";
static const char resp_dashboard[] =
    "HTTP/1.0 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Connection: close\r\n"
    "Server: zmy/0.1\r\n"
    "Content-Length: 4045\r\n\r\n"
#include "dashboard.h"
    ;

static err_t tcp_conn_close(void *arg) {
    HTTP_SERVER_T *state = (HTTP_SERVER_T *)arg;
    err_t err = ERR_OK;
    if (state->client_pcb != NULL) {
        tcp_arg(state->client_pcb, NULL);
        tcp_sent(state->client_pcb, NULL);
        tcp_recv(state->client_pcb, NULL);
        tcp_err(state->client_pcb, NULL);
        err = tcp_close(state->client_pcb);
        if (err != ERR_OK) {
            printf("Close failed (%d), calling abort\n", err);
            tcp_abort(state->client_pcb);
            err = ERR_ABRT;
        }
        state->client_pcb = NULL;
    }
    return err;
}

static err_t tcp_conn_fail(void *arg, int status, const char *function) {
    printf("TCP server failed with status %d at %s\n", status, function);
    return tcp_conn_close(arg);
}

static err_t http_server_write(HTTP_SERVER_T *state, const char *buf,
                               size_t size, uint8_t copy) {
    struct tcp_pcb *tpcb = state->client_pcb;
    // this method is callback from lwIP, so cyw43_arch_lwip_begin is not
    // required, however you can use this method to cause an assertion in debug
    // mode, if this method is called when cyw43_arch_lwip_begin IS needed
    cyw43_arch_lwip_check();
    err_t err = tcp_write(tpcb, buf, size, copy);
    if (err != ERR_OK) {
        return tcp_conn_fail((void *)state, err, "write");
    }
    return ERR_OK;
}

// `true` means that we can stop reading
static bool http_req_check_parse(HTTP_SERVER_T *state) {
    char *found, *buf = state->received;
    found = strchr(buf, '\r');
    if (found)
        *found = 0;
    else {
        found = strchr(buf, '\n');
        if (found)
            *found = 0;
        else
            // Not complete yet
            return false;
    }
    cyw43_arch_lwip_check();
    char *path = strchr(buf, ' ');
    if (path == NULL)
        goto finish;
    /* Extract method (GET, POST, etc.) */
    const char *method = buf;
    size_t method_length = path - buf;
    /* Extract version (HTTP/1.1, etc.) to help end `path` */
    char *version = strrchr(buf, ' ');
    /* Set the space to NUL */
    *path = 0;
    /* Jump to the char after space */
    path += 1;
    if (version != NULL && version != path) {
        *version = 0;
        /* We don't use version anymore though */
        version += 1;
    }
    /* Only process GET because I discard the entire body */
    if (strncmp(method, "GET", method_length) != 0) {
        http_server_write(state, resp_405_pre, sizeof(resp_405_pre) - 1, 0);
        http_server_write(state, resp_common, sizeof(resp_common) - 1, 0);
        http_server_write(state, resp_405_post, sizeof(resp_405_post) - 1, 0);
        goto finish;
    }
    if (strcmp(path, "/") == 0) {
        http_server_write(state, resp_dashboard, 500, 0);
        http_server_write(state, resp_dashboard + 500, 500, 0);
        http_server_write(state, resp_dashboard + 1000, 500, 0);
        http_server_write(state, resp_dashboard + 1500, 500, 0);
        http_server_write(state, resp_dashboard + 2000, 500, 0);
        http_server_write(state, resp_dashboard + 2500, 500, 0);
        http_server_write(state, resp_dashboard + 3000, 500, 0);
        http_server_write(state, resp_dashboard + 3500, 500, 0);
        http_server_write(state, resp_dashboard + 4000, sizeof(resp_dashboard) - 3500 - 1, 0);
        goto finish;
    }
    if (strncmp(path, "/3light_dim", 11) == 0) {
        char *pos = strstr(path, "level=");
        if (pos == NULL) {
            http_server_write(state, resp_400_pre, sizeof(resp_400_pre) - 1, 0);
            http_server_write(state, resp_common, sizeof(resp_common) - 1, 0);
            http_server_write(state, resp_400_post, sizeof(resp_400_post) - 1, 0);
            goto finish;
        }
        float intensity = atof(pos + 6);
        // Max length + nn\r\n\r\n + \0
        char response[37] = {0};
        size_t length;
        light_dim(intensity);
        /* Generate response */
        length = snprintf(response, 37,
                     "30\r\n\r\n{\"dim\": true, \"value\": %3.2f}", intensity);
        snprintf(response, 37, "%u\r\n\r\n{\"dim\": true, \"value\": %3.2f}",
                 (unsigned)length - 6, intensity);
        http_server_write(state, resp_200_pre, sizeof(resp_200_pre) - 1, 0);
        http_server_write(state, resp_common, sizeof(resp_common) - 1, 0);
        // This one needs to be copied
        http_server_write(state, response, length, 1);
        goto finish;
    }
    http_server_write(state, resp_404_pre, sizeof(resp_404_pre) - 1, 0);
    http_server_write(state, resp_common, sizeof(resp_common) - 1, 0);
    http_server_write(state, resp_404_post, sizeof(resp_404_post) - 1, 0);

finish:
    // We only need one recv/send cycle, so we simply
    // close the connection here.
    free(state->received);
    state->received = NULL;
    state->allocated_len = 0;
    sleep_ms(0);
    tcp_conn_close(state);
    return true;
}

static err_t tcp_conn_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p,
                           err_t err) {
    HTTP_SERVER_T *state = (HTTP_SERVER_T *)arg;
    if (!p)
        return tcp_conn_fail(arg, -1, "remote disconnected");
    else if (err != ERR_OK) {
        /* cleanup, for unknown reason */
        pbuf_free(p);
        return err;
    }
    // this method is callback from lwIP, so cyw43_arch_lwip_begin is not
    // required, however you can use this method to cause an assertion in debug
    // mode, if this method is called when cyw43_arch_lwip_begin IS needed
    cyw43_arch_lwip_check();
    if (state->conn_state == HTTP_ACCEPTED) {
        // First chunk
        state->received = malloc(p->tot_len + 1);
        if (!state->received)
            return tcp_conn_fail(arg, -1, "malloc");
        state->allocated_len = p->tot_len;
        state->received[state->allocated_len] = 0;
        pbuf_copy_partial(p, state->received, p->tot_len, 0);
    }
    else if (state->conn_state == HTTP_RECEIVING) {
        // Not first chunk
        state->received =
            realloc(state->received, state->allocated_len + p->tot_len + 1);
        if (!state->received)
            return tcp_conn_fail(arg, -1, "realloc");
        // Copy into our buffer, beginning from the last chunk
        pbuf_copy_partial(p, state->received + state->allocated_len, p->tot_len,
                          0);
        // Now increase the counter
        state->allocated_len += p->tot_len;
        state->received[state->allocated_len] = 0;
    }
    else {
        // Might be DONE or something else
    }
    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    if (http_req_check_parse(state))
        state->conn_state = HTTP_OTHER;
    return ERR_OK;
}

static void tcp_conn_err(void *arg, err_t err) {
    if (err != ERR_ABRT)
        tcp_conn_fail(arg, err, "TCP error callback invoked");
}

static err_t http_server_accept(void *arg, struct tcp_pcb *client_pcb,
                                err_t err) {
    HTTP_SERVER_T *state = (HTTP_SERVER_T *)arg;
    if (err != ERR_OK || client_pcb == NULL) {
        tcp_conn_fail(arg, err, "accept");
        return ERR_VAL;
    }
    puts("Client connected");
    state->conn_state = HTTP_ACCEPTED;

    state->client_pcb = client_pcb;
    tcp_arg(client_pcb, state);
    tcp_recv(client_pcb, tcp_conn_recv);
    tcp_err(client_pcb, tcp_conn_err);

    return ERR_OK;
}

bool http_server_open(HTTP_SERVER_T *state) {
    if (!state)
        return false;
    printf("Starting server on port %u\n", TCP_PORT);

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        puts("Failed to create pcb");
        return false;
    }

    err_t err = tcp_bind(pcb, NULL, TCP_PORT);
    if (err) {
        puts("Failed to bind to port");
        return false;
    }

    state->server_pcb = tcp_listen_with_backlog(pcb, 1);
    if (!state->server_pcb) {
        puts("Failed to listen");
        http_server_close(state);
        return false;
    }

    // Specify the payload for the callbacks
    tcp_arg(state->server_pcb, state);
    tcp_accept(state->server_pcb, http_server_accept);

    return true;
}

void http_server_close(void *arg) {
    HTTP_SERVER_T *state = (HTTP_SERVER_T *)arg;
    if (state->server_pcb) {
        tcp_arg(state->server_pcb, NULL);
        tcp_close(state->server_pcb);
        state->server_pcb = NULL;
    }
}
