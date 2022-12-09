/* inetd-style HTTP server for interfacing with thekit3_pwm. */
/*
 *  thekitd3.c
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

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *xmalloc(size_t bytes);
void *xrealloc(void *pointer, size_t bytes);
void xfree(const void *string);

static char DASHBOARD_RESP[] =
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "    <meta name=\"viewport\" content=\"width=device-width, "
    "initial-scale=1\" />\n"
    "    <meta charset=\"UTF-8\" />\n"
    "    <title> The Kit Controller </title>\n"
    "    <style>\n"
    ".color1 {background-color: #fffba9;}\n"
    ".color2 {background-color: #eb80fd;}\n"
    ".color3 {background-color: #76ff9a;}\n"
    ".color4 {background-color: #fffbf4;}\n"
    ".color5 {background-color: #ff817b;}\n"
    "\n"
    "html {\n"
    "    background-image: "
    "url(\"https://myzhangll.xyz/assets/img/scenery/image1.jpg\");\n"
    "    background-size: 100vw 100vh;\n"
    "    text-align: center;\n"
    "}\n"
    "\n"
    "h1 {\n"
    "    color: #411f07;\n"
    "    font-family: fantasy;\n"
    "    font-weight: bold;\n"
    "    font-size: 1.5em;\n"
    "}\n"
    "\n"
    "#resblk {\n"
    "    max-width: 100vw;\n"
    "    text-align: start;\n"
    "    word-wrap: break-word;\n"
    "    overflow-x: auto;\n"
    "}\n"
    "\n"
    "button, .slider {\n"
    "    color: #00152c;\n"
    "    opacity: 0.7;\n"
    "    margin: 0.5vh 1vw;\n"
    "    border: 2px solid #bfe3ae;\n"
    "    border-radius: 40px;\n"
    "    font-family: sans-serif;\n"
    "    font-size: 1.5em;\n"
    "    height: 8vh;\n"
    "}\n"
    "\n"
    ".slider {\n"
    "    -webkit-appearance: none;\n"
    "    outline: none;\n"
    "    -webkit-transition: .2s;\n"
    "    transition: opacity .2s;\n"
    "}\n"
    "\n"
    ".slider:hover, button:hover, form:hover {\n"
    "    opacity: 1;\n"
    "}\n"
    "\n"
    ".slider::-webkit-slider-thumb {\n"
    "    -webkit-appearance: none;\n"
    "    appearance: none;\n"
    "    width: 8vh;\n"
    "    height: 8vh;\n"
    "    border-radius: 40px;\n"
    "    background: #00152c;\n"
    "    cursor: pointer;\n"
    "}\n"
    "\n"
    ".slider::-moz-range-thumb {\n"
    "    width: 8vh;\n"
    "    height: 8vh;\n"
    "    border-radius: 40px;\n"
    "    background: #00152c;\n"
    "    cursor: pointer;\n"
    "}\n"
    "\n"
    ".reorg {\n"
    "    display: grid;\n"
    "    grid-template-columns: 50% 50%;\n"
    "    width: 100%;\n"
    "    vertical-align: middle;\n"
    "}\n"
    "\n"
    "form {\n"
    "    margin: auto;\n"
    "    font-size: 3em;\n"
    "    opacity: 0.7;\n"
    "}\n"
    "    </style>\n"
    "</head>\n"
    "<body>\n"
    "    <h1> The Kit Controller </h1>\n"
    "\n"
    "<div id=\"buttons\">"
    "    <!--div class=reorg>"
    "        <button class=\"turnon color1\" type=\"button\" "
    "onclick=\"lightSwitch(1, true)\">"
    "            Big Mikey I"
    "        </button>"
    "        <button class=\"turnoff color1\" type=\"button\" "
    "onclick=\"lightSwitch(1, false)\">"
    "            Big Mikey O"
    "        </button>"
    "    </div-->"
    "    <div class=reorg>"
    "        <button class=\"turnon color2\" type=\"button\" "
    "onclick=\"lightSwitch(2, true)\">"
    "            Stephanie I"
    "        </button>"
    "        <button class=\"turnoff color2\" type=\"button\" "
    "onclick=\"lightSwitch(2, false)\">"
    "            Stephanie O"
    "        </button>"
    "    </div>"
    "    <div class=\"reorg\">"
    "        <input type=\"range\" min=\"0\" max=\"100\" value=\"50\" "
    "class=\"slider color3\" id=\"light3_dimmer\" onchange=\"lightDim(3)\">"
    "        <!--button id=\"getinfo\" class=\"color5\" type=\"button\" "
    "onclick=\"getInfo()\">"
    "            Mikey Info"
    "        </button-->"
    "    </div>"
    "    <div class=\"reorg\">"
    "        <button id=\"playrecorded\" class=\"color1\" type=\"button\" "
    "onclick=\"playRecorded()\">"
    "            Good Morning"
    "        </button>"
    "        <button id=\"playsent\" class=\"color1\" type=\"button\" "
    "onclick=\"playSent()\">"
    "            Play Audio"
    "        </button>"
    "    </div>"
    "    <form method=\"POST\" enctype=\"multipart/form-data\" "
    "action=\"/player\">"
    "        <input type=\"file\" name=\"audio\">"
    "        <br />"
    "        <input type=\"submit\" value=\"Play at Mikey's\">"
    "    </form>"
    "</div>"
    "\n"
    "    <pre id=\"resblk\"></pre>\n"
    "\n"
    "    <script>\n"
    "        function xhrGlue(endpoint) {\n"
    "            let xhttp = new XMLHttpRequest();\n"
    "            xhttp.onreadystatechange = function() {\n"
    "                if (this.readyState == 4) {\n"
    "                    let text;\n"
    "                    try {\n"
    "                        const obj = JSON.parse(this.responseText);\n"
    "                        text = JSON.stringify(obj, null, 2);\n"
    "                    } catch (err) {\n"
    "                        text = err.name + \": \" + err.message;\n"
    "                    }\n"
    "                    document.getElementById(\"resblk\").innerHTML = "
    "text;\n"
    "                }\n"
    "            };\n"
    "            xhttp.open(\"GET\", endpoint, true);\n"
    "            xhttp.send();\n"
    "        }\n"
    "        function lightDim(n) {\n"
    "            xhrGlue(\"/\" + n + \"light_dim?level=\" + "
    "document.getElementById(\"light\" + n + \"_dimmer\").value);\n"
    "        }\n"
    "        function lightSwitch(n, on) {\n"
    "            xhrGlue(\"/\" + n + \"light_\" + (on ? \"on\" : \"off\"));\n"
    "        }\n"
    "        function playRecorded() {\n"
    "            xhrGlue(\"/playR\");\n"
    "        }\n"
    "        function playSent() {\n"
    "            xhrGlue(\"/playP\");\n"
    "        }\n"
    "</script>\n"
    "\n"
    "</body>\n"
    "</html>";

int print_ok(const char *type, const char *resp, const char *version)
{
    return printf("%s 200 OK\r\n"
                  "Content-Type: %s\r\n"
                  "Content-Length: %zu\r\n"
                  "Connection: close\r\n"
                  "Server: zmy/0.1\r\n\r\n"
                  "%s\r\n",
                  version, type, strlen(resp), resp);
}

int print_dashboard(const char *version)
{
    return print_ok("text/html", DASHBOARD_RESP, version);
}

int print_disallowed_request(const char *method, const char *version)
{
    const char complaint[] = "This method is not allowed: ";
    return printf("%s 405 METHOD NOT ALLOWED\r\n"
                  "Content-Type: text/plain\r\n"
                  "Content-Length: %zu\r\n"
                  "Connection: close\r\n"
                  "Server: zmy/0.1\r\n\r\n"
                  "%s%s\r\n",
                  version, strlen(method) + sizeof(complaint), complaint,
                  method);
}

int print_404(const char *version)
{
    const char complaint[] = "{\"error\": \"not found\"}";
    return printf("%s 404 NOT FOUND\r\n"
                  "Content-Type: application/json\r\n"
                  "Content-Length: %zu\r\n"
                  "Connection: close\r\n"
                  "Server: zmy/0.1\r\n\r\n"
                  "%s\r\n",
                  version, sizeof(complaint), complaint);
}

int print_malformed(const char *version)
{
    const char complaint[] = "{\"error\": \"bad request\"}";
    return printf("%s 400 BAD REQUEST\r\n"
                  "Content-Type: application/json\r\n"
                  "Content-Length: %zu\r\n"
                  "Connection: close\r\n"
                  "Server: zmy/0.1\r\n\r\n"
                  "%s\r\n",
                  version, sizeof(complaint), complaint);
}

int print_error(const char *errordesc, const char *version)
{
    return printf("%s 500 INTERNAL SERVER ERROR\r\n"
                  "Content-Type: application/json\r\n"
                  "Content-Length: %zu\r\n"
                  "Connection: close\r\n"
                  "Server: zmy/0.1\r\n\r\n"
                  "{\"error\": \"%s\"}\r\n",
                  version, 12 + strlen(errordesc), errordesc);
}

char *psh_fgets(FILE *fp)
{
    if (fp == NULL)
        return NULL;
    {
        size_t charcount = 0, nowhave = 256;
        int ch;
        char *result = xmalloc(nowhave);
        char *ptr = result;
        if (result == NULL)
            return NULL;
        while (1)
        {
            ch = fgetc(fp);
            if (ch == EOF)
            {
                if (ptr == result) /* nothing read */
                {
                    xfree(result);
                    return NULL;
                }
                break;
            }
            if (ch == '\n' || ch == '\r')
                break;
            *ptr++ = ch;
            if ((++charcount) == nowhave)
                result = xrealloc(result, (nowhave <<= 1));
        }
        *ptr = 0; /* Replace EOF or \n with NUL */
        result = xrealloc(result,
                          strlen(result) + 1); /* Resize the array to minimum */
        return result;
    }
}

