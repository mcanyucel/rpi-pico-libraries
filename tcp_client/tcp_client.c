/**
 * @file tcp_client.c
 * @author
 * @brief TCP client driver implementation for Pico W
 * @version 0.1
 * @date 2025-09-17
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "tcp_client.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/ip_addr.h"
#include "lwip/pbuf.h"
#include <string.h>
#include <stdio.h>

// TCP client internal structure
struct tcp_client
{
    tcp_client_config_t config;
    struct tcp_pcb *pcb;
    ip_addr_t server_addr;

    bool connected;
    bool complete;
    bool success;

    char response_buffer[512];
    size_t response_length;

    absolute_time_t start_time;
    uint32_t round_trip_time_ms;
};

// Error message strings
static const char *error_messages[] = {
    [0] = "Success",
    [-TCP_CLIENT_ERROR_WIFI] = "WiFi not ready",
    [-TCP_CLIENT_ERROR_INVALID] = "Invalid parameters",
    [-TCP_CLIENT_ERROR_MEMORY] = "Memory allocation failed",
    [-TCP_CLIENT_ERROR_CONNECT] = "Connection failed",
    [-TCP_CLIENT_ERROR_TIMEOUT] = "Timeout occurred",
    [-TCP_CLIENT_ERROR_SEND] = "Send failed",
    [-TCP_CLIENT_ERROR_RECEIVE] = "Receive failed"};

// Internal callback functions
static err_t tcp_client_connected_callback(void *arg, struct tcp_pcb *tcp_pcb, err_t err)
{
    tcp_client_t *client = (tcp_client_t *)arg;

    if (err != ERR_OK)
    {
        printf("[TCP_CLIENT] Connection failed: %d\n", err);
        client->success = false;
        client->complete = true;
        return err;
    }

    printf("[TCP_CLIENT] Connected to server\n");
    client->connected = true;

    if (client->config.status_callback)
    {
        client->config.status_callback("Connected to server");
    }

    return ERR_OK;
}

static err_t tcp_client_recv_callback(void *arg, struct tcp_pcb *tcp_pcb, struct pbuf *p, err_t err)
{
    tcp_client_t *client = (tcp_client_t *)arg;

    if (!p)
    {
        printf("[TCP_CLIENT] Connection closed by server\n");
        client->complete = true;
        client->round_trip_time_ms = absolute_time_diff_us(client->start_time, get_absolute_time()) / 1000;
        return ERR_OK;
    }

    if (p->tot_len > 0)
    {
        size_t available_space = sizeof(client->response_buffer) - client->response_length - 1;
        size_t copy_length = (p->tot_len < available_space) ? p->tot_len : available_space;

        if (copy_length > 0)
        {
            pbuf_copy_partial(p, client->response_buffer + client->response_length, copy_length, 0);
            client->response_length += copy_length;
            client->response_buffer[client->response_length] = '\0'; // Null-terminate
        }

        printf("[TCP_CLIENT] Received %u bytes (total: %zu)\n", (unsigned)copy_length, client->response_length);

        // Very simple success detection for now
        if (strstr(client->response_buffer, "OK") || strstr(client->response_buffer, "200"))
        {
            client->success = true;
        }

        if (client->config.status_callback)
        {
            char status_msg[64];
            snprintf(status_msg, sizeof(status_msg), "Received %zu bytes", client->response_length);
            client->config.status_callback(status_msg);
        }
    }

    tcp_recved(tcp_pcb, p->tot_len);
    pbuf_free(p);

    return ERR_OK;
}

static void tcp_client_error_callback(void *arg, err_t err)
{
    tcp_client_t *client = (tcp_client_t *)arg;

    printf("[TCP_CLIENT] TCP error: %d\n", err);

    client->success = false;
    client->complete = true;

    if (client->config.status_callback)
    {
        char status_msg[64];
        snprintf(status_msg, sizeof(status_msg), "TCP error: %d", err);
        client->config.status_callback(status_msg);
    }
}

static err_t tcp_client_sent_callback(void *arg, struct tcp_pcb *tcp_pcb, u16_t len)
{
    printf("[TCP_CLIENT] Sent %u bytes\n", (unsigned)len);
    return ERR_OK;
}

tcp_client_t *tcp_client_create(const tcp_client_config_t *config)
{
    if (!config || !config->server_ip[0] || config->server_port == 0)
    {
        printf("[TCP_CLIENT] Invalid configuration\n");
        return NULL;
    }

    tcp_client_t *client = malloc(sizeof(tcp_client_t));
    if (!client)
    {
        printf("[TCP_CLIENT] Memory allocation failed\n");
        return NULL;
    }

    memcpy(&client->config, config, sizeof(tcp_client_config_t));

    // Set defaults for unspecified timeouts
    if (client->config.connect_timeout_ms == 0)
    {
        client->config.connect_timeout_ms = TCP_CLIENT_DEFAULT_CONNECT_TIMEOUT_MS;
    }
    if (client->config.response_timeout_ms == 0)
    {
        client->config.response_timeout_ms = TCP_CLIENT_DEFAULT_RESPONSE_TIMEOUT_MS;
    }

    if (!ip4addr_aton(client->config.server_ip, &client->server_addr))
    {
        printf("[TCP_CLIENT] Invalid server IP: %s\n", client->config.server_ip);
        free(client);
        return NULL;
    }

    printf("[TCP_CLIENT] Server IP: %s, Port: %u\n", client->config.server_ip, client->config.server_port);

    return client;
}

bool tcp_client_wifi_ready(void)
{
    return cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA) == CYW43_LINK_UP;
}

int tcp_client_send(tcp_client_t *client,
                    const void *data,
                    size_t data_length,
                    tcp_client_response_t *response)
{
    if (!client || !data || data_length == 0 || !response)
    {
        return TCP_CLIENT_ERROR_INVALID;
    }

    if (!tcp_client_wifi_ready())
    {
        printf("[TCP_CLIENT] WiFi not ready\n");
        return TCP_CLIENT_ERROR_WIFI;
    }

    // Initialize response
    memset(response, 0, sizeof(tcp_client_response_t));

    // Reset client state
    client->connected = false;
    client->complete = false;
    client->success = false;
    client->response_length = 0;
    memset(client->response_buffer, 0, sizeof(client->response_buffer));
    client->start_time = get_absolute_time();

    if (client->config.status_callback)
    {
        client->config.status_callback("Connecting to server...");
    }

    // Create new TCP PCB
    client->pcb = tcp_new();
    if (!client->pcb)
    {
        printf("[TCP_CLIENT] Failed to create TCP PCB\n");
        return TCP_CLIENT_ERROR_MEMORY;
    }

    // Set up callbacks
    tcp_arg(client->pcb, client);
    tcp_err(client->pcb, tcp_client_error_callback);
    tcp_recv(client->pcb, tcp_client_recv_callback);
    tcp_sent(client->pcb, tcp_client_sent_callback);

    // Connect to server
    cyw43_arch_lwip_begin();
    err_t err = tcp_connect(client->pcb, &client->server_addr, client->config.server_port, tcp_client_connected_callback);
    cyw43_arch_lwip_end();

    if (err != ERR_OK) {
        printf("[TCP_CLIENT] Connect initiation failed: %d\n", err);
        tcp_close(client->pcb);
        return TCP_CLIENT_ERROR_CONNECT;
    }

    // Wait for connection or timeout
    absolute_time_t connect_timeout = make_timeout_time_ms(client->config.connect_timeout_ms);
    while (!client->connected && !client->complete && !time_reached(connect_timeout)) {
        cyw43_arch_poll();
        sleep_ms(10);
    }

    if (!client->connected) {
        printf("[TCP_CLIENT] Connection timeout\n");
        tcp_close(client->pcb);
        return TCP_CLIENT_ERROR_TIMEOUT;
    }

    if (client->config.status_callback) {
        client->config.status_callback("Sending data...");
    }

    // Send data
    cyw43_arch_lwip_begin();
    err = tcp_write(client->pcb, data, data_length, TCP_WRITE_FLAG_COPY);
    if (err == ERR_OK) {
        tcp_output(client->pcb);
    }
    cyw43_arch_lwip_end();

    if (err != ERR_OK) {
        printf("[TCP_CLIENT] Write failed: %d\n", err);
        tcp_close(client->pcb);
        return TCP_CLIENT_ERROR_SEND;
    }

    // Wait for response or timeout
    absolute_time_t response_timeout = make_timeout_time_ms(client->config.response_timeout_ms);
    while (!client->complete && !time_reached(response_timeout)) {
        cyw43_arch_poll();
        sleep_ms(10);
    }

    // Close connection
    tcp_close(client->pcb);

    if (!client->complete) {
        printf("[TCP_CLIENT] Response timeout\n");
        return TCP_CLIENT_ERROR_TIMEOUT;
    }

    // Fill response structure
    response->success = client->success;
    response->error_code = client->success ? TCP_CLIENT_SUCCESS : TCP_CLIENT_ERROR_RECEIVE;
    strncpy(response->response_data, client->response_buffer, sizeof(response->response_data) - 1);
    response->response_data[sizeof(response->response_data) - 1] = '\0'; // Ensure null-termination
    response->response_length = client->response_length;
    response->round_trip_time_ms = client->round_trip_time_ms;

    printf("[TCP_CLIENT] Request complete: %s, %zu bytes received in %u ms\n",
           response->success ? "Success" : "Failure",
           response->response_length,
           response->round_trip_time_ms);

    return response->error_code;
}

int tcp_client_send_json(tcp_client_t *client,
                         const char *json_data,
                         tcp_client_response_t *response)
{
    if (!json_data) {
        return TCP_CLIENT_ERROR_INVALID;
    }

    // TODO: JSON validation could be added here

    return tcp_client_send(client, json_data, strlen(json_data), response);
}

const char* tcp_client_error_string(int error_code) {
    int index = -error_code;
    if (index >= 0 && index < (int)(sizeof(error_messages) / sizeof(error_messages[0]))) {
        return error_messages[index];
    }
    return "Unknown error";
}

void tcp_client_destroy(tcp_client_t *client)
{
    if (client) {
        printf("[TCP_CLIENT] Starting destroy\n");
        if (client->pcb) {
            printf("[TCP_CLIENT] Aborting TCP PCB\n");
            tcp_abort(client->pcb);
            client->pcb = NULL; // Prevent double free
        }
        printf("[TCP_CLIENT] Freeing client memory\n");
        free(client);
        printf("[TCP_CLIENT] Client destroyed\n");
    }
}   