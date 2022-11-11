add_executable(discharger discharger.c)

target_link_libraries(discharger pico_stdlib hardware_adc)

# create map/bin/hex file etc.
pico_add_extra_outputs(discharger)
pico_enable_stdio_usb(discharger 1)
pico_enable_stdio_uart(discharger 0)
