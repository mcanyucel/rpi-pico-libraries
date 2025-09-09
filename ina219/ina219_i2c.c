/**
 * @file ina219_i2c.c
 * @author Mustafa Can Yucel via Claude Sonnet 4 (mustafacan@bridgewiz.com)
 * @brief INA219 I2C Power Monitor Driver Implementation
 * @version 0.1
 * @date 2025-08-20
 * 
 * @copyright Copyright (c) 2025
 */

 #include "ina219_i2c.h"
 #include <string.h>
 #include <math.h>
 #include <stdio.h>
 #include "pico/stdlib.h"
 #include "hardware/i2c.h"

 // Predefine common battery configurations
 static const ina219_battery_config_t battery_configs[] = {
    [INA219_BATTERY_1S] = {
        .cell_count = 1,
        .parallel_count = 1,
        .nominal_voltage = 3.7,
        .max_voltage = 4.2,
        .min_voltage = 3.0,
        .bus_voltage_range = 16,
        .capacity_mah = 2000,
        .config_name = "1S"
    }
 };

 // Internal helper functions
static bool ina219_write_register(ina219_t* ina219, uint8_t reg, uint16_t value);
static bool ina219_read_register(ina219_t* ina219, uint8_t reg, uint16_t* value);
static void ina219_log(ina219_t* ina219, const char* message);
static bool ina219_configure_for_battery_system(ina219_t* ina219);
static float ina219_calculate_single_cell_percentage(float voltage);

bool ina219_init_default(ina219_t* ina219, i2c_inst_t* i2c, uint8_t sda_pin, uint8_t scl_pin, uint8_t addr) {
    return ina219_init(ina219, i2c, sda_pin, scl_pin, addr, 1, 0.1f, 3.0f);
}

bool ina219_init(ina219_t* ina219, i2c_inst_t* i2c, uint8_t sda_pin, uint8_t scl_pin, 
                 uint8_t addr, uint8_t parallel_count, float shunt_ohms, float max_current_a) {
    
    if (!ina219 || parallel_count == 0) {
        return false;
    }

    // Initialize structure
    memset(ina219, 0, sizeof(ina219_t));
    
    ina219->i2c = i2c;
    ina219->addr = addr;
    ina219->sda_pin = sda_pin;
    ina219->scl_pin = scl_pin;
    ina219->shunt_ohms = shunt_ohms;
    ina219->max_current_a = max_current_a;
    ina219->debug = false;
    
    // Copy and customize battery configuration for 1SnP
    memcpy(&ina219->battery_config, &battery_configs[INA219_BATTERY_1S], sizeof(ina219_battery_config_t));
    ina219->battery_config.parallel_count = parallel_count;
    
    // Update config name to reflect parallel count
    static char config_name_buffer[16];
    snprintf(config_name_buffer, sizeof(config_name_buffer), "1S%dP", parallel_count);
    ina219->battery_config.config_name = config_name_buffer;
    
    // Initialize I2C
    i2c_init(ina219->i2c, INA219_I2C_CLK);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);
    
    sleep_ms(100);  // Allow I2C to stabilize
    
    // Check if device is present
    if (!ina219_is_present(ina219)) {
        printf("ERROR: INA219 not found at address 0x%02X\n", addr);
        return false;
    }
    
    // Configure INA219 for the battery system
    if (!ina219_configure_for_battery_system(ina219)) {
        printf("ERROR: Failed to configure INA219\n");
        return false;
    }
    
    ina219->initialized = true;
    
    printf("INA219 initialized for %s battery system\n", ina219->battery_config.config_name);
    printf("  Voltage range: %.1fV - %.1fV\n", 
           ina219->battery_config.min_voltage, ina219->battery_config.max_voltage);
    printf("  Shunt: %.3fΩ, Max current: %.1fA\n", 
           ina219->shunt_ohms, ina219->max_current_a);
    
    return true;
}

