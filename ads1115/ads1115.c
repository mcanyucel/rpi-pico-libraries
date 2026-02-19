/**
 * @file ads1115.c
 * @brief ADS1115 16-bit ADC driver implementation
 * @version 2.0
 */

#include "ads1115.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include <stdio.h>

// ============================================================================
// INTERNAL: LOW-LEVEL I2C
// ============================================================================

static bool write_reg(ads1115_t *dev, uint8_t reg, uint16_t value) {
    uint8_t data[3] = {
        reg,
        (uint8_t)(value >> 8),
        (uint8_t)(value & 0xFF)
    };
    return i2c_write_blocking(dev->config.i2c, dev->config.address, data, 3, false) == 3;
}

static bool read_reg(ads1115_t *dev, uint8_t reg, uint16_t *value) {
    if (i2c_write_blocking(dev->config.i2c, dev->config.address, &reg, 1, true) != 1) {
        return false;
    }
    uint8_t data[2];
    if (i2c_read_blocking(dev->config.i2c, dev->config.address, data, 2, false) != 2) {
        return false;
    }
    *value = ((uint16_t)data[0] << 8) | data[1];
    return true;
}

// ============================================================================
// INTERNAL: MUX BITS FROM CHANNEL
// ============================================================================

static uint16_t channel_to_mux(ads1115_channel_t channel) {
    switch (channel) {
        case ADS1115_CHANNEL_0: return ADS1115_MUX_SINGLE_0;
        case ADS1115_CHANNEL_1: return ADS1115_MUX_SINGLE_1;
        case ADS1115_CHANNEL_2: return ADS1115_MUX_SINGLE_2;
        case ADS1115_CHANNEL_3: return ADS1115_MUX_SINGLE_3;
        default:                return ADS1115_MUX_SINGLE_0;
    }
}

// ============================================================================
// INITIALIZATION
// ============================================================================

