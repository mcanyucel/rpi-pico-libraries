/**
 * @file battery.h
 * @author Mustafa Can Yucel (mustafacan@bridgewiz.com)
 * @brief Fixed battery monitoring utility for Raspberry Pi Pico
 * @version 0.3
 * @date 2025-08-19
 * 
 * Provides methods to read battery voltage and calculate charge percentage
 * using the VSYS ADC channel (ADC3) on Raspberry Pi Pico.
 * 
 * @copyright Copyright (c) 2025
 * 
 */

 #ifndef BATTERY_H
 #define BATTERY_H

 #include <stdbool.h>
 #include <stdint.h>

 typedef enum {
    BATTERY_STATUS_USB_POWER,   // USB powered or error (voltage <= 1.0V)
    BATTERY_STATUS_CRITICAL,    // <= 10%
    BATTERY_STATUS_LOW,         // 11-25%
    BATTERY_STATUS_OK,          // 26-75%
    BATTERY_STATUS_GOOD         // > 75%
} battery_status_t;

typedef struct {
    float voltage;              // Battery voltage in Volts
    uint8_t percentage;         // Battery charge percentage (0-100)
    uint16_t raw_adc_value;     // Raw ADC value from the battery voltage divider
    battery_status_t status;    // Battery status enum
} battery_measurement_t;

// CORRECTED ADC Constants
#define VSYS_ADC_CHANNEL    3                       // ADC3 is internally connected to VSYS
#define ADC_CONVERSION_FACTOR (3.284f / 4095.0f)     // 12-bit ADC, 3.3V reference - It should be 3.3, but measured as 3.284
#define VSYS_DIVIDER_RATIO  2.414f                    // Voltage divider ratio for VSYS - It should be 3, but empirical tests show 2.414

// GPIO Constant (for reference only - don't manipulate directly for VSYS)
#define BATTERY_SENSE_PIN  29   // GPIO29 for battery sensing

// Battery calculation constants
#define BATTERY_COEFF_A     29.756f     // Quadratic coefficient
#define BATTERY_COEFF_B     -134.67f    // Linear coefficient
#define BATTERY_COEFF_C     148.37f     // Constant term
#define MIN_VALID_VOLTAGE   1.0f        // Minimum valid voltage

/**
 * @brief Initialize the battery monitoring system
 * 
 * Initializes the ADC3 (VSYS) with proper configuration
 * 
 * @return true if initialization was successful
 * @return false if initialization failed
 */
bool battery_init(void);

/**
 * @brief Get battery voltage reading with averaging
 * 
 * Reads the VSYS ADC channel multiple times, discards invalid readings,
 * and returns the averaged actual battery voltage accounting for the 
 * internal 1/3 voltage divider.
 * 
 * @return float Battery voltage in Volts, or -1.0 on error
 */
float battery_get_voltage(void);

/**
 * @brief Get raw ADC reading with averaging and zero-rejection
 * 
 * Takes multiple ADC samples and returns the average of valid readings.
 * Automatically rejects readings that are too low or obvious outliers.
 * 
 * @return uint16_t Averaged raw ADC value, or 0 on error
 */
uint16_t battery_get_raw_adc(void);

/**
 * @brief Convert battery voltage to percentage for 18650 Li-ion cells
 * 
 * Uses a quadratic approximation optimized for 18650 Li-ion batteries:
 * percentage = 29.756 * voltage^2 - 134.67 * voltage + 148.37
 * 
 * @param voltage Battery voltage in volts
 * @return uint8_t Battery charge percentage (0-100), or 0 if voltage < 1.0V
 */
uint8_t battery_get_percentage(float voltage);

/**
 * @brief Get comprehensive battery measurement with averaging
 * 
 * Performs a complete battery reading including voltage, percentage,
 * raw ADC value, and status determination using averaged ADC readings.
 * 
 * @param measurement Pointer to a battery_measurement_t structure to fill
 * @return true if the measurement was successful
 * @return false if the measurement failed
 */
bool battery_get_measurement(battery_measurement_t *measurement);

/**
 * @brief Get human-readable battery status string
 * 
 * @param status Battery status enum value
 * @return const char* Pointer to a string representing the battery status
 */
const char* battery_get_status_string(battery_status_t status);

/**
 * @brief Get battery status based on voltage and percentage
 * 
 * @param voltage Battery voltage in volts
 * @param percentage Battery charge percentage (0-100)
 * @return battery_status_t Battery status enum value
 */
battery_status_t battery_get_status(float voltage, uint8_t percentage);

/**
 * @brief Check if the battery monitoring system is initialized
 * 
 * @return true if the system is initialized
 * @return false if the system is not initialized
 */
bool battery_is_initialized(void);

#endif // BATTERY_H