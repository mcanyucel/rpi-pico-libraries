/**
 * @file ble_nordic_uart.c
 * @author
 * @brief Nordic UART Service (NUS) implementation using BTstack on CYW43
 * @version 0.1
 * @date 2025-10-08
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "ble_nordic_uart.h"
#include "btstack.h"
#include "pico/cyw43_arch.h"
#include "pico/btstack_cyw43.h"
#include <stdio.h>
#include <string.h>

// Include generated GATT database
#include "nordic_uart.h"

// ============================================================================
// PRIVATE CONSTANTS
// ============================================================================

#define ADV_DATA_MAX_SIZE 31

// ============================================================================
// PRIVATE TYPES
// ============================================================================

/**
 * @brief Internal driver context
 *
 */
typedef struct
{
    ble_uart_state_t state;
    bool notifications_enabled;
    hci_con_handle_t connection_handle;
    ble_uart_connection_callback_t connection_callback;
    char device_name[BLE_UART_MAX_DEVICE_NAME_LENGTH];
    uint8_t message_buffer[BLE_UART_MAX_MESSAGE_LENGTH];
    uint16_t message_length;
    uint8_t adv_data[ADV_DATA_MAX_SIZE];
    uint8_t adv_data_len;
} ble_uart_context_t;

// ============================================================================
// PRIVATE VARIABLES
// ============================================================================

static ble_uart_context_t ble_ctx = {
    .state = BLE_UART_DISABLED,
    .notifications_enabled = false,
    .connection_handle = 0,
    .connection_callback = NULL,
    .device_name = {0},
    .message_buffer = {0},
    .message_length = 0,
    .adv_data = {0},
    .adv_data_len = 0
};

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
static uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle,
                                  uint16_t offset, uint8_t *buffer, uint16_t buffer_size);
static int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle,
                              uint16_t transaction_mode, uint16_t offset,
                              uint8_t *buffer, uint16_t buffer_size);
static void setup_advertising(void);

// ============================================================================
// PRIVATE HELPER FUNCTIONS
// ============================================================================

static void setup_advertising(void) {
    // Build advertising data with device name
    ble_ctx.adv_data_len = 0;  // Use context buffer instead of local
    
    // Flags: General Discoverable, BR/EDR not supported
    ble_ctx.adv_data[ble_ctx.adv_data_len++] = 2;    // Length
    ble_ctx.adv_data[ble_ctx.adv_data_len++] = 0x01; // Type: Flags
    ble_ctx.adv_data[ble_ctx.adv_data_len++] = 0x06; // Value
    
    // Complete Local Name
    uint8_t name_len = strlen(ble_ctx.device_name);
    printf("[BLE UART] Device name length: %d\n", name_len);
    printf("[BLE UART] Device name: '%s'\n", ble_ctx.device_name);
    
    if (name_len > 0 && (ble_ctx.adv_data_len + name_len + 2) <= ADV_DATA_MAX_SIZE) {
        ble_ctx.adv_data[ble_ctx.adv_data_len++] = name_len + 1;  // Length
        ble_ctx.adv_data[ble_ctx.adv_data_len++] = 0x09;          // Type: Complete Local Name
        memcpy(&ble_ctx.adv_data[ble_ctx.adv_data_len], ble_ctx.device_name, name_len);
        ble_ctx.adv_data_len += name_len;
        
        printf("[BLE UART] Added name to adv data, total length: %d\n", ble_ctx.adv_data_len);
    }
    
    // Debug: Print advertising data bytes
    printf("[BLE UART] Advertising data (%d bytes): ", ble_ctx.adv_data_len);
    for (int i = 0; i < ble_ctx.adv_data_len; i++) {
        printf("%02X ", ble_ctx.adv_data[i]);
    }
    printf("\n");
    
    // Set advertising parameters
    uint16_t adv_int_min = 0x0030;  // 30ms
    uint16_t adv_int_max = 0x0030;  // 30ms
    uint8_t adv_type = 0;           // Connectable undirected
    bd_addr_t null_addr;
    memset(null_addr, 0, 6);
    
    printf("[BLE UART] Setting advertising parameters...\n");
    gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
    
    printf("[BLE UART] Setting advertising data...\n");
    gap_advertisements_set_data(ble_ctx.adv_data_len, ble_ctx.adv_data);
    
    // Also set as scan response data (belt and suspenders approach)
    printf("[BLE UART] Setting scan response data...\n");
    gap_scan_response_set_data(ble_ctx.adv_data_len, ble_ctx.adv_data);
    
    printf("[BLE UART] Enabling advertisements...\n");
    gap_advertisements_enable(1);
    
    printf("[BLE UART] Advertising started with name: %s\n", ble_ctx.device_name);
}

