/**
 * @file ads1115.h
 * @author 
 * @brief ADS1116 16-bit ADC driver
 * @version 0.1
 * @date 2025-08-26
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
 * @copyright Copyright (c) 2025
 * 
 */

 #ifndef ADS1115_H
 #define ADS1115_H

 #include <stdint.h>
 #include <stdbool.h>

// ============================================================================
// HARDWARE CONFIGURATION
// ============================================================================

#ifndef ADS1115_I2C_PORT
#define ADS1115_I2C_PORT    i2c0
#endif

#ifndef ADS1115_I2C_ADDR
#define ADS1115_I2C_ADDR    0x48
#endif

#ifndef ADS1115_SDA_PIN
#define ADS1115_SDA_PIN     16
#endif

#ifndef ADS1115_SCL_PIN
#define ADS1115_SCL_PIN     17
#endif

// ============================================================================
// REGISTER ADDRESSES
// ============================================================================

#define ADS1115_REG_CONVERSION  0x00        ///< Conversion result register
#define ADS1115_REG_CONFIG      0x01        ///< Configuration register
#define ADS1115_REG_LO_THRESH   0x02        ///< Low threshold register
#define ADS1115_REG_HI_THRESH   0x03        ///< High threshold register

// ============================================================================
// CONFIGURATION REGISTER BIT DEFINITIONS
// ============================================================================

// Operational Status (OS) - Bit 15
#define ADS1115_OS_SINGLE       0x8000      ///< Start single conversion
#define ADS1115_OS_BUSY         0x0000      ///< Currently converting
#define ADS1115_OS_NOT_BUSY     0x8000      ///< Not currently converting

// Multiplexer Configuration (MUX) - Bits 14:12
#define ADS1115_MUX_DIFF_0_1    0x0000      ///< Differential: AIN0 - AIN1
#define ADS1115_MUX_DIFF_0_3    0x1000      ///< Differential: AIN0 - AIN3
#define ADS1115_MUX_DIFF_1_3    0x2000      ///< Differential: AIN1 - AIN3
#define ADS1115_MUX_DIFF_2_3    0x3000      ///< Differential: AIN2 - AIN3
#define ADS1115_MUX_SINGLE_0    0x4000      ///< Single-ended: AIN0 to GND
#define ADS1115_MUX_SINGLE_1    0x5000      ///< Single-ended: AIN1 to GND
#define ADS1115_MUX_SINGLE_2    0x6000      ///< Single-ended: AIN2 to GND
#define ADS1115_MUX_SINGLE_3    0x7000      ///< Single-ended: AIN3 to GND

// Programmable Gain Amplifier (PGA) - Bits 11:9
#define ADS1115_PGA_6_144V      0x0000      ///< ±6.144V (default)
#define ADS1115_PGA_4_096V      0x0200      ///< ±4.096V
#define ADS1115_PGA_2_048V      0x0400      ///< ±2.048V
#define ADS1115_PGA_1_024V      0x0600      ///< ±1.024V
#define ADS1115_PGA_0_512V      0x0800      ///< ±0.512V
#define ADS1115_PGA_0_256V      0x0A00      ///< ±0.256V

// Operating Mode (MODE) - Bit 8
#define ADS1115_MODE_CONTINUOUS 0x0000      ///< Continuous conversion mode
#define ADS1115_MODE_SINGLE     0x0100      ///< Single-shot mode (default)

// Data Rate (DR) - Bits 7:5
#define ADS1115_DR_8_SPS        0x0000      ///< 8 samples per second
#define ADS1115_DR_16_SPS       0x0020      ///< 16 samples per second
#define ADS1115_DR_32_SPS       0x0040      ///< 32 samples per second
#define ADS1115_DR_64_SPS       0x0060      ///< 64 samples per second
#define ADS1115_DR_128_SPS      0x0080      ///< 128 samples per second (default)
#define ADS1115_DR_250_SPS      0x00A0      ///< 250 samples per second
#define ADS1115_DR_475_SPS      0x00C0      ///< 475 samples per second
#define ADS1115_DR_860_SPS      0x00E0      ///< 860 samples per second

// Comparator Mode (COMP_MODE) - Bit 4
#define ADS1115_COMP_TRADITIONAL 0x0000     ///< Traditional comparator
#define ADS1115_COMP_WINDOW     0x0010      ///< Window comparator

// Comparator Polarity (COMP_POL) - Bit 3
#define ADS1115_COMP_POL_LOW    0x0000      ///< Active low (default)
#define ADS1115_COMP_POL_HIGH   0x0008      ///< Active high

// Latching Comparator (COMP_LAT) - Bit 2
#define ADS1115_COMP_LAT_NON    0x0000      ///< Non-latching (default)
#define ADS1115_COMP_LAT_LATCH  0x0004      ///< Latching