bool ina219_init_custom(ina219_t* ina219, i2c_inst_t* i2c, uint8_t sda_pin, uint8_t scl_pin,
                        uint8_t addr, const ina219_battery_config_t* custom_config,
                        float shunt_ohms, float max_current_a) {
    
    if (!ina219 || !custom_config) {
        return false;
    }

    // Initialize structure
    memset(ina219, 0, sizeof(ina219_t));
    
    ina219->i2c = i2c;
    ina219->addr = addr;
    ina219->sda_pin = sda_pin;
    ina219->scl_pin = scl_pin;
    ina219->shunt_ohms = shunt_ohms;
    ina219->max_current_a = max_current_a;
    ina219->debug = false;
    
    // Copy custom battery configuration
    memcpy(&ina219->battery_config, custom_config, sizeof(ina219_battery_config_t));
    
    // Initialize I2C
    i2c_init(ina219->i2c, INA219_I2C_CLK);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);
    
    sleep_ms(100);
    
    // Check if device is present
    if (!ina219_is_present(ina219)) {
        printf("ERROR: INA219 not found at address 0x%02X\n", addr);
        return false;
    }
    
    // Configure INA219 for the battery system
    if (!ina219_configure_for_battery_system(ina219)) {
        printf("ERROR: Failed to configure INA219\n");
        return false;
    }
    
    ina219->initialized = true;
    
    printf("INA219 initialized for custom battery system\n");
    printf("  Voltage range: %.1fV - %.1fV\n", 
           ina219->battery_config.min_voltage, ina219->battery_config.max_voltage);
    printf("  Shunt: %.3fΩ, Max current: %.1fA\n", 
           ina219->shunt_ohms, ina219->max_current_a);
    
    return true;
}

void ina219_set_debug(ina219_t* ina219, bool enabled) {
    if (!ina219) {
        return;
    }
    ina219->debug = enabled;
}

float ina219_get_shunt_voltage_mv(ina219_t* ina219) {
    if (!ina219 || !ina219->initialized) {
        return 0.0f;
    }

    uint16_t raw_value;
    if (!ina219_read_register(ina219, INA219_REG_SHUNT_VOLTAGE, &raw_value)) {
        return 0.0f;
    }

    int16_t signed_value = (int16_t)raw_value;
    float voltage_mv = signed_value * 0.01f; // Convert to mV
    ina219_log(ina219, "Shunt Voltage measured");
    return voltage_mv;
}

float ina219_get_bus_voltage_v(ina219_t* ina219) {
    if (!ina219 || !ina219->initialized) {
        return 0.0f;
    }
    
    uint16_t raw_value;
    if (!ina219_read_register(ina219, INA219_REG_BUS_VOLTAGE, &raw_value)) {
        return 0.0f;
    }
    
    // Extract voltage (bits 15-3), bit 2 is CNVR, bit 1 is OVF, bit 0 is reserved
    uint16_t voltage_raw = (raw_value >> 3) & 0x1FFF;
    
    // Bus voltage LSB = 4mV
    float voltage_v = voltage_raw * 0.004f;
    
    ina219_log(ina219, "Read bus voltage");
    
    return voltage_v;
}

float ina219_get_battery_voltage(ina219_t* ina219) {
    return ina219_get_bus_voltage_v(ina219);
}

float ina219_get_current_ma(ina219_t* ina219) {
    if (!ina219 || !ina219->initialized) {
        return 0.0f;
    }
    
    uint16_t raw_value;
    if (!ina219_read_register(ina219, INA219_REG_CURRENT, &raw_value)) {
        return 0.0f;
    }
    
    // Convert to signed
    int16_t signed_value = (int16_t)raw_value;
    
    // Current = raw_value * current_lsb
    float current_ma = signed_value * ina219->current_lsb * 1000.0f;  // Convert to mA
    
    ina219_log(ina219, "Read current");
    
    return current_ma;
}

float ina219_get_power_mw(ina219_t* ina219) {
    if (!ina219 || !ina219->initialized) {
        return 0.0f;
    }
    
    uint16_t raw_value;
    if (!ina219_read_register(ina219, INA219_REG_POWER, &raw_value)) {
        return 0.0f;
    }
    
    // Power = raw_value * power_lsb
    float power_mw = raw_value * ina219->power_lsb * 1000.0f;  // Convert to mW
    
    ina219_log(ina219, "Read power");
    
    return power_mw;
}