// ============================================================================
// PACKET HANDLER (BLE Events)
// ============================================================================

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    UNUSED(channel);
    UNUSED(size);

    if (packet_type != HCI_EVENT_PACKET)
        return; // Only handle HCI events

    uint8_t event = hci_event_packet_get_type(packet);

    switch (event)
    {
    case BTSTACK_EVENT_STATE:
    {
        uint8_t state = btstack_event_state_get_state(packet);

        if (state == HCI_STATE_WORKING)
        {
            printf("[BLE UART] BTstack is ready\n");
            ble_ctx.state = BLE_UART_ADVERTISING;
            setup_advertising();
        }
        break;
    }

    case HCI_EVENT_DISCONNECTION_COMPLETE:
    {
        printf("[BLE UART] Client disconnected\n");
        ble_ctx.state = BLE_UART_ADVERTISING;
        ble_ctx.notifications_enabled = false;
        ble_ctx.connection_handle = 0;

        // Call user callback
        if (ble_ctx.connection_callback)
        {
            ble_ctx.connection_callback(false);
        }

        // Restart advertising
        gap_advertisements_enable(1);
        break;
    }

    case ATT_EVENT_CONNECTED:
    {
        printf("[BLE UART] Client connected\n");
        ble_ctx.state = BLE_UART_CONNECTED;
        ble_ctx.connection_handle = att_event_connected_get_handle(packet);
        // Notifications are not enabled by default - wait for client to enable before calling callback
        break;
    }

    case ATT_EVENT_MTU_EXCHANGE_COMPLETE:
    {
        printf("BLE UART: MTU = %u bytes\n",
               att_event_mtu_exchange_complete_get_MTU(packet));
        break;
    }

    case ATT_EVENT_CAN_SEND_NOW:
    {
        // Ready to send more data if needed
        break;
    }

    // Handle other events as needed:
    case HCI_EVENT_COMMAND_COMPLETE:
    case HCI_EVENT_LE_META:
    case HCI_EVENT_NUMBER_OF_COMPLETED_PACKETS:
    case 0x06E: // HCI_EVENT_LE_META (alternative)
    case 0xE7: // HCI_EVENT_HANDLE_VALUE_INDICATION_COMPLETE
    case 0x61: // ATT MTU exchange event
    case 0xFF: // Vendor-specific event
        break;

    default:
        printf("[BLE UART] Unhandled event: 0x%02X\n", event);
        break;
    }
}

// ============================================================================
// ATT CALLBACKS (GATT Read/Write Operations)
// ============================================================================

static uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle,
                                  uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    UNUSED(connection_handle);

    if (att_handle == ATT_CHARACTERISTIC_6E400003_B5A3_F393_E0A9_E50E24DCCA9E_01_VALUE_HANDLE)
    {
        uint16_t bytes_to_copy = (ble_ctx.message_length - offset);
        if (bytes_to_copy > buffer_size)
        {
            bytes_to_copy = buffer_size;
        }
        if (bytes_to_copy > 0)
        {
            memcpy(buffer, &ble_ctx.message_buffer[offset], bytes_to_copy);
        }
        return bytes_to_copy;
    }

    return 0; // No other readable characteristics
}

static int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle,
                              uint16_t transaction_mode, uint16_t offset,
                              uint8_t *buffer, uint16_t buffer_size)
{
    UNUSED(connection_handle);
    UNUSED(transaction_mode);
    UNUSED(offset);

    if (att_handle == ATT_CHARACTERISTIC_6E400003_B5A3_F393_E0A9_E50E24DCCA9E_01_CLIENT_CONFIGURATION_HANDLE)
    {
        if (buffer_size >= 2)
        {
            uint16_t config = little_endian_read_16(buffer, 0);
            bool was_enabled = ble_ctx.notifications_enabled;
            ble_ctx.notifications_enabled = (config & 0x0001) != 0;

            printf("[BLE UART] Notifications %s\n", ble_ctx.notifications_enabled ? "enabled" : "disabled");

            // Call user callback when notifications are first enabled
            if (!was_enabled && ble_ctx.notifications_enabled && ble_ctx.connection_callback)
            {
                ble_ctx.connection_callback(true);
            }
        }
        return 0; // Success
    }

    // Handle RX characteristic writes (incoming data) - NOT IMPLEMENTED. E.g. for commands:
    // if (att_handle == ATT_CHARACTERISTIC_6E400002_B5A3_F393_E0A9_E50E24DCCA9E_01_VALUE_HANDLE) {
    //     // RX data received - could implement command handling here if needed
    // }

    return 0; // Success for unhandled writes
}

