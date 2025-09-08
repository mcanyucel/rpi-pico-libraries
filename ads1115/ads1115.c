/**
 * @file ads1115.c
 * @author
 * @brief ADS1115 16-bit ADC I2C Driver Implementation
 * @version 1.0
 * @date 2025-08-26
 *
 * Implementation of ADS1115 16-bit Analog-to-Digital Converter driver
 * for high-precision LVDT measurements.
 */

#include "ads1115.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include <stdio.h>

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

static ads1115_gain_t current_gain = ADS1115_GAIN_4_096V;       ///< Current gain setting
static ads1115_data_rate_t current_rate = ADS1115_RATE_128_SPS; ///< Current data rate
static ads1115_channel_t current_channel = ADS1115_CHANNEL_0;   ///< Current channel

// ============================================================================
// LOW-LEVEL I2C FUNCTIONS
// ============================================================================

/**
 * @brief Write a 16-bit value to an ADS1115 register
 * @param reg Register address (0x00-0x03)
 * @param value 16-bit value to write (big-endian)
 * @return true if successful, false otherwise
 */
bool ads1115_write_reg(uint8_t reg, uint16_t value) {
    // ADS1115 expects big-endian (MSB first)
    uint8_t data[3] = {
        reg,
        (uint8_t)(value >> 8),    // MSB
        (uint8_t)(value & 0xFF)   // LSB
    };
    
    int result = i2c_write_blocking(ADS1115_I2C_PORT, ADS1115_I2C_ADDR, data, 3, false);
    return result == 3;
}


/**
 * @brief Read a 16-bit value from an ADS1115 register
 * @param reg Register address (0x00-0x03)
 * @param value Pointer to store the 16-bit result
 * @return true if successful, false otherwise
 */
bool ads1115_read_reg(uint8_t reg, uint16_t *value) {
    // Write register address
    if (i2c_write_blocking(ADS1115_I2C_PORT, ADS1115_I2C_ADDR, &reg, 1, true) != 1) {
        return false;
    }
    
    // Read 16-bit value (big-endian)
    uint8_t data[2];
    if (i2c_read_blocking(ADS1115_I2C_PORT, ADS1115_I2C_ADDR, data, 2, false) != 2) {
        return false;
    }
    
    // Combine bytes (ADS1115 sends MSB first)
    *value = ((uint16_t)data[0] << 8) | data[1];
    return true;
}

// ============================================================================
// INITIALIZATION AND CONFIGURATION
// ============================================================================

/**
 * @brief Initialize the ADS1115 ADC
 * 
 * @return true if successful, false otherwise
 */
bool ads1115_init(void) {
    printf("Initializing ADS1115...\n");
    printf("ADS1115 using I2C0 on GP%d/GP%d (shared with OLED)\n", 
           ADS1115_SDA_PIN, ADS1115_SCL_PIN);
    
    sleep_ms(50); // Small delay for power stabilization

    // Test communication by reading config register
    uint16_t config;
    if (!ads1115_read_reg(ADS1115_REG_CONFIG, &config)) {
        printf("ERROR: ADS1115 not responding on I2C address 0x%02X\n", ADS1115_I2C_ADDR);
        return false;   
    }

    printf("ADS1115 communication OK (config reg: 0x%04X)\n", config);

    // Configure for LVDT measurement
    // Single-shot mode, A0 channel, ±4.096V range, 128 SPS
    uint16_t init_config = ADS1115_CONFIG_DEFAULT;
    if (!ads1115_write_reg(ADS1115_REG_CONFIG, init_config)) {
        printf("ERROR: Failed to write initial ADS1115 configuration\n");
        return false;
    }

    // Set initial state variables
    current_gain = ADS1115_GAIN_4_096V;
    current_rate = ADS1115_RATE_128_SPS;
    current_channel = ADS1115_CHANNEL_0;
    
    printf("ADS1115 initialized successfully\n");
    printf("- Channel: A0 (LVDT probe)\n");
    printf("- Gain: ±4.096V (125 µV/bit resolution)\n");
    printf("- Data Rate: 128 SPS\n");
    printf("- Mode: Single-shot\n");
    
    return true;
}

/**
 * @brief Check if ADS1115 is present and responding
 * @return true if device responds, false otherwise
 */
bool ads1115_is_present(void) {
    uint16_t test_val;
    return ads1115_read_reg(ADS1115_REG_CONFIG, &test_val);
}