float ina219_get_battery_percentage(ina219_t* ina219, float voltage) {
    if (!ina219 || !ina219->initialized) {
        return 0.0f;
    }
    
    if (voltage == 0.0f) {
        voltage = ina219_get_battery_voltage(ina219);
    }
    
    if (voltage == 0.0f) {
        return 0.0f;
    }
    
    // Always use single cell percentage calculation for 1SnP configurations
    // TODO: Implement multi-cell configurations
    return ina219_calculate_single_cell_percentage(voltage);
}

ina219_battery_status_t ina219_get_battery_status(ina219_t* ina219, float voltage) {
    if (!ina219 || !ina219->initialized) {
        return INA219_BATTERY_CRITICAL;
    }
    
    if (voltage == 0.0f) {
        voltage = ina219_get_battery_voltage(ina219);
    }
    
    float percentage = ina219_get_battery_percentage(ina219, voltage);
    
    if (voltage < ina219->battery_config.min_voltage) {
        return INA219_BATTERY_CRITICAL;
    } else if (percentage < 10.0f) {
        return INA219_BATTERY_LOW;
    } else if (percentage < 25.0f) {
        return INA219_BATTERY_FAIR;
    } else if (percentage < 75.0f) {
        return INA219_BATTERY_GOOD;
    } else {
        return INA219_BATTERY_EXCELLENT;
    }
}

const char* ina219_get_battery_status_str(ina219_t* ina219, float voltage) {
    static const char* status_strings[] = {
        [INA219_BATTERY_CRITICAL] = "CRITICAL",
        [INA219_BATTERY_LOW] = "LOW",
        [INA219_BATTERY_FAIR] = "FAIR",
        [INA219_BATTERY_GOOD] = "GOOD",
        [INA219_BATTERY_EXCELLENT] = "EXCELLENT"
    };
    
    ina219_battery_status_t status = ina219_get_battery_status(ina219, voltage);
    return status_strings[status];
}

bool ina219_is_battery_healthy(ina219_t* ina219, float voltage) {
    if (!ina219 || !ina219->initialized) {
        return false;
    }
    
    if (voltage == 0.0f) {
        voltage = ina219_get_battery_voltage(ina219);
    }
    
    return (voltage >= ina219->battery_config.min_voltage && 
            voltage <= ina219->battery_config.max_voltage);
}

float ina219_get_runtime_hours(ina219_t* ina219, float battery_capacity_mah) {
    if (!ina219 || !ina219->initialized) {
        return 0.0f;
    }
    
    float current_ma = ina219_get_current_ma(ina219);
    float percentage = ina219_get_battery_percentage(ina219, 0.0f);
    
    if (current_ma <= 0.0f) {
        return INFINITY;  // No current draw
    }
    
    float remaining_capacity_mah = (battery_capacity_mah * percentage) / 100.0f;
    float runtime_hours = remaining_capacity_mah / current_ma;
    
    return runtime_hours;
}

bool ina219_is_present(ina219_t* ina219) {
    if (!ina219) {
        return false;
    }
    
    uint8_t test_byte;
    int result = i2c_read_blocking(ina219->i2c, ina219->addr, &test_byte, 1, false);
    
    return (result >= 0);
}

bool ina219_reset(ina219_t* ina219) {
    if (!ina219) {
        return false;
    }
    
    // Send reset command
    bool result = ina219_write_register(ina219, INA219_REG_CONFIG, INA219_CONFIG_RESET);
    
    if (result) {
        sleep_ms(100);  // Wait for reset to complete
        ina219->initialized = false;
    }
    
    return result;
}

bool ina219_create_1s_monitor(ina219_t* ina219, i2c_inst_t* i2c, uint8_t sda_pin, uint8_t scl_pin,
                              uint8_t addr, uint8_t parallel_count, float shunt_ohms, float max_current_a) {
    return ina219_init(ina219, i2c, sda_pin, scl_pin, addr, parallel_count, shunt_ohms, max_current_a);
}

// Internal helper functions

