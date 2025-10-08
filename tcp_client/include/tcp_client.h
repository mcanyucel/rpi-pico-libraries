/**
 * @file tcp_client.h
 * @author
 * @brief Reusable TCP client driver for Pico W
 * @version 0.1
 * @date 2025-09-17
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

struct tcp_pcb;

// Status callback function type
typedef void (*tcp_client_status_callback_t)(const char *status_message);

/**
 * @brief TCP client configuration structure
 *
 * This structure holds the configuration parameters for the TCP client,
 * including server IP, port, timeouts, and status callback function.
 *
 * @note The server_ip should be a null-terminated string.
 */
typedef struct
{
    char server_ip[16];
    uint16_t server_port;
    uint32_t connect_timeout_ms;
    uint32_t response_timeout_ms;
    tcp_client_status_callback_t status_callback;
} tcp_client_config_t;

/**
 * @brief TCP client response structure
 *
 * This structure holds the response data and metadata from a TCP request,
 * including success status, error code, response data buffer, and round-trip time.
 *
 * @note The response_data buffer is fixed size; ensure it is large enough for expected responses.
 */
typedef struct
{
    bool success;
    int error_code;
    char response_data[512];
    size_t response_length;
    uint32_t round_trip_time_ms;
} tcp_client_response_t;

/**
 * @brief Opaque TCP client structure
 *
 * This structure represents the TCP client instance. Its internal details are hidden
 * from the user to encapsulate implementation specifics.
 */
typedef struct tcp_client tcp_client_t;

tcp_client_t* tcp_client_create(const tcp_client_config_t *config);

/**
 * @brief Send data over the TCP connection.
 * 
 * @param client 
 * @param data 
 * @param data_length 
 * @param response 
 * @return int 
 */
int tcp_client_send(tcp_client_t *client,
                    const void *data,
                    size_t data_length,
                    tcp_client_response_t *response);

int tcp_client_send_json(tcp_client_t *client,
                         const char *json_data,
                         tcp_client_response_t *response);

bool tcp_client_wifi_ready(void);

const char* tcp_client_error_string(int error_code);

/**
 * @brief Destroy the TCP client instance.
 *
 * This function cleans up and frees all resources associated with the
 * TCP client instance. It calls tcp_abort() on any active connection.
 * 
 * @note The TCP client will report TCP error -13: "Aborted" if there is an active connection as
 *       part of the destruction process.
 *
 * @param client The TCP client instance to destroy.
 */
void tcp_client_destroy(tcp_client_t *client);

// Error codes
#define TCP_CLIENT_SUCCESS           0
#define TCP_CLIENT_ERROR_WIFI       -1   // WiFi not ready
#define TCP_CLIENT_ERROR_INVALID    -2   // Invalid parameters  
#define TCP_CLIENT_ERROR_MEMORY     -3   // Memory allocation failed
#define TCP_CLIENT_ERROR_CONNECT    -4   // Connection failed
#define TCP_CLIENT_ERROR_TIMEOUT    -5   // Timeout occurred
#define TCP_CLIENT_ERROR_SEND       -6   // Send failed
#define TCP_CLIENT_ERROR_RECEIVE    -7   // Receive failed

// Default configuration values
#define TCP_CLIENT_DEFAULT_CONNECT_TIMEOUT_MS  5000
#define TCP_CLIENT_DEFAULT_RESPONSE_TIMEOUT_MS 10000