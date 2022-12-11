add_executable(thekit4_pico_w thekit4_pico_w.c temperature.c light.c mdns_fix.c ntp.c http_server.c wifi.c)

target_link_libraries(thekit4_pico_w
    pico_stdlib
    hardware_adc hardware_pwm hardware_rtc
    pico_lwip_mdns
    pico_cyw43_arch_lwip_threadsafe_background
)

target_include_directories(thekit4_pico_w PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/..
)

target_compile_definitions(thekit4_pico_w PRIVATE
    WIFI1_SSID=\"${WIFI1_SSID}\"
    WIFI1_PASSWORD=\"${WIFI1_PASSWORD}\"
    WIFI2_SSID=\"${WIFI2_SSID}\"
    WIFI2_PASSWORD=\"${WIFI2_PASSWORD}\"
)

# create map/bin/hex file etc.
pico_add_extra_outputs(thekit4_pico_w)
pico_enable_stdio_usb(thekit4_pico_w 1)
pico_enable_stdio_uart(thekit4_pico_w 0)