bool ads1115_init(ads1115_t *dev, const ads1115_config_t *config) {
    dev->config = *config;
    dev->initialized = false;
    dev->current_channel = ADS1115_CHANNEL_0;

    gpio_set_function(config->sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(config->scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(config->sda_pin);
    gpio_pull_up(config->scl_pin);

    sleep_ms(10);

    if (!ads1115_is_present(dev)) {
        return false;
    }

    // Write initial config: single-shot, channel 0, configured gain and rate,
    // comparator disabled
    uint16_t gain_bits[] = {
        ADS1115_PGA_6_144V, ADS1115_PGA_4_096V, ADS1115_PGA_2_048V,
        ADS1115_PGA_1_024V, ADS1115_PGA_0_512V, ADS1115_PGA_0_256V
    };
    uint16_t rate_bits[] = {
        ADS1115_DR_8_SPS,  ADS1115_DR_16_SPS,  ADS1115_DR_32_SPS,
        ADS1115_DR_64_SPS, ADS1115_DR_128_SPS, ADS1115_DR_250_SPS,
        ADS1115_DR_475_SPS, ADS1115_DR_860_SPS
    };

    uint16_t cfg = ADS1115_MUX_SINGLE_0
                 | gain_bits[config->gain]
                 | ADS1115_MODE_SINGLE
                 | rate_bits[config->rate]
                 | ADS1115_COMP_QUE_DISABLE;

    if (!write_reg(dev, ADS1115_REG_CONFIG, cfg)) {
        return false;
    }

    dev->initialized = true;
    return true;
}

bool ads1115_is_present(ads1115_t *dev) {
    uint16_t val;
    return read_reg(dev, ADS1115_REG_CONFIG, &val);
}

// ============================================================================
// CONFIGURATION
// ============================================================================

bool ads1115_set_gain(ads1115_t *dev, ads1115_gain_t gain) {
    uint16_t cfg;
    if (!read_reg(dev, ADS1115_REG_CONFIG, &cfg)) return false;

    static const uint16_t gain_bits[] = {
        ADS1115_PGA_6_144V, ADS1115_PGA_4_096V, ADS1115_PGA_2_048V,
        ADS1115_PGA_1_024V, ADS1115_PGA_0_512V, ADS1115_PGA_0_256V
    };

    cfg = (cfg & ~0x0E00) | gain_bits[gain];

    if (!write_reg(dev, ADS1115_REG_CONFIG, cfg)) return false;

    dev->config.gain = gain;
    return true;
}

bool ads1115_set_data_rate(ads1115_t *dev, ads1115_data_rate_t rate) {
    uint16_t cfg;
    if (!read_reg(dev, ADS1115_REG_CONFIG, &cfg)) return false;

    static const uint16_t rate_bits[] = {
        ADS1115_DR_8_SPS,   ADS1115_DR_16_SPS,  ADS1115_DR_32_SPS,
        ADS1115_DR_64_SPS,  ADS1115_DR_128_SPS, ADS1115_DR_250_SPS,
        ADS1115_DR_475_SPS, ADS1115_DR_860_SPS
    };

    cfg = (cfg & ~0x00E0) | rate_bits[rate];

    if (!write_reg(dev, ADS1115_REG_CONFIG, cfg)) return false;

    dev->config.rate = rate;
    return true;
}

// ============================================================================
// MEASUREMENT
// ============================================================================

bool ads1115_start_conversion(ads1115_t *dev, ads1115_channel_t channel) {
    uint16_t cfg;
    if (!read_reg(dev, ADS1115_REG_CONFIG, &cfg)) return false;

    cfg = (cfg & ~0x7000) | channel_to_mux(channel);   // set channel
    cfg |= ADS1115_OS_SINGLE;                           // trigger conversion

    if (!write_reg(dev, ADS1115_REG_CONFIG, cfg)) return false;

    dev->current_channel = channel;
    return true;
}

bool ads1115_is_conversion_ready(ads1115_t *dev) {
    uint16_t cfg;
    if (!read_reg(dev, ADS1115_REG_CONFIG, &cfg)) return false;
    return (cfg & ADS1115_OS_NOT_BUSY) != 0;
}

bool ads1115_read_conversion(ads1115_t *dev, int16_t *result) {
    uint16_t raw;
    if (!read_reg(dev, ADS1115_REG_CONVERSION, &raw)) return false;
    *result = (int16_t)raw;
    return true;
}

/**
 * @brief Blocking convenience function: start conversion, wait, read voltage.
 * @param voltage  Output voltage in volts
 * @return false on I2C error or timeout
 */
bool ads1115_read_voltage(ads1115_t *dev, ads1115_channel_t channel, float *voltage) {
    if (!ads1115_start_conversion(dev, channel)) return false;

    uint32_t timeout_ms = ads1115_get_conversion_time_ms(dev->config.rate) + 10;
    uint32_t start = to_ms_since_boot(get_absolute_time());

    while (!ads1115_is_conversion_ready(dev)) {
        if (to_ms_since_boot(get_absolute_time()) - start > timeout_ms) {
            return false;
        }
        sleep_ms(1);
    }

    int16_t raw;
    if (!ads1115_read_conversion(dev, &raw)) return false;

    *voltage = ads1115_raw_to_voltage(raw, dev->config.gain);
    return true;
}

// ============================================================================
// UTILITIES
// ============================================================================

float ads1115_get_voltage_range(ads1115_gain_t gain) {
    static const float ranges[] = {6.144f, 4.096f, 2.048f, 1.024f, 0.512f, 0.256f};
    return (gain <= ADS1115_GAIN_0_256V) ? ranges[gain] : 0.0f;
}

uint32_t ads1115_get_conversion_time_ms(ads1115_data_rate_t rate) {
    static const uint32_t times[] = {125, 63, 32, 16, 8, 5, 3, 2};
    return (rate <= ADS1115_RATE_860_SPS) ? times[rate] : 10;
}