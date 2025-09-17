#pragma once

#include <stdbool.h>

typedef struct {
    uint8_t gate_pin;   // GPIO pin controlling the MOSFET gate
    bool start_enabled; // Whether to start with power enabled
} peripheral_power_config_t;

typedef struct {
    peripheral_power_config_t config;
    bool initialized;
    bool power_enabled;
} peripheral_power_t;

/**
 * @brief Create a peripheral power configuration
 * 
 * @param gate_pin GPIO pin connected to the MOSFET gate
 * @return peripheral_power_config_t Configuration struct
 */
void peripheral_power_create_config(peripheral_power_config_t *config, uint8_t gate_pin, bool start_enabled);

/**
 * @brief Initialize the peripheral power management system
 * 
 * Sets up the MOSFET gate control pin. Call this once during system startup.
 * Starts with power enabled.
 */
void peripheral_power_init(peripheral_power_t *power, const peripheral_power_config_t *config);

/**
 * @brief Enable power to peripherals
 * 
 * Turns on the P-channel MOSFET by setting gate LOW.
 * 
 * @returns 1 if power was successfully enabled, 0 if already enabled, -1 if not initialized
 */
bool peripheral_power_enable(peripheral_power_t *power);

/**
 * @brief Disable power to peripherals
 * 
 * Turns off the P-channel MOSFET by setting gate HIGH.
 * 
 * @returns 1 if power was successfully disabled, 0 if already disabled, -1 if not initialized
 */
bool peripheral_power_disable(peripheral_power_t *power);