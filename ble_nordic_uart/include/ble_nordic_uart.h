/**
 * @file ble_nordic_uart.h
 * @author 
 * @brief Nordic UART Service (NUS) BLE driver for Raspberry Pi Pico W and 2W
 * @version 0.1
 * @date 2025-10-07
 * 
 * Bluetooth Low Energy (BLE) driver implementing Nordic UART Service for simple 
 * wireless data transmission. Provides one-way data transmission from Pico to
 * connected BLE clients (e.g. smartphones, tablets).
 * 
 * Nordic UART Service UUIDs:
 * - Service:  6E400001-B5A3-F393-E0A9-E50E24DCCA9E
 * - TX Char:  6E400003-B5A3-F393-E0A9-E50E24DCCA9E (Notifications)
 * - RX Char:  6E400002-B5A3-F393-E0A9-E50E24DCCA9E (Write - Not implemented)
 * 
 * Features:
 * - Simple string-based API for sending data
 * - Automatic advertising with custom device name
 * - Connection state callbacks for UI updates
 * - Compatible with Nordic UART apps on iOS/Android (e.g. nRF Connect)
 * 
 * Requirements:
 * - Pico W or 2W with CYW43 wireless chip
 * - pico_cyw43_arch_lwip_poll or pico_cyw43_arch_lwip_threadsafe_background
 * - pico_btstack_ble library
 * - Must call cyw43_arch_init() before ble_nordic_uart_init()
 * - Must call cyw43_arch_poll() in main loop (poll mode only)
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// CONFIGURATION CONSTANTS
// ============================================================================

#ifndef BLE_UART_MAX_MESSAGE_LENGTH
#define BLE_UART_MAX_MESSAGE_LENGTH  128      ///< Maximum length of a single BLE message (in bytes)
#endif

#ifndef BLE_UART_MAX_DEVICE_NAME_LENGTH
#define BLE_UART_MAX_DEVICE_NAME_LENGTH  32   ///< Maximum length of the BLE device name (in bytes)
#endif

// ============================================================================
// TYPE DEFINITIONS
// ============================================================================

/**
 * @brief BLE UART connection states
 * 
 */
typedef enum {
    BLE_UART_DISABLED,      ///< BLE not initialized
    BLE_UART_INITIALIZING,  ///< BLE starting up
    BLE_UART_ADVERTISING,   ///< BLE advertising / waiting for connection
    BLE_UART_CONNECTED      ///< BLE client connected and ready
} ble_uart_state_t;

/**
 * @brief Connection event callback function type.
 * 
 * Called when BLE connection state changes (connected/disconnected).
 * 
 * @param connected True if connected, false if disconnected
 */
typedef void (*ble_uart_connection_callback_t)(bool connected);

// ============================================================================
// CORE FUNCTIONS
// ============================================================================

/**
 * @brief Initialize BLE Nordic UART Service with specified device name.
 * 
 * Initializes BTstack and begins advertising with the specified device name. 
 * Must be called after cyw43_arch_init() and before any other BLE functions.
 * 
 * @param device_name Device name to use for advertising (max length BLE_UART_MAX_DEVICE_NAME_LENGTH)
 * @return true if initialization successful, false on error
 * 
 * @note Automatically starts advertising when BTstack is ready.
 * @note Device name appears in BLE scans and connection dialogs.
 */
bool ble_nordic_uart_init(const char *device_name);

/**
 * @brief Send string message via BLE
 * 
 * Sends null-terminated string to connected BLE client via notifications.
 * Non-blocking - returns immediately.
 * 
 * @param message Null-terminated string to send (max length BLE_UART_MAX_MESSAGE_LENGTH)
 * @return true if message sent successfully, false if not connected or error
 * 
 * @note Returns false if no client is connected or notifications are not enabled.
 * @note Message is truncated if longer than BLE_UART_MAX_MESSAGE_LENGTH.
 */
bool ble_nordic_uart_send(const char *message);

/**
 * @brief Get current BLE connection state
 * 
 * @return Current state (DISABLED, INITIALIZING, ADVERTISING, CONNECTED)
 */
ble_uart_state_t ble_nordic_uart_get_state(void);

/**
 * @brief Check if BLE client is connected and ready for data
 * 
 * @return true if connected AND notifications enabled, false otherwise
 * 
 * @note Being connected alone is not sufficient - notifications must be enabled by the client.
 */
bool ble_nordic_uart_is_connected(void);

/**
 * @brief Register callback for connection events
 * 
 * Sets a callback function that will be called whenever a client connects or disconnects.
 * It is useful for updating UI or auto-starting data streaming.
 * 
 * @param callback Function to call on connection state changes, or NULL to disable 
 
 * @example
 * void on_ble_connection_changed(bool connected) {
 *     if (connected) {
 *         printf("Phone connected!\n");
 *         start_data_streaming();
 *     } else {
 *         printf("Phone disconnected\n");
 *         stop_data_streaming();
 *     }
 * }
 * ble_nordic_uart_set_connection_callback(on_ble_connection_changed);
 */
void ble_nordic_uart_set_connection_callback(ble_uart_connection_callback_t callback);


// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * @brief Get human-readable name for BLE state
 * 
 * @param state BLE state enum value
 * @return Human-readable string name
 */
const char* ble_nordic_uart_get_state_name(ble_uart_state_t state);

/**
 * @brief Stop BLE and disable advertising
 * 
 * Disconnects any connected clients and stops advertising.
 * Can be restarted by calling ble_nordic_uart_init() again.
 * 
 */
void ble_nordic_uart_stop(void);

// ============================================================================
// INLINE HELPER FUNCTIONS
// ============================================================================

/**
 * @brief Check if BLE is currently advertising
 * 
 * @return true if advertising, false otherwise
 */
static inline bool ble_nordic_uart_is_advertising(void) {
    return ble_nordic_uart_get_state() == BLE_UART_ADVERTISING;
}

/**
 * @brief Check if BLE is ready for data transmission
 * 
 * @return true if ready, false otherwise
 */
static inline bool ble_nordic_uart_ready(void) {
    return ble_nordic_uart_is_connected();
}