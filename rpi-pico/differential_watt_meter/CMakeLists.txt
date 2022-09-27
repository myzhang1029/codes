add_executable(differential_watt_meter differential_watt_meter.c)

target_link_libraries(differential_watt_meter pico_stdlib hardware_adc)

# create map/bin/hex file etc.
pico_add_extra_outputs(differential_watt_meter)
pico_enable_stdio_usb(differential_watt_meter 1)
pico_enable_stdio_uart(differential_watt_meter 0)
