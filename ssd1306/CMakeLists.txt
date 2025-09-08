cmake_minimum_required(VERSION 3.13)

set(LIB_NAME ssd1306)

add_library(${LIB_NAME} INTERFACE)
target_sources(${LIB_NAME} INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/ssd1306_i2c.c
)

target_include_directories(${LIB_NAME} INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(${LIB_NAME} INTERFACE
    pico_stdlib         # Pulls in commonly used features
    hardware_i2c        # I2C support
    hardware_gpio       # GPIO support
    pico_binary_info    # Binary information support
)