add_executable(wwvb_simul ntp.c wwvb_simul.c wwvb.c)

target_link_libraries(wwvb_simul
    pico_stdlib
    hardware_pwm hardware_rtc hardware_uart
    pico_cyw43_arch_lwip_threadsafe_background
)

target_include_directories(wwvb_simul PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

# create map/bin/hex file etc.
pico_add_extra_outputs(wwvb_simul)
pico_enable_stdio_usb(wwvb_simul 1)
pico_enable_stdio_uart(wwvb_simul 0)