// Comparator Queue (COMP_QUE) - Bits 1:0
#define ADS1115_COMP_QUE_1      0x0000      ///< Assert after 1 conversion
#define ADS1115_COMP_QUE_2      0x0001      ///< Assert after 2 conversions
#define ADS1115_COMP_QUE_4      0x0002      ///< Assert after 4 conversions
#define ADS1115_COMP_QUE_DISABLE 0x0003     ///< Disable comparator (default)

// ============================================================================
// DEFAULT CONFIGURATIONS
// ============================================================================

// Default configuration for LVDT measurement
#define ADS1115_CONFIG_DEFAULT  (ADS1115_OS_SINGLE     |   \
                                ADS1115_MUX_SINGLE_0    |   \
                                ADS1115_PGA_4_096V      |   \
                                ADS1115_MODE_SINGLE     |   \
                                ADS1115_DR_128_SPS      |   \
                                ADS1115_COMP_QUE_DISABLE)


// ============================================================================
// DATA STRUCTURES
// ============================================================================

/**
 * @brief ADS1115 gain settings with corresponding voltage ranges
 */
typedef enum {
    ADS1115_GAIN_6_144V = 0,    ///< ±6.144V, 187.5 µV/bit
    ADS1115_GAIN_4_096V = 1,    ///< ±4.096V, 125 µV/bit
    ADS1115_GAIN_2_048V = 2,    ///< ±2.048V, 62.5 µV/bit  
    ADS1115_GAIN_1_024V = 3,    ///< ±1.024V, 31.25 µV/bit
    ADS1115_GAIN_0_512V = 4,    ///< ±0.512V, 15.625 µV/bit
    ADS1115_GAIN_0_256V = 5     ///< ±0.256V, 7.8125 µV/bit
} ads1115_gain_t;

/**
 * @brief ADS1115 data rate settings
 */
typedef enum {
    ADS1115_RATE_8_SPS = 0,     ///< 8 samples per second
    ADS1115_RATE_16_SPS = 1,    ///< 16 samples per second
    ADS1115_RATE_32_SPS = 2,    ///< 32 samples per second
    ADS1115_RATE_64_SPS = 3,    ///< 64 samples per second
    ADS1115_RATE_128_SPS = 4,   ///< 128 samples per second
    ADS1115_RATE_250_SPS = 5,   ///< 250 samples per second
    ADS1115_RATE_475_SPS = 6,   ///< 475 samples per second
    ADS1115_RATE_860_SPS = 7    ///< 860 samples per second
} ads1115_data_rate_t;

/**
 * @brief ADS1115 input channels
 */
typedef enum {
    ADS1115_CHANNEL_0 = 0,      ///< Single-ended channel 0 (A0)
    ADS1115_CHANNEL_1 = 1,      ///< Single-ended channel 1 (A1)
    ADS1115_CHANNEL_2 = 2,      ///< Single-ended channel 2 (A2)
    ADS1115_CHANNEL_3 = 3       ///< Single-ended channel 3 (A3)
} ads1115_channel_t;


// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

// Low-level I2C Functions
bool ads1115_write_reg(uint8_t reg, uint16_t value);
bool ads1115_read_reg(uint8_t reg, uint16_t *value);

// Initialization and Configuration
bool ads1115_init(void);
bool ads1115_is_present(void);
bool ads1115_set_gain(ads1115_gain_t gain);
bool ads1115_set_data_rate(ads1115_data_rate_t rate);
bool ads1115_set_channel(ads1115_channel_t channel);

// Measurement Functions
bool ads1115_start_conversion(ads1115_channel_t channel);
bool ads1115_is_conversion_ready(void);
bool ads1115_read_conversion(int16_t *result);
float ads1115_read_voltage(ads1115_channel_t channel);

// High-level Utility Functions
float ads1115_get_voltage_range(ads1115_gain_t gain);
uint32_t ads1115_get_conversion_time_ms(ads1115_data_rate_t rate);

// ============================================================================
// INLINE HELPER FUNCTIONS
// ============================================================================

/**
 * @brief Convert raw ADC value to voltage based on current gain setting
 * @param raw_value 16-bit signed ADC reading
 * @param gain Current gain setting
 * @return Voltage in volts
 */
static inline float ads1115_raw_to_voltage(int16_t raw_value, ads1115_gain_t gain) {
    // Voltage per bit for each gain setting
    static const float volts_per_bit[] = {
        0.0001875f,     // ±6.144V: 187.5 µV/bit
        0.000125f,      // ±4.096V: 125 µV/bit
        0.0000625f,     // ±2.048V: 62.5 µV/bit
        0.00003125f,    // ±1.024V: 31.25 µV/bit
        0.000015625f,   // ±0.512V: 15.625 µV/bit
        0.0000078125f   // ±0.256V: 7.8125 µV/bit
    };
    
    return raw_value * volts_per_bit[gain];
}

 #endif // ADS1115_H