int intensity_to_dcycle(double intensity)
{
    double real_intensity = exp(intensity * log(101.) / 100.) - 1.;
    double voltage = real_intensity * (19.2 - 7.845) / 100. + 7.845;
    if (7.845 < voltage && voltage <= 9.275)
        return (int)((-7.664 + voltage) * 281970.);
    if (9.275 < voltage && voltage <= 13.75)
        return (int)((6.959 + voltage) * 26520.);
    if (13.75 < voltage && voltage <= 16.88)
        return (int)((-2.529 + voltage) * 49485.);
    if (16.88 < voltage)
    {
        int r = (int)((26.90 + voltage) * 21692.);
        return r > 1000000 ? 1000000 : r;
    }
    return 0;
}

int main(void)
{
    char *command = psh_fgets(stdin);
    /* I don't care about the rest of the request */
    /* start of path */
    char *path = strchr(command, ' ');
    if (path == NULL)
    {
        /* What can I do? Simply drop the connection? */
        goto finish;
    }
    /* Extract version (HTTP/1.1, etc.) */
    char *version = strrchr(command, ' ');
    if (version == NULL || version == path)
    {
        /* What can I do? Still simply drop the connection? */
        goto finish;
    }
    /* Extract method (GET, POST, etc.) */
    char *method = command;
    size_t method_length = path - command;
    /* Set the space to NUL */
    *path = 0;
    *version = 0;
    /* Jump to the char after space */
    path += 1;
    version += 1;
    /* Log it for systemd */
    fprintf(stderr, "Got %s request for `%s', %s\n", method, path, version);
    /* Only process GET because I discard the entire body */
    if (strncmp(method, "GET", method_length) != 0)
    {
        print_disallowed_request(method, version);
        goto finish;
    }
    if (strcmp(path, "/") == 0)
    {
        print_dashboard(version);
        goto finish;
    }
    if (strncmp(path, "/3light_dim", 11) == 0)
    {
        char *pos = strstr(path, "level=");
        if (pos == NULL)
        {
            print_malformed(version);

            goto finish;
        }
        double intensity = atof(pos + 6);
        int dcycle = intensity_to_dcycle(intensity);
        char *response, *to_write;
        size_t size_alloc;
        FILE *ctrl = fopen("/dev/thekit_pwm", "w");
        if (ctrl == NULL)
        {
            print_error(strerror(errno), version);
            goto finish;
        }
        fprintf(stderr, "Dimming to %f (%d)\n", intensity, dcycle);
        /* Write to ctrl interface */
        size_alloc = snprintf(NULL, 0, "%d\n", dcycle) + 1;
        to_write = xmalloc(size_alloc);
        snprintf(to_write, size_alloc, "%d\n", dcycle);
        fwrite(to_write, size_alloc - 1, 1, ctrl);
        xfree(to_write);
        fclose(ctrl);
        /* Generate response */
        size_alloc =
            snprintf(NULL, 0, "{\"dim\": true, \"value\": %f, \"duty\": %d}",
                     intensity, dcycle) +
            1;
        response = xmalloc(size_alloc);
        snprintf(response, size_alloc,
                 "{\"dim\": true, \"value\": %f, \"duty\": %d}", intensity,
                 dcycle);
        print_ok("application/json", response, version);
        xfree(response);
        goto finish;
    }
    print_404(version);
finish:
    xfree(command);
    return 0;
}

/* Utilities: xmalloc */
static void memory_error_and_abort(char *fname)
{
    fprintf(stderr, "%s: out of virtual memory\n", fname);
    exit(2);
}

void *xmalloc(size_t bytes)
{
    void *temp;

    temp = malloc(bytes);
#ifdef DEBUG
    fprintf(stderr, "[xmalloc] %p(malloc %d)\n", temp, ++nref);
#endif
    if (temp == 0)
        memory_error_and_abort("xmalloc");
    return (temp);
}

void *xrealloc(void *pointer, size_t bytes)
{
    void *temp;

    temp = pointer ? realloc(pointer, bytes) : malloc(bytes);
#ifdef DEBUG
    fprintf(stderr, "[xmalloc] %p(free_realloc %d)\n", pointer, nref);
    fprintf(stderr, "[xmalloc] %p(malloc_realloc %d)\n", temp, nref);
#endif

    if (temp == 0)
        memory_error_and_abort("xrealloc");
    return (temp);
}

void xfree(const void *string)
{
#if DEBUG
    if (string)
        fprintf(stderr, "[xmalloc] %p(free %d)\n", string, nref--);
#endif
    free((void *)string);
}
