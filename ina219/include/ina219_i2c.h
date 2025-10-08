/**
 * @file ina219_i2c.h
 * @author Mustafa Can Yucel via Claude 4 Sonnet (mustafacan@bridgewiz.com)
 * @brief INA219 I2C Power Monitor Driver Header
 * @version 0.1
 * @date 2025-08-20
 *
 * INA219 power monitor driver for various battery configurations.
 * Supports configurable parameters for current monitoring and battery percentage calculation.
 *
 * @copyright Copyright (c) 2025
 */

#ifndef INA219_I2C_H
#define INA219_I2C_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"

// Default I2C configuration
#ifndef INA219_I2C_ADDRESS
#define INA219_I2C_ADDRESS 0x40 // Default I2C address
#endif                          // INA219_I2C_ADDRESS

#ifndef INA219_I2C_SDA_PIN
#define INA219_I2C_SDA_PIN 0 // GP0
#endif                       // INA219_I2C_SDA_PIN

#ifndef INA219_I2C_SCL_PIN
#define INA219_I2C_SCL_PIN 1 // GP1
#endif                       // INA219_I2C_SCL_PIN

#ifndef INA219_I2C_CLK
#define INA219_I2C_CLK 100000 // 100 kHz
#endif                        // INA219_I2C_CLK

// INA219 Register addresses
#define INA219_REG_CONFIG 0x00
#define INA219_REG_SHUNT_VOLTAGE 0x01
#define INA219_REG_BUS_VOLTAGE 0x02
#define INA219_REG_POWER 0x03
#define INA219_REG_CURRENT 0x04
#define INA219_REG_CALIBRATION 0x05

// Configuration bit masks
#define INA219_CONFIG_RESET 0x8000
#define INA219_CONFIG_BUS_16V 0x0000
#define INA219_CONFIG_BUS_32V 0x2000
#define INA219_CONFIG_SHUNT_320MV 0x1800
#define INA219_CONFIG_ADC_12BIT 0x0400
#define INA219_CONFIG_ADC_12BIT_S 0x0200
#define INA219_CONFIG_MODE_CONT 0x0007

// Battery pack types
typedef enum
{
    INA219_BATTERY_1S = 0,
    INA219_BATTERY_CUSTOM,
} ina219_battery_type_t;

// Battery status
typedef enum
{
    INA219_BATTERY_CRITICAL = 0,
    INA219_BATTERY_LOW,
    INA219_BATTERY_FAIR,
    INA219_BATTERY_GOOD,
    INA219_BATTERY_EXCELLENT
} ina219_battery_status_t;

// Battery configuration structure
typedef struct
{
    uint8_t cell_count;        // Number of cells in series (Always 1 for 1SnP)
    uint8_t parallel_count;    // Number of cells in parallel (n in 1SnP)
    float nominal_voltage;     // Nominal voltage of the battery pack (typically 3.7V for Li-ion)
    float max_voltage;         // Maximum voltage of the battery pack (typically 4.2V for Li-ion)
    float min_voltage;         // Minimum voltage of the battery pack (typically 3.0V for Li-ion)
    uint8_t bus_voltage_range; // Bus voltage range setting - 16V or 32V
    float capacity_mah;        // Battery capacity in mAh
    const char *config_name;   // Configuration name
} ina219_battery_config_t;

// INA219 device structure
typedef struct
{
    i2c_inst_t *i2c; // I2C instance
    uint8_t addr;    // I2C address
    uint8_t sda_pin; // SDA pin number
    uint8_t scl_pin; // SCL pin number

    float shunt_ohms;    // Shunt resistor value in ohms
    float max_current_a; // Maximum current in Amperes

    ina219_battery_config_t battery_config;

    // Calibration values
    float current_lsb;          // Current LSB (Least Significant Bit) value
    float power_lsb;            // Power LSB value
    uint16_t calibration_value; // Calibration value for INA219

    // Status
    bool initialized;
    bool debug;
} ina219_t;

// Function declarations

/**
 * @brief Initialize INA219 device with the default 1S battery configuration
 *
 * @param ina219 Pointer to the INA219 device structure
 * @param i2c Pointer to the I2C instance
 * @param sda_pin SDA pin number
 * @param scl_pin SCL pin number
 * @param addr I2C address
 * @return true Initialization successful
 * @return false Initialization failed
 */
bool ina219_init_default(ina219_t *ina219, i2c_inst_t *i2c, uint8_t sda_pin, uint8_t scl_pin, uint8_t addr);

/**
 * @brief Initialize INA219 device with specific battery type
 *
 * @param ina219 Pointer to INA219 device structure
 * @param i2c I2C instance to use
 * @param sda_pin SDA pin number
 * @param scl_pin SCL pin number
 * @param addr I2C address of INA219
 * @param parallel_count Number of parallel 18650 cells (n in 1SnP configuration)
 * @param shunt_ohms Shunt resistor value in ohms
 * @param max_current_a Maximum expected current in amps
 * @return true if initialization successful
 */
bool ina219_init(ina219_t *ina219, i2c_inst_t *i2c, uint8_t sda_pin, uint8_t scl_pin,
                 uint8_t addr, uint8_t parallel_count, float shunt_ohms, float max_current_a);