static bool ina219_write_register(ina219_t* ina219, uint8_t reg, uint16_t value) {
    uint8_t data[3] = {reg, (value >> 8) & 0xFF, value & 0xFF};
    
    int result = i2c_write_blocking(ina219->i2c, ina219->addr, data, 3, false);
    return (result == 3);
}

static bool ina219_read_register(ina219_t* ina219, uint8_t reg, uint16_t* value) {
    // Write register address
    int result = i2c_write_blocking(ina219->i2c, ina219->addr, &reg, 1, true);
    if (result != 1) {
        return false;
    }
    
    // Read 2 bytes
    uint8_t data[2];
    result = i2c_read_blocking(ina219->i2c, ina219->addr, data, 2, false);
    if (result != 2) {
        return false;
    }
    
    *value = (data[0] << 8) | data[1];
    return true;
}

static void ina219_log(ina219_t* ina219, const char* message) {
    if (ina219 && ina219->debug) {
        printf("[INA219-%s] %s\n", ina219->battery_config.config_name, message);
    }
}

static bool ina219_configure_for_battery_system(ina219_t* ina219) {
    // Reset device first
    if (!ina219_write_register(ina219, INA219_REG_CONFIG, INA219_CONFIG_RESET)) {
        return false;
    }
    sleep_ms(100);
    
    // Determine bus voltage range
    uint16_t bus_voltage_bits = (ina219->battery_config.bus_voltage_range == 32) ? 
                                INA219_CONFIG_BUS_32V : INA219_CONFIG_BUS_16V;
    
    // Configuration:
    // - Configurable bus voltage range (16V or 32V)
    // - ±320mV shunt voltage range (suitable for 0.1Ω shunt)
    // - 12-bit ADC resolution
    // - Continuous measurement mode
    uint16_t config = bus_voltage_bits | 
                      INA219_CONFIG_SHUNT_320MV |
                      INA219_CONFIG_ADC_12BIT |
                      INA219_CONFIG_ADC_12BIT_S |
                      INA219_CONFIG_MODE_CONT;
    
    if (!ina219_write_register(ina219, INA219_REG_CONFIG, config)) {
        return false;
    }
    sleep_ms(100);
    
    // Calculate calibration based on actual parameters
    // Current_LSB = Max_Expected_Current / 32768
    ina219->current_lsb = ina219->max_current_a / 32768.0f;
    
    // Cal = 0.04096 / (Current_LSB * Rshunt)
    float calibration_float = 0.04096f / (ina219->current_lsb * ina219->shunt_ohms);

    // Ensure calibration is within valid range
    ina219->calibration_value = (uint16_t)fmax(1.0f, fmin(65535.0f, calibration_float));

    if (!ina219_write_register(ina219, INA219_REG_CALIBRATION, ina219->calibration_value)) {
        return false;
    }
    
    // Power LSB is 20x current LSB
    ina219->power_lsb = ina219->current_lsb * 20.0f;
    
    ina219_log(ina219, "Configuration complete");
    
    if (ina219->debug) {
        printf("Current LSB: %.3fmA, Cal: %d\n", 
               ina219->current_lsb * 1000.0f, ina219->calibration_value);
    }
    
    return true;
}

static float ina219_calculate_single_cell_percentage(float voltage) {
    // Single cell Li-ion discharge curve approximation
    if (voltage >= 4.1f) return 100.0f;
    if (voltage >= 3.9f) return 90.0f + (voltage - 3.9f) * 50.0f;  // 90-100%
    if (voltage >= 3.8f) return 70.0f + (voltage - 3.8f) * 200.0f; // 70-90%
    if (voltage >= 3.7f) return 40.0f + (voltage - 3.7f) * 300.0f; // 40-70%
    if (voltage >= 3.6f) return 20.0f + (voltage - 3.6f) * 200.0f; // 20-40%
    if (voltage >= 3.4f) return 5.0f + (voltage - 3.4f) * 75.0f;   // 5-20%
    if (voltage >= 3.0f) return (voltage - 3.0f) * 12.5f;          // 0-5%
    return 0.0f;
}