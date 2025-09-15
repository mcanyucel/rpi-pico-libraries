#ifndef PERIPHERAL_POWER_H
#define PERIPHERAL_POWER_H

#ifndef MOSFET_GATE_PIN
#define MOSFET_GATE_PIN 17 // GPIO17 to control MOSFET gate
#endif // MOSFET_GATE_PIN

#include "pico/stdlib.h"

/**
 * @brief Initialize the peripheral power management system
 * 
 * Sets up the MOSFET gate control pin. Call this once during system startup.
 * Starts with power enabled.
 */
void peripheral_power_init(bool start_enabled);

/**
 * @brief Enable power to peripherals
 * 
 * Turns on the P-channel MOSFET by setting gate LOW.
 * 
 * @returns 1 if power was successfully enabled, 0 if already enabled, -1 if not initialized
 */
int peripheral_power_enable(void);

/**
 * @brief Disable power to peripherals
 * 
 * Turns off the P-channel MOSFET by setting gate HIGH.
 * 
 * @returns 1 if power was successfully disabled, 0 if already disabled, -1 if not initialized
 */
int peripheral_power_disable(void);

/**
 * @brief Check if peripheral power is currently enabled
 * 
 * @return true if power is enabled, false if disabled
 */
bool peripheral_power_is_enabled(void);

#endif // PERIPHERAL_POWER_H