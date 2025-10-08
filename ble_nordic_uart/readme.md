# BLE Nordic UART Service Driver for Raspberry Pi Pico W

## Overview

This driver provides a simple, robust Bluetooth Low Energy (BLE) communication interface for Raspberry Pi Pico W/2W using the Nordic UART Service (NUS) protocol. It enables wireless data transmission from the Pico to mobile devices (Android/iOS) or computers with minimal configuration and clean API design.

The driver implements one-way communication (Pico → Client) optimized for sensor data streaming, logging, and real-time monitoring applications. It handles all low-level BTstack complexity, automatic advertising, connection management, and GATT profile configuration, exposing only a simple string-based API for application developers.

## Why Nordic UART Service?

Nordic UART Service (NUS) is a de facto standard for simple BLE communication, widely supported across platforms with well-documented implementations. Despite being created by Nordic Semiconductor, the service uses public UUIDs and can be freely implemented by anyone. This makes it ideal for:

- **Rapid prototyping** - Test with existing apps like nRF Connect before writing custom mobile apps
- **Cross-platform compatibility** - Easy to implement in Android, iOS, React Native, Flutter, etc.
- **Simple data model** - Treats BLE like a wireless serial port (UART over BLE)
- **Proven reliability** - Used in thousands of commercial products worldwide

Unlike proprietary BLE profiles, NUS requires no special licensing or hardware dependencies. Any BLE client can connect and receive notifications using standard GATT operations.

## Features

- **Simple API** - Send data with single function call: `ble_nordic_uart_send("data")`
- **Automatic advertising** - Device appears with custom name in BLE scanners
- **Connection callbacks** - React to connect/disconnect events for UI updates
- **Zero-copy operation** - Efficient memory usage with persistent buffers
- **State machine** - Clear connection states (Disabled, Initializing, Advertising, Connected)
- **Standard compliant** - Uses official Nordic UART Service UUIDs for maximum compatibility
- **Mobile-friendly** - Works with nRF Connect, LightBlue, and custom apps
- **Encapsulated configuration** - Self-contained BTstack config, no global conflicts

## Hardware Requirements

- **Board**: Raspberry Pi Pico W or Pico 2 W (requires CYW43439 wireless chip)
- **SDK**: Pico SDK 2.0.0 or later with BTstack support
- **Dependencies**: 
  - `pico_btstack_ble` - BTstack BLE library
  - `pico_btstack_cyw43` - BTstack CYW43 integration
  - `pico_cyw43_arch_lwip_poll` - CYW43 driver (poll or threadsafe_background mode)

## Quick Start

### 1. Initialize CYW43 (Required First)
```c
#include "pico/cyw43_arch.h"

// In main(), before BLE init
if (cyw43_arch_init()) {
    printf("Failed to initialize wireless\n");
    return -1;
}
```

### 2. Initialize BLE Driver
```c
#include "ble_nordic_uart.h"

// Initialize with device name (appears in BLE scanners)
ble_nordic_uart_init("MyDevice");
```

### 3. Send Data
```c
// Send string data (CSV, JSON, plain text, etc.)
char message[64];
snprintf(message, sizeof(message), "Temperature: %.1f C\n", temp);
ble_nordic_uart_send(message);
```

### 4. Poll CYW43 (Required in Main Loop)
```c
while (true) {
    cyw43_arch_poll();  // Process BLE/WiFi events
    
    if (ble_nordic_uart_is_connected()) {
        // Send data at your desired rate
        ble_nordic_uart_send(sensor_data);
    }
    
    sleep_ms(100);
}
```

## API Reference

### Core Functions

```c
// Initialize BLE and start advertising
bool ble_nordic_uart_init(const char *device_name);

// Send string message to connected client
bool ble_nordic_uart_send(const char *message);

// Check if client is connected and ready
bool ble_nordic_uart_is_connected(void);

// Get current connection state
ble_uart_state_t ble_nordic_uart_get_state(void);

// Register callback for connection events
void ble_nordic_uart_set_connection_callback(ble_uart_connection_callback_t callback);
```

### Connection States

```c
typedef enum {
    BLE_UART_DISABLED,      // Not initialized
    BLE_UART_INITIALIZING,  // Starting up
    BLE_UART_ADVERTISING,   // Waiting for connection
    BLE_UART_CONNECTED      // Client connected and notifications enabled
} ble_uart_state_t;
```

## Example: Connection Callback

```c
void on_ble_connection(bool connected) {
    if (connected) {
        printf("Phone connected - starting data stream\n");
        start_sensor_logging();
    } else {
        printf("Phone disconnected - stopping stream\n");
        stop_sensor_logging();
    }
}

// Register callback
ble_nordic_uart_set_connection_callback(on_ble_connection);
```

## Mobile App Integration

### Testing with nRF Connect (Android/iOS)

1. Open nRF Connect app
2. Scan for devices - your device name will appear
3. Connect to device
4. Expand "Nordic UART Service"
5. Tap the TX characteristic and enable notifications (↓ icon)
6. Data will appear in real-time as sent by Pico

### Custom App Implementation

**Android (Java/Kotlin)**:
- Use `BluetoothGatt` API with service UUID `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`
- Subscribe to TX characteristic `6E400003-B5A3-F393-E0A9-E50E24DCCA9E`

**iOS (Swift)**:
- Use `CoreBluetooth` framework
- Search for peripheral with service UUID
- Subscribe to TX characteristic for notifications

**Cross-platform**:
- React Native: `react-native-ble-manager` or `react-native-ble-plx`
- Flutter: `flutter_blue_plus` package

All platforms support Nordic UART Service with extensive documentation and examples available online.

## Performance Characteristics

- **Throughput**: 5-10 KB/s typical (depends on connection interval and MTU)
- **Latency**: 7.5-30ms per packet (connection interval dependent)
- **Max message size**: 128 bytes per call (configurable via `BLE_UART_MAX_MESSAGE_LENGTH`)
- **Recommended data rate**: Up to 50 messages/second for smooth real-time streaming
- **Connection range**: 10-30 meters line-of-sight (typical for BLE Class 2 devices)

## Integration Notes

### CMake Integration

Add to your project's `CMakeLists.txt`:

```cmake
add_subdirectory(drivers/ble_nordic_uart)

target_link_libraries(your_project
    ble_nordic_uart
    # ... other libraries
)
```

### Poll vs Background Mode

**Poll Mode** (more control):
```cmake
pico_cyw43_arch_lwip_poll
```
Requires `cyw43_arch_poll()` in main loop.

**Background Mode** (automatic):
```cmake
pico_cyw43_arch_lwip_threadsafe_background
```
Events processed in interrupt, no manual polling needed.

## Limitations

- **One-way communication**: RX characteristic exists but not implemented (send only, no receive)
- **Single connection**: Only one client can connect at a time
- **No pairing/bonding**: Connections are not persistent across power cycles
- **No encryption**: Data transmitted in plaintext (add security if needed)

These limitations keep the driver simple and focused. RX capability and security features can be added if needed for specific applications.