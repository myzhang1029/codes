add_executable(mus7final mus7final.c)

target_link_libraries(mus7final pico_stdlib hardware_adc hardware_dma)

# create map/bin/hex file etc.
pico_add_extra_outputs(mus7final)
pico_enable_stdio_usb(mus7final 1)
pico_enable_stdio_uart(mus7final 0)