/**
 * @brief Set the programmable gain amplifier (PGA) setting
 * @param gain Gain setting from ads1115_gain_t enum
 * @return true if successful, false otherwise
 */
bool ads1115_set_gain(ads1115_gain_t gain) {
    uint16_t config;
    if (!ads1115_read_reg(ADS1115_REG_CONFIG, &config)) {
        return false;
    }
    
    // Clear existing gain bits and set new ones
    config &= ~0x0E00;  // Clear bits 11:9
    
    switch (gain) {
        case ADS1115_GAIN_6_144V: config |= ADS1115_PGA_6_144V; break;
        case ADS1115_GAIN_4_096V: config |= ADS1115_PGA_4_096V; break;
        case ADS1115_GAIN_2_048V: config |= ADS1115_PGA_2_048V; break;
        case ADS1115_GAIN_1_024V: config |= ADS1115_PGA_1_024V; break;
        case ADS1115_GAIN_0_512V: config |= ADS1115_PGA_0_512V; break;
        case ADS1115_GAIN_0_256V: config |= ADS1115_PGA_0_256V; break;
        default: return false;
    }
    
    if (ads1115_write_reg(ADS1115_REG_CONFIG, config)) {
        current_gain = gain;
        printf("ADS1115 gain set to ±%.3fV\n", ads1115_get_voltage_range(gain));
        return true;
    }
    
    return false;
}

/**
 * @brief Set the data rate (samples per second)
 * @param rate Data rate from ads1115_data_rate_t enum
 * @return true if successful, false otherwise
 */
bool ads1115_set_data_rate(ads1115_data_rate_t rate) {
    uint16_t config;
    if (!ads1115_read_reg(ADS1115_REG_CONFIG, &config)) {
        return false;
    }
    
    // Clear existing data rate bits and set new ones
    config &= ~0x00E0;  // Clear bits 7:5
    
    switch (rate) {
        case ADS1115_RATE_8_SPS:   config |= ADS1115_DR_8_SPS; break;
        case ADS1115_RATE_16_SPS:  config |= ADS1115_DR_16_SPS; break;
        case ADS1115_RATE_32_SPS:  config |= ADS1115_DR_32_SPS; break;
        case ADS1115_RATE_64_SPS:  config |= ADS1115_DR_64_SPS; break;
        case ADS1115_RATE_128_SPS: config |= ADS1115_DR_128_SPS; break;
        case ADS1115_RATE_250_SPS: config |= ADS1115_DR_250_SPS; break;
        case ADS1115_RATE_475_SPS: config |= ADS1115_DR_475_SPS; break;
        case ADS1115_RATE_860_SPS: config |= ADS1115_DR_860_SPS; break;
        default: return false;
    }
    
    if (ads1115_write_reg(ADS1115_REG_CONFIG, config)) {
        current_rate = rate;
        static const uint16_t sps_values[] = {8, 16, 32, 64, 128, 250, 475, 860};
        printf("ADS1115 data rate set to %d SPS\n", sps_values[rate]);
        return true;
    }
    
    return false;
}

/**
 * @brief Set the input channel for single-ended measurements
 * @param channel Channel from ads1115_channel_t enum
 * @return true if successful, false otherwise
 */
bool ads1115_set_channel(ads1115_channel_t channel) {
    uint16_t config;
    if (!ads1115_read_reg(ADS1115_REG_CONFIG, &config)) {
        return false;
    }
    
    // Clear existing MUX bits and set new ones
    config &= ~0x7000;  // Clear bits 14:12
    
    switch (channel) {
        case ADS1115_CHANNEL_0: config |= ADS1115_MUX_SINGLE_0; break;
        case ADS1115_CHANNEL_1: config |= ADS1115_MUX_SINGLE_1; break;
        case ADS1115_CHANNEL_2: config |= ADS1115_MUX_SINGLE_2; break;
        case ADS1115_CHANNEL_3: config |= ADS1115_MUX_SINGLE_3; break;
        default: return false;
    }
    
    if (ads1115_write_reg(ADS1115_REG_CONFIG, config)) {
        current_channel = channel;
        printf("ADS1115 channel set to A%d\n", channel);
        return true;
    }
    
    return false;
}

// ============================================================================
// MEASUREMENT FUNCTIONS
// ============================================================================