/**
 * @brief Initialize INA219 device with custom battery configuration
 *
 * @param ina219 Pointer to INA219 device structure
 * @param i2c I2C instance to use
 * @param sda_pin SDA pin number
 * @param scl_pin SCL pin number
 * @param addr I2C address of INA219
 * @param custom_config Pointer to custom battery configuration
 * @param shunt_ohms Shunt resistor value in ohms
 * @param max_current_a Maximum expected current in amps
 * @return true if initialization successful
 */
bool ina219_init_custom(ina219_t *ina219, i2c_inst_t *i2c, uint8_t sda_pin, uint8_t scl_pin,
                        uint8_t addr, const ina219_battery_config_t *custom_config,
                        float shunt_ohms, float max_current_a);


/**
 * @brief Enable or disable debug output
 * 
 * @param ina219 Pointer to INA219 device structure
 * @param enabled true to enable debug output
 */
void ina219_set_debug(ina219_t* ina219, bool enabled);

/**
 * @brief Read shunt voltage in millivolts
 * 
 * @param ina219 Pointer to INA219 device structure
 * @return Shunt voltage in mV, or 0.0 if error
 */
float ina219_get_shunt_voltage_mv(ina219_t* ina219);

/**
 * @brief Read bus voltage in volts
 * 
 * @param ina219 Pointer to INA219 device structure
 * @return Bus voltage in V, or 0.0 if error
 */
float ina219_get_bus_voltage_v(ina219_t* ina219);

/**
 * @brief Read battery voltage (bus voltage) in volts
 * 
 * @param ina219 Pointer to INA219 device structure
 * @return Battery voltage in V, or 0.0 if error
 */
float ina219_get_battery_voltage(ina219_t* ina219);

/**
 * @brief Read current in milliamps
 * 
 * @param ina219 Pointer to INA219 device structure
 * @return Current in mA, negative for discharge, positive for charge
 */
float ina219_get_current_ma(ina219_t* ina219);

/**
 * @brief Read power in milliwatts
 * 
 * @param ina219 Pointer to INA219 device structure
 * @return Power in mW, or 0.0 if error
 */
float ina219_get_power_mw(ina219_t* ina219);

/**
 * @brief Calculate battery percentage based on voltage
 * 
 * @param ina219 Pointer to INA219 device structure
 * @param voltage Battery voltage in V (optional, if 0 will read current voltage)
 * @return Battery percentage (0-100), or 0 if error
 */
float ina219_get_battery_percentage(ina219_t* ina219, float voltage);

/**
 * @brief Get battery status as enum
 * 
 * @param ina219 Pointer to INA219 device structure
 * @param voltage Battery voltage in V (optional, if 0 will read current voltage)
 * @return Battery status enum
 */
ina219_battery_status_t ina219_get_battery_status(ina219_t* ina219, float voltage);

/**
 * @brief Get battery status as string
 * 
 * @param ina219 Pointer to INA219 device structure
 * @param voltage Battery voltage in V (optional, if 0 will read current voltage)
 * @return Pointer to status string (static, do not free)
 */
const char* ina219_get_battery_status_str(ina219_t* ina219, float voltage);

/**
 * @brief Check if battery voltage is within healthy range
 * 
 * @param ina219 Pointer to INA219 device structure
 * @param voltage Battery voltage in V (optional, if 0 will read current voltage)
 * @return true if battery is healthy
 */
bool ina219_is_battery_healthy(ina219_t* ina219, float voltage);

/**
 * @brief Estimate runtime in hours based on current consumption
 * 
 * @param ina219 Pointer to INA219 device structure
 * @param battery_capacity_mah Battery capacity in mAh
 * @return Estimated runtime in hours, or infinity if no consumption
 */
float ina219_get_runtime_hours(ina219_t* ina219, float battery_capacity_mah);

/**
 * @brief Check if INA219 device is present on I2C bus
 * 
 * @param ina219 Pointer to INA219 device structure
 * @return true if device responds to I2C communication
 */
bool ina219_is_present(ina219_t* ina219);

/**
 * @brief Reset INA219 device to default configuration
 * 
 * @param ina219 Pointer to INA219 device structure
 * @return true if reset successful
 */
bool ina219_reset(ina219_t* ina219);

// Convenience factory functions

/**
 * @brief Create and initialize INA219 for 1SnP Li-ion system
 * 
 * @param ina219 Pointer to INA219 device structure
 * @param i2c I2C instance to use
 * @param sda_pin SDA pin number
 * @param scl_pin SCL pin number
 * @param addr I2C address
 * @param parallel_count Number of parallel 18650 cells (1, 4, 16, etc.)
 * @param shunt_ohms Shunt resistor value in ohms
 * @param max_current_a Maximum expected current in amps
 * @return true if successful
 */
bool ina219_create_1s_monitor(ina219_t* ina219, i2c_inst_t* i2c, uint8_t sda_pin, uint8_t scl_pin,
                              uint8_t addr, uint8_t parallel_count, float shunt_ohms, float max_current_a);

#endif // INA219_I2C_H