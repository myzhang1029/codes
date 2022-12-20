#include <stdio.h>

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "pico/stdlib.h"

#include "lwip/apps/http_client.h"

#include "lwip/netif.h"

static const char DDNS_HOST[] = "dyn.dns.he.net";
static const char DDNS_URI[] = "/";

static err_t http_recv_callback(void *arg, struct altcp_pcb *_conn,
        struct pbuf *p, err_t err) {
    cyw43_arch_lwip_check();
    pbuf_free(p);
    return ERR_OK;
}

int main() {
    stdio_init_all();

    if (cyw43_arch_init() != 0) {
        puts("ERROR: Cannot init CYW43");
        return 1;
    }
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms("SSID", "PASSWORD", CYW43_AUTH_WPA2_AES_PSK, 30000) != ERR_OK)
        puts("ERROR: Cannot connect to Wi-Fi");
    while (1) {
        cyw43_arch_lwip_begin();
        httpc_get_file_dns(
            DDNS_HOST,
            HTTP_DEFAULT_PORT,
            DDNS_URI,
            NULL,
            &http_recv_callback,
            NULL,
            NULL
        );
        cyw43_arch_lwip_end();
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(50);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(50);
    }
    cyw43_arch_deinit();
}