/**
 * @brief Start a single-shot conversion on specified channel
 * @param channel Channel to measure
 * @return true if conversion started successfully, false otherwise
 */
bool ads1115_start_conversion(ads1115_channel_t channel) {
    // Set channel if different from current
    if (channel != current_channel) {
        if (!ads1115_set_channel(channel)) {
            return false;
        }
    }
    
    // Read current config
    uint16_t config;
    if (!ads1115_read_reg(ADS1115_REG_CONFIG, &config)) {
        return false;
    }
    
    // Set OS bit to start conversion
    config |= ADS1115_OS_SINGLE;
    
    return ads1115_write_reg(ADS1115_REG_CONFIG, config);
}

/**
 * @brief Check if conversion is complete
 * @return true if conversion is ready, false if still converting
 */
bool ads1115_is_conversion_ready(void) {
    uint16_t config;
    if (!ads1115_read_reg(ADS1115_REG_CONFIG, &config)) {
        return false; // Error reading, assume not ready
    }
    
    // Check OS bit - it's set when conversion is complete
    return (config & ADS1115_OS_NOT_BUSY) != 0;
}

/**
 * @brief Read the conversion result register
 * @param result Pointer to store 16-bit signed result
 * @return true if successful, false otherwise
 */
bool ads1115_read_conversion(int16_t *result) {
    uint16_t raw_result;
    if (ads1115_read_reg(ADS1115_REG_CONVERSION, &raw_result)) {
        *result = (int16_t)raw_result;  // Cast to signed
        return true;
    }
    return false;
}

/**
 * @brief Perform complete measurement cycle and return voltage
 * @param channel Channel to measure (A0-A3)
 * @return Measured voltage in volts, or -999.0 on error
 */
float ads1115_read_voltage(ads1115_channel_t channel) {
    // Start conversion
    if (!ads1115_start_conversion(channel)) {
        printf("ERROR: Failed to start ADS1115 conversion\n");
        return -999.0f;
    }
    
    // Wait for conversion to complete
    uint32_t timeout = ads1115_get_conversion_time_ms(current_rate) + 50; // Add 50ms margin
    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    
    while (!ads1115_is_conversion_ready()) {
        if (to_ms_since_boot(get_absolute_time()) - start_time > timeout) {
            printf("ERROR: ADS1115 conversion timeout\n");
            return -999.0f;
        }
        sleep_ms(1); // Small delay to prevent excessive I2C traffic
    }
    
    // Read result
    int16_t raw_result;
    if (!ads1115_read_conversion(&raw_result)) {
        printf("ERROR: Failed to read ADS1115 conversion result\n");
        return -999.0f;
    }
    
    // Convert to voltage
    return ads1115_raw_to_voltage(raw_result, current_gain);
}

// ============================================================================
// HIGH-LEVEL UTILITY FUNCTIONS
// ============================================================================

/**
 * @brief Get the voltage range for a given gain setting
 * @param gain Gain setting
 * @return Full-scale voltage range (positive value)
 */
float ads1115_get_voltage_range(ads1115_gain_t gain) {
    static const float voltage_ranges[] = {
        6.144f,     // ±6.144V
        4.096f,     // ±4.096V  
        2.048f,     // ±2.048V
        1.024f,     // ±1.024V
        0.512f,     // ±0.512V
        0.256f      // ±0.256V
    };
    
    if (gain <= ADS1115_GAIN_0_256V) {
        return voltage_ranges[gain];
    }
    return 0.0f;
}

/**
 * @brief Get the expected conversion time for a data rate
 * @param rate Data rate setting
 * @return Conversion time in milliseconds
 */
uint32_t ads1115_get_conversion_time_ms(ads1115_data_rate_t rate) {
    // Conversion time = 1000ms / samples_per_second + small margin
    static const uint32_t conversion_times[] = {
        125,    // 8 SPS:   125ms
        63,     // 16 SPS:  62.5ms  
        32,     // 32 SPS:  31.25ms
        16,     // 64 SPS:  15.625ms
        8,      // 128 SPS: 7.8ms
        5,      // 250 SPS: 4ms
        3,      // 475 SPS: 2.1ms
        2       // 860 SPS: 1.16ms
    };
    
    if (rate <= ADS1115_RATE_860_SPS) {
        return conversion_times[rate];
    }
    return 10; // Default fallback
}