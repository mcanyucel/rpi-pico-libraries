# BLE Nordic UART Service Driver for Raspberry Pi Pico W / 2W

# Create the library FIRST (as STATIC, not INTERFACE)
add_library(ble_nordic_uart STATIC
    ${CMAKE_CURRENT_LIST_DIR}/ble_nordic_uart.c
)

# Link required BTstack libraries
target_link_libraries(ble_nordic_uart PUBLIC
    pico_stdlib
    pico_btstack_ble
    pico_btstack_cyw43
    pico_cyw43_arch_lwip_poll
)

# Include directories
target_include_directories(ble_nordic_uart PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

# NOW compile the GATT profile (after library is created)
pico_btstack_make_gatt_header(ble_nordic_uart PRIVATE 
    "${CMAKE_CURRENT_LIST_DIR}/nordic_uart.gatt")