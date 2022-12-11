#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "thekit4_pico_w.h"

// Light-related
// Definitions
static const uint LIGHT_PIN = 2;
static const uint BUTTON1_PIN = 16;
// Magic. TODO: Try to reach 1MHz at PWM?
static const float clockdiv = 1.;
// Max duty
static const uint16_t WRAP = 1000;

// Light-based alarms
// Sort chronologically
static const LIGHT_SCHED_ENTRY_T light_sched[] = {
    {7, 30, true},
    {8, 30, false},
    {21, 30, true},
    {22, 30, false},
};

// Temperature-related
// Zeroing pin
static const uint ADC_ZERO_PIN = 28;
// Temperature pin
static const uint ADC_TEMP_PIN = 26;
// LM2020
static const float VAref = 3.0; // Volts
// NTC base resistance
static const float R0 = 1e4; // Ohm \pm 1%
// Temperature corresponding to R0
static const float T0 = 25.0 + 273.15; // Kelvin
// divider resistance
static const float R = 1e4;     // Ohm \pm 1%
static const float BETA = 3977; // Kelvin \pm 0.75%

static const WIFI_CONFIG_T wifi_config[] = {
    {WIFI1_SSID, WIFI1_PASSWORD, CYW43_AUTH_WPA2_AES_PSK},
    {WIFI2_SSID, WIFI2_PASSWORD, CYW43_AUTH_WPA2_AES_PSK},
};

#define HOSTNAME "rpipicow"

// Time-related
#define NTP_SERVER "pool.ntp.org"
#define NTP_PORT 123
// One hour between syncs
#define NTP_INTERVAL_MS (3600 * 1000)
#define NTP_RESEND_TIME_MS (10 * 1000)
// Crude TZ conversion
#define TZ_DIFF_SEC (-8 * 3600)
