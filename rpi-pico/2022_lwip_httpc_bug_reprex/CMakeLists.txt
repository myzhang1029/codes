add_executable(2022_lwip_httpc_bug_reprex reprex.c)

target_link_libraries(2022_lwip_httpc_bug_reprex
    pico_stdlib
    pico_lwip_http
    pico_cyw43_arch_lwip_threadsafe_background
)

target_include_directories(2022_lwip_httpc_bug_reprex PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

# create map/bin/hex file etc.
pico_add_extra_outputs(2022_lwip_httpc_bug_reprex)
pico_enable_stdio_usb(2022_lwip_httpc_bug_reprex 1)
pico_enable_stdio_uart(2022_lwip_httpc_bug_reprex 1)