// ============================================================================
// PUBLIC API IMPLEMENTATION
// ============================================================================

bool ble_nordic_uart_init(const char *device_name)
{
    if (!device_name || strlen(device_name) == 0 || strlen(device_name) >= BLE_UART_MAX_DEVICE_NAME_LENGTH)
    {
        printf("[BLE UART] Invalid device name\n");
        return false;
    }

    printf("[BLE UART] Initializing with device name: %s\n", device_name);

    // Copy device name
    strncpy(ble_ctx.device_name, device_name, BLE_UART_MAX_DEVICE_NAME_LENGTH - 1);
    ble_ctx.device_name[BLE_UART_MAX_DEVICE_NAME_LENGTH - 1] = '\0'; // Ensure null-termination

    ble_ctx.state = BLE_UART_INITIALIZING;

    // Initialize BTstack layers
    l2cap_init();
    sm_init();

    att_server_init(profile_data, att_read_callback, att_write_callback);
    printf("[BLE UART] GATT database initialized\n");

    static btstack_packet_callback_registration_t callback_registration;
    callback_registration.callback = &packet_handler;
    hci_add_event_handler(&callback_registration);

    att_server_register_packet_handler(&packet_handler);

    hci_power_control(HCI_POWER_ON);

    printf("[BLE UART] BTstack initialization complete - waiting for BTstack ready\n");
    return true;
}

bool ble_nordic_uart_send(const char *message)
{
    if (!ble_nordic_uart_is_connected())
    {
        return false;
    }

    if (!message)
    {
        return false;
    }

    size_t msg_len = strlen(message); // exclude null terminator
    if (msg_len == 0)
    {
        return false;
    }

    if (msg_len > BLE_UART_MAX_MESSAGE_LENGTH - 1)
    {
        printf("[BLE UART] Message too long, truncating to %d bytes\n", BLE_UART_MAX_MESSAGE_LENGTH - 1);
        msg_len = BLE_UART_MAX_MESSAGE_LENGTH - 1;
    }

    memcpy(ble_ctx.message_buffer, message, msg_len);
    ble_ctx.message_length = msg_len;

    att_server_notify(ble_ctx.connection_handle,
                      ATT_CHARACTERISTIC_6E400003_B5A3_F393_E0A9_E50E24DCCA9E_01_VALUE_HANDLE,
                      ble_ctx.message_buffer,
                      ble_ctx.message_length);

    return true;
}

ble_uart_state_t ble_nordic_uart_get_state(void)
{
    return ble_ctx.state;
}

bool ble_nordic_uart_is_connected(void) {
    return (ble_ctx.state == BLE_UART_CONNECTED && ble_ctx.notifications_enabled);
}

void ble_nordic_uart_set_connection_callback(ble_uart_connection_callback_t callback)
{
    ble_ctx.connection_callback = callback;
}

const char* ble_nordic_uart_get_state_name(ble_uart_state_t state) {
    switch (state) {
        case BLE_UART_DISABLED:      return "DISABLED";
        case BLE_UART_INITIALIZING:  return "INITIALIZING";
        case BLE_UART_ADVERTISING:   return "ADVERTISING";
        case BLE_UART_CONNECTED:     return "CONNECTED";
        default:                     return "UNKNOWN";
    }
}

void ble_nordic_uart_stop(void)
{
    if (ble_ctx.state == BLE_UART_DISABLED) {
        return;
    }

    printf("[BLE UART] Stopping BLE and disabling advertising...\n");

    gap_advertisements_enable(0);

    if (ble_ctx.connection_handle)
    {
        gap_disconnect(ble_ctx.connection_handle);
    }

    // Reset state
    ble_ctx.state = BLE_UART_DISABLED;
    ble_ctx.notifications_enabled = false;
    ble_ctx.connection_handle = 0;

    printf("[BLE UART] Stopped.\n");
}