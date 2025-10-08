cmake_minimum_required(VERSION 3.13)

set(LIB_NAME ina219)

add_library(${LIB_NAME} INTERFACE)
target_sources(${LIB_NAME} INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/ina219_i2c.c
)

target_include_directories(${LIB_NAME} INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(${LIB_NAME} INTERFACE
    pico_stdlib         # Pulls in commonly used features
    hardware_adc        # ADC support
    pico_platform       # Math library support
)