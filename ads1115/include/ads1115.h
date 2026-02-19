/**
 * @file ads1115.h
 * @author 
 * @brief ADS1116 16-bit ADC driver
 * @version 2.0
 * @date 2026-02-19
 * 
 * Driver for Texas Instruments ADS1115 16-bit Analog-to-Digital Converter (ADC)
 * Features:
 * - 16-bit resolution (65,536 levels)
 * - I2C interface with configurable address
 * - Programmable gain amplifier (PGA): +-6.144V to +-0.256V
 * - Programmable data rates: 8 to 860 SPS
 * - Single-ended or differential inputs (4 channels)
 * 
 * Hardware Configuration:
 * - ADDR pin to GND = I2C address 0x48
 * @copyright Copyright (c) 2026
 * 
 */

 #ifndef ADS1115_H
 #define ADS1115_H

 #include <stdint.h>
 #include <stdbool.h>
 #include "hardware/i2c.h"

// ============================================================================
// REGISTER ADDRESSES
// ============================================================================

#define ADS1115_REG_CONVERSION  0x00
#define ADS1115_REG_CONFIG      0x01
#define ADS1115_REG_LO_THRESH   0x02
#define ADS1115_REG_HI_THRESH   0x03

// ============================================================================
// CONFIGURATION REGISTER BIT DEFINITIONS
// ============================================================================

#define ADS1115_OS_SINGLE           0x8000
#define ADS1115_OS_NOT_BUSY         0x8000

#define ADS1115_MUX_DIFF_0_1        0x0000
#define ADS1115_MUX_DIFF_0_3        0x1000
#define ADS1115_MUX_DIFF_1_3        0x2000
#define ADS1115_MUX_DIFF_2_3        0x3000
#define ADS1115_MUX_SINGLE_0        0x4000
#define ADS1115_MUX_SINGLE_1        0x5000
#define ADS1115_MUX_SINGLE_2        0x6000
#define ADS1115_MUX_SINGLE_3        0x7000

#define ADS1115_PGA_6_144V          0x0000
#define ADS1115_PGA_4_096V          0x0200
#define ADS1115_PGA_2_048V          0x0400
#define ADS1115_PGA_1_024V          0x0600
#define ADS1115_PGA_0_512V          0x0800
#define ADS1115_PGA_0_256V          0x0A00

#define ADS1115_MODE_CONTINUOUS     0x0000
#define ADS1115_MODE_SINGLE         0x0100

#define ADS1115_DR_8_SPS            0x0000
#define ADS1115_DR_16_SPS           0x0020
#define ADS1115_DR_32_SPS           0x0040
#define ADS1115_DR_64_SPS           0x0060
#define ADS1115_DR_128_SPS          0x0080
#define ADS1115_DR_250_SPS          0x00A0
#define ADS1115_DR_475_SPS          0x00C0
#define ADS1115_DR_860_SPS          0x00E0

#define ADS1115_COMP_QUE_DISABLE    0x0003

// ============================================================================
// ENUMERATIONS
// ============================================================================

typedef enum {
    ADS1115_GAIN_6_144V = 0,    ///< ±6.144V, 187.5 µV/bit
    ADS1115_GAIN_4_096V = 1,    ///< ±4.096V, 125 µV/bit
    ADS1115_GAIN_2_048V = 2,    ///< ±2.048V, 62.5 µV/bit
    ADS1115_GAIN_1_024V = 3,    ///< ±1.024V, 31.25 µV/bit
    ADS1115_GAIN_0_512V = 4,    ///< ±0.512V, 15.625 µV/bit
    ADS1115_GAIN_0_256V = 5     ///< ±0.256V, 7.8125 µV/bit
} ads1115_gain_t;

typedef enum {
    ADS1115_RATE_8_SPS   = 0,
    ADS1115_RATE_16_SPS  = 1,
    ADS1115_RATE_32_SPS  = 2,
    ADS1115_RATE_64_SPS  = 3,
    ADS1115_RATE_128_SPS = 4,
    ADS1115_RATE_250_SPS = 5,
    ADS1115_RATE_475_SPS = 6,
    ADS1115_RATE_860_SPS = 7
} ads1115_data_rate_t;

typedef enum {
    ADS1115_CHANNEL_0 = 0,
    ADS1115_CHANNEL_1 = 1,
    ADS1115_CHANNEL_2 = 2,
    ADS1115_CHANNEL_3 = 3
} ads1115_channel_t;

// ============================================================================
// CONFIG AND DEVICE STRUCTS
// ============================================================================

/**
 * @brief Hardware configuration — set once at init, never changes.
 */
typedef struct {
    i2c_inst_t          *i2c;       ///< I2C instance (i2c0 or i2c1)
    uint8_t              sda_pin;   ///< SDA GPIO pin
    uint8_t              scl_pin;   ///< SCL GPIO pin
    uint8_t              address;   ///< I2C address (0x48–0x4B)
    ads1115_gain_t       gain;      ///< Initial gain setting
    ads1115_data_rate_t  rate;      ///< Initial data rate
} ads1115_config_t;

/**
 * @brief Device instance — one per physical ADS1115.
 *        Pass a pointer to this to all driver functions.
 */
typedef struct {
    ads1115_config_t    config;         ///< Hardware configuration (set at init)
    ads1115_channel_t   current_channel;///< Last selected channel
    bool                initialized;    ///< Whether init succeeded
} ads1115_t;

// ============================================================================
// DEFAULT INITIALIZER
// ============================================================================

/**
 * @brief Create a config with sensible defaults for LVDT measurement.
 *        Gain ±4.096V, 128 SPS. Caller supplies the I2C wiring.
 */
static inline ads1115_config_t ads1115_create_config(
        i2c_inst_t *i2c, uint8_t sda_pin, uint8_t scl_pin, uint8_t address) {
    ads1115_config_t cfg = {
        .i2c     = i2c,
        .sda_pin = sda_pin,
        .scl_pin = scl_pin,
        .address = address,
        .gain    = ADS1115_GAIN_4_096V,
        .rate    = ADS1115_RATE_128_SPS
    };
    return cfg;
}

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

// Initialization
bool ads1115_init(ads1115_t *dev, const ads1115_config_t *config);
bool ads1115_is_present(ads1115_t *dev);

// Configuration
bool ads1115_set_gain(ads1115_t *dev, ads1115_gain_t gain);
bool ads1115_set_data_rate(ads1115_t *dev, ads1115_data_rate_t rate);

// Measurement
bool ads1115_start_conversion(ads1115_t *dev, ads1115_channel_t channel);
bool ads1115_is_conversion_ready(ads1115_t *dev);
bool ads1115_read_conversion(ads1115_t *dev, int16_t *result);
bool ads1115_read_voltage(ads1115_t *dev, ads1115_channel_t channel, float *voltage);

// Utilities
float ads1115_get_voltage_range(ads1115_gain_t gain);
uint32_t ads1115_get_conversion_time_ms(ads1115_data_rate_t rate);

// ============================================================================
// INLINE HELPERS
// ============================================================================

static inline float ads1115_raw_to_voltage(int16_t raw, ads1115_gain_t gain) {
    static const float volts_per_bit[] = {
        0.0001875f,
        0.000125f,
        0.0000625f,
        0.00003125f,
        0.000015625f,
        0.0000078125f
    };
    return raw * volts_per_bit[gain];
}

 #endif // ADS1115_H