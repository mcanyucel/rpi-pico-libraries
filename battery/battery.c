/**
 * @file battery.c
 * @author Mustafa Can Yucel (mustafacan@bridgewiz.com)
 * @brief Fixed battery monitoring with proper ADC resolution and averaging
 * @version 0.3
 * @date 2025-08-19
 */

#include "battery.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include <stdio.h>

#define MAX_SAMPLES         10                      // Maximum samples for averaging
#define SETTLING_TIME_MS    50                      // ADC settling time

// Static variables
static bool initialized = false;

bool battery_init(void)
{
        if (initialized) {
        return true;
    }

    printf("Initializing battery monitoring system...\n");

    // Initialize ADC
    adc_init();
    
    // CRITICAL: Set GPIO25 HIGH to enable VSYS reading on GPIO29
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);
    gpio_put(25, 1);  // HIGH enables GPIO29 ADC
    
    // Initialize GPIO29 for battery sensing
    gpio_init(29);
    gpio_set_dir(29, GPIO_IN);
    
    adc_select_input(VSYS_ADC_CHANNEL);
    sleep_ms(200);
    
    printf("Battery monitoring initialized (ADC%d - VSYS)\n", VSYS_ADC_CHANNEL);
    
    initialized = true;
    return true;
}


static uint16_t battery_get_single_adc(void) {
    if (!initialized) {
        printf("ERROR: Battery system not initialized\n");
        return 0;
    }

    adc_select_input(VSYS_ADC_CHANNEL);
    sleep_ms(10); // Additional ADC settling

    uint16_t raw = adc_read();

    return raw;
}

/**
 * @brief Get raw ADC reading with averaging and zero-rejection
 * 
 * Takes multiple ADC samples, discards zeros and obvious outliers,
 * then returns the average of valid readings.
 */
static uint16_t battery_get_averaged_adc(void)
{
    if (!initialized) {
        printf("ERROR: Battery system not initialized\n");
        return 0;
    }

    uint16_t samples[MAX_SAMPLES];
    uint8_t valid_samples = 0;
    uint32_t sum = 0;
    
    printf("DEBUG: Taking %d ADC samples for averaging...\n", MAX_SAMPLES);
    
    // Take multiple samples
    for (int i = 0; i < MAX_SAMPLES; i++) {
        // Ensure GPIO25 is HIGH before every reading
        // gpio_put(25, 1);
        // sleep_ms(10); // Brief delay for wireless to settle
        // gpio_init(29);
        // gpio_set_dir(29, GPIO_IN);
        // sleep_ms(100); // Match Python's settling time
        
        adc_select_input(VSYS_ADC_CHANNEL);
        sleep_ms(10); // Additional ADC settling
        
        uint16_t raw = adc_read();
        printf("DEBUG: Sample %d: %d\n", i + 1, raw);
        
        if (raw > 10) {
            samples[valid_samples] = raw;
            sum += raw;
            valid_samples++;
        } else {
            printf("DEBUG: Rejecting sample %d (too low: %d)\n", i + 1, raw);
        }
        
        sleep_ms(10);
    }
    
    if (valid_samples == 0) {
        printf("ERROR: No valid ADC samples obtained\n");
        return 0;
    }
    
    uint16_t average = sum / valid_samples;
    printf("DEBUG: Valid samples: %d/%d, Average: %d\n", valid_samples, MAX_SAMPLES, average);
    
    return average;
}

float battery_get_voltage(void)
{
    if (!initialized) {
        printf("ERROR: Battery system not initialized\n");
        return -1.0f;
    }

    uint16_t raw_value = battery_get_single_adc();
    
    if (raw_value == 0) {
        printf("ERROR: Failed to get valid ADC reading\n");
        return -1.0f;
    }
    
    float voltage_12bit = (raw_value * 3.3f / 4095.0f) * 3.0f;
    return voltage_12bit;
}

uint8_t battery_get_percentage(float voltage)
{
    // Don't calculate if voltage is suspiciously low (USB powered or error)
    if (voltage < MIN_VALID_VOLTAGE) {
        return 0;
    }
    
    // Quadratic formula optimized for Li-ion 18650:
    // percentage = 29.756 * voltage^2 - 134.67 * voltage + 148.37
    float percentage = BATTERY_COEFF_A * voltage * voltage + 
                      BATTERY_COEFF_B * voltage + 
                      BATTERY_COEFF_C;
    
    // Clamp to valid range
    if (percentage < 0.0f) percentage = 0.0f;
    if (percentage > 100.0f) percentage = 100.0f;
    
    return (uint8_t)percentage;
}

battery_status_t battery_get_status(float voltage, uint8_t percentage)
{
    if (voltage < MIN_VALID_VOLTAGE) {
        return BATTERY_STATUS_USB_POWER;
    } else if (percentage > 75) {
        return BATTERY_STATUS_GOOD;
    } else if (percentage > 25) {
        return BATTERY_STATUS_OK;
    } else if (percentage > 10) {
        return BATTERY_STATUS_LOW;
    } else {
        return BATTERY_STATUS_CRITICAL;
    }
}

bool battery_get_measurement(battery_measurement_t *measurement)
{
    if (!measurement || !initialized) {
        return false;
    }

    uint16_t raw_value = battery_get_single_adc();
    
    if (raw_value == 0) {
        return false;
    }
    
    // Calculate voltage using corrected conversion factor
    float adc_voltage = raw_value * ADC_CONVERSION_FACTOR;
    float voltage = adc_voltage * VSYS_DIVIDER_RATIO;
    
    // Calculate percentage
    uint8_t percentage = battery_get_percentage(voltage);
    
    // Determine status
    battery_status_t status = battery_get_status(voltage, percentage);
    
    // Fill measurement structure
    measurement->voltage = voltage;
    measurement->percentage = percentage;
    measurement->raw_adc_value = raw_value;
    measurement->status = status;
    
    return true;
}

const char* battery_get_status_string(battery_status_t status)
{
    switch (status) {
        case BATTERY_STATUS_USB_POWER:
            return "USB Power (or Error)";
        case BATTERY_STATUS_GOOD:
            return "Good";
        case BATTERY_STATUS_OK:
            return "OK";
        case BATTERY_STATUS_LOW:
            return "Low";
        case BATTERY_STATUS_CRITICAL:
            return "Critical";
        default:
            return "Unknown";
    }
}

bool battery_is_initialized(void)
{
    return initialized;
}

uint16_t battery_get_raw_adc(void)
{
    return battery_get_single_adc();
}