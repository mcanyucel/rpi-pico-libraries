cmake_minimum_required(VERSION 3.13)

set(LIB_NAME tcp_client)

add_library(${LIB_NAME} INTERFACE)
target_sources(${LIB_NAME} INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/tcp_client.c
)

target_include_directories(${LIB_NAME} INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(${LIB_NAME} INTERFACE
    pico_stdlib # Pulls in commonly used features
    pico_cyw43_arch_lwip_poll # For networking with the CYW43 chip
)