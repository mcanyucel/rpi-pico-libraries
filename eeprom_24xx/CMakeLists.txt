cmake_minimum_required(VERSION 3.13)

set(LIB_NAME eeprom_24xx)

add_library(${LIB_NAME} INTERFACE)
target_sources(${LIB_NAME} INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/eeprom_24xx.c
)

target_include_directories(${LIB_NAME} INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(${LIB_NAME} INTERFACE
    pico_stdlib
    hardware_i2c
    hardware_gpio
)