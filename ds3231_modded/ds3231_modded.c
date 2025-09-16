#include "ds3231_modded.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include <stdio.h>

// Forward declarations for internal functions
static bool _ds3231_modded_write_reg(ds3231_modded_t *device, uint8_t reg, uint8_t value);
static bool _ds3231_modded_read_reg(ds3231_modded_t *device, uint8_t reg, uint8_t *value);
static bool _ds3231_modded_read_regs(ds3231_modded_t *device, uint8_t start_reg, uint8_t *buffer, uint8_t count);

// Helper Functions
uint8_t ds3231_modded_dec_to_bcd(uint8_t dec) {
    return ((dec / 10) << 4) | (dec % 10);
}

uint8_t ds3231_modded_bcd_to_dec(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

// Configuration helper
ds3231_modded_config_t ds3231_modded_create_config(uint8_t sda_pin, uint8_t scl_pin, uint8_t int_pin, i2c_inst_t *i2c_port) {
    ds3231_modded_config_t config = {
        .i2c_port = i2c_port,
        .i2c_address = 0x68,
        .sda_pin = sda_pin,     // Use the passed parameter
        .scl_pin = scl_pin,     // Use the passed parameter
        .int_pin = int_pin,     // Use the passed parameter
        .baudrate = 100000
    };
    return config;
}

// Internal I2C Functions (no initialization check)
static bool _ds3231_modded_write_reg(ds3231_modded_t *device, uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg, value};
    int result = i2c_write_blocking(device->config.i2c_port, device->config.i2c_address, data, 2, false);
    return result == 2;
}

static bool _ds3231_modded_read_reg(ds3231_modded_t *device, uint8_t reg, uint8_t *value) {
    if (i2c_write_blocking(device->config.i2c_port, device->config.i2c_address, &reg, 1, true) != 1) {
        return false;
    }
    return i2c_read_blocking(device->config.i2c_port, device->config.i2c_address, value, 1, false) == 1;
}

static bool _ds3231_modded_read_regs(ds3231_modded_t *device, uint8_t start_reg, uint8_t *buffer, uint8_t count) {
    if (i2c_write_blocking(device->config.i2c_port, device->config.i2c_address, &start_reg, 1, true) != 1) {
        return false;
    }
    return i2c_read_blocking(device->config.i2c_port, device->config.i2c_address, buffer, count, false) == count;
}

// Public I2C Functions (with initialization check)
bool ds3231_modded_write_reg(ds3231_modded_t *device, uint8_t reg, uint8_t value) {
    if (!device || !device->initialized) {
        printf("ERROR: DS3231 modded not initialized\n");
        return false;
    }
    return _ds3231_modded_write_reg(device, reg, value);
}

bool ds3231_modded_read_reg(ds3231_modded_t *device, uint8_t reg, uint8_t *value) {
    if (!device || !device->initialized) {
        printf("ERROR: DS3231 modded not initialized\n");
        return false;
    }
    return _ds3231_modded_read_reg(device, reg, value);
}

bool ds3231_modded_read_regs(ds3231_modded_t *device, uint8_t start_reg, uint8_t *buffer, uint8_t count) {
    if (!device || !device->initialized) {
        printf("ERROR: DS3231 modded not initialized\n");
        return false;
    }
    return _ds3231_modded_read_regs(device, start_reg, buffer, count);
}

// Initialization
bool ds3231_modded_init(ds3231_modded_t *device, const ds3231_modded_config_t *config) {
    if (!device || !config) {
        printf("ERROR: Invalid device or config pointer\n");
        return false;
    }
    
    // Copy configuration to device
    device->config = *config;
    device->initialized = false;
    
    printf("\n=== Initializing Hardware-Modified DS3231 ===\n");
    printf("CRITICAL: This assumes hardware pull-ups have been removed!\n");
    printf("Required modifications:\n");
    printf("  1. Pull-up resistor pack removed (4.7kΩ pack marked '472')\n");
    printf("  2. VCC NOT connected (DS3231 powered only from CR2032)\n");
    printf("  3. Charging resistor removed (recommended for safety)\n");
    printf("  4. CR2032 battery installed\n\n");
    
    // Initialize I2C with software pull-ups (CRITICAL for modded hardware)
    printf("Configuring I2C with software pull-ups...\n");
    i2c_init(device->config.i2c_port, device->config.baudrate);
    
    // Configure pins with explicit pull-ups since hardware pull-ups were removed
    gpio_set_function(device->config.sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(device->config.scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(device->config.sda_pin);  // SOFTWARE pull-up (hardware ones removed!)
    gpio_pull_up(device->config.scl_pin);  // SOFTWARE pull-up (hardware ones removed!)
    
    printf("I2C pins configured:\n");
    printf("  SDA: GP%d (with software pull-up)\n", device->config.sda_pin);
    printf("  SCL: GP%d (with software pull-up)\n", device->config.scl_pin);
    
    // Give I2C extra time to stabilize (battery operation may be slower)
    printf("Allowing I2C to stabilize (battery operation)...\n");
    sleep_ms(500);
    
    // Scan for devices to verify I2C is working
    printf("Scanning I2C bus for DS3231...\n");
    bool found = false;
    for (uint8_t addr = 0x08; addr < 0x78; ++addr) {
        uint8_t rxdata;
        if (i2c_read_blocking(device->config.i2c_port, addr, &rxdata, 1, false) >= 0) {
            printf("  Found device at 0x%02X", addr);
            if (addr == device->config.i2c_address) {
                printf(" <- DS3231!");
                found = true;
            }
            printf("\n");
        }
    }
    
    if (!found) {
        printf("ERROR: DS3231 not found at address 0x%02X!\n", device->config.i2c_address);
        printf("Troubleshooting:\n");
        printf("  1. Is CR2032 battery installed and >2.8V?\n");
        printf("  2. Are hardware pull-ups completely removed?\n");
        printf("  3. Is VCC disconnected/not powered?\n");
        printf("  4. Are SDA/SCL/GND connections secure?\n");
        return false;
    }
    
    // Test basic communication with control register
    uint8_t control_reg;
    if (!_ds3231_modded_read_reg(device, DS3231_MODDED_REG_CONTROL, &control_reg)) {
        printf("ERROR: Cannot read DS3231 control register\n");
        printf("Battery may be too low or hardware modification incomplete\n");
        return false;
    }
    
    printf("DS3231 communication successful (control reg: 0x%02X)\n", control_reg);
    
    // Verify battery operation
    if (!ds3231_modded_verify_battery_operation(device)) {
        printf("WARNING: Battery operation verification failed\n");
        // Don't fail initialization - might still work
    }
    
    // Set the initialized flag BEFORE trying to read time/temp
    device->initialized = true;
    
    // Read and display current time (if available)
    ds3231_modded_time_t current_time;
    if (ds3231_modded_read_time(device, &current_time)) {
        printf("Current time: %02d:%02d:%02d\n", 
               current_time.hours, current_time.minutes, current_time.seconds);
    } else {
        printf("Note: Could not read current time (normal for first setup)\n");
    }
    
    // Read temperature to further verify communication
    float temperature;
    if (ds3231_modded_read_temperature(device, &temperature)) {
        printf("DS3231 temperature: %.1f°C\n", temperature);
    }
    
    printf("Hardware-modified DS3231 initialized successfully!\n");
    printf("Expected dormant current: ~0mA from main supply (battery only)\n\n");
    
    return true;
}

bool ds3231_modded_is_present(ds3231_modded_t *device) {
    if (!device) {
        return false;
    }
    uint8_t test_val;
    return _ds3231_modded_read_reg(device, DS3231_MODDED_REG_CONTROL, &test_val);
}

void ds3231_modded_print_status(ds3231_modded_t *device) {
    printf("\n=== DS3231 Hardware-Modified Status ===\n");
    
    if (!device || !device->initialized) {
        printf("Status: NOT INITIALIZED\n");
        return;
    }
    
    printf("Status: INITIALIZED\n");
    
    uint8_t control, status;
    if (ds3231_modded_read_control_register(device, &control) && 
        ds3231_modded_read_status_register(device, &status)) {
        printf("Control Register: 0x%02X\n", control);
        printf("Status Register:  0x%02X\n", status);
        printf("Alarm 1 Enabled:  %s\n", (control & 0x01) ? "YES" : "NO");
        printf("Alarm 1 Flag:     %s\n", (status & 0x01) ? "SET" : "CLEAR");
        printf("Oscillator:       %s\n", (status & 0x80) ? "STOPPED" : "RUNNING");
    }
    
    float temp;
    if (ds3231_modded_read_temperature(device, &temp)) {
        printf("Temperature:      %.1f°C\n", temp);
    }
    
    ds3231_modded_datetime_t dt;
    if (ds3231_modded_read_datetime(device, &dt)) {
        printf("Current Time:     20%02d-%02d-%02d %02d:%02d:%02d\n",
               dt.date.year, dt.date.month, dt.date.day,
               dt.time.hours, dt.time.minutes, dt.time.seconds);
    }
    
    printf("=======================================\n\n");
}

// Time Functions (same logic as original, but using modded functions)
bool ds3231_modded_read_time(ds3231_modded_t *device, ds3231_modded_time_t *time) {
    uint8_t raw_sec, raw_min, raw_hour;
    
    if (!ds3231_modded_read_reg(device, DS3231_MODDED_REG_SECONDS, &raw_sec)) return false;
    if (!ds3231_modded_read_reg(device, DS3231_MODDED_REG_MINUTES, &raw_min)) return false;
    if (!ds3231_modded_read_reg(device, DS3231_MODDED_REG_HOURS, &raw_hour)) return false;
    
    time->seconds = ds3231_modded_bcd_to_dec(raw_sec & 0x7F);  // Mask out oscillator bit
    time->minutes = ds3231_modded_bcd_to_dec(raw_min);
    time->hours = ds3231_modded_bcd_to_dec(raw_hour & 0x3F);   // Mask out 12/24 hour bit
    
    return true;
}

bool ds3231_modded_read_date(ds3231_modded_t *device, ds3231_modded_date_t *date) {
    uint8_t raw_day, raw_month, raw_year, raw_weekday;
    
    if (!ds3231_modded_read_reg(device, DS3231_MODDED_REG_WEEKDAY, &raw_weekday)) return false;
    if (!ds3231_modded_read_reg(device, DS3231_MODDED_REG_DAY, &raw_day)) return false;
    if (!ds3231_modded_read_reg(device, DS3231_MODDED_REG_MONTH, &raw_month)) return false;
    if (!ds3231_modded_read_reg(device, DS3231_MODDED_REG_YEAR, &raw_year)) return false;
    
    date->weekday = raw_weekday & 0x07;
    date->day = ds3231_modded_bcd_to_dec(raw_day);
    date->month = ds3231_modded_bcd_to_dec(raw_month & 0x1F);  // Mask out century bit
    date->year = ds3231_modded_bcd_to_dec(raw_year);
    
    return true;
}

bool ds3231_modded_read_datetime(ds3231_modded_t *device, ds3231_modded_datetime_t *datetime) {
    return ds3231_modded_read_date(device, &datetime->date) && ds3231_modded_read_time(device, &datetime->time);
}

bool ds3231_modded_set_time(ds3231_modded_t *device, const ds3231_modded_time_t *time) {
    if (!ds3231_modded_write_reg(device, DS3231_MODDED_REG_SECONDS, ds3231_modded_dec_to_bcd(time->seconds))) return false;
    if (!ds3231_modded_write_reg(device, DS3231_MODDED_REG_MINUTES, ds3231_modded_dec_to_bcd(time->minutes))) return false;
    if (!ds3231_modded_write_reg(device, DS3231_MODDED_REG_HOURS, ds3231_modded_dec_to_bcd(time->hours))) return false;
    
    return true;
}

bool ds3231_modded_set_date(ds3231_modded_t *device, const ds3231_modded_date_t *date) {
    if (!ds3231_modded_write_reg(device, DS3231_MODDED_REG_WEEKDAY, date->weekday)) return false;
    if (!ds3231_modded_write_reg(device, DS3231_MODDED_REG_DAY, ds3231_modded_dec_to_bcd(date->day))) return false;
    if (!ds3231_modded_write_reg(device, DS3231_MODDED_REG_MONTH, ds3231_modded_dec_to_bcd(date->month))) return false;
    if (!ds3231_modded_write_reg(device, DS3231_MODDED_REG_YEAR, ds3231_modded_dec_to_bcd(date->year))) return false;
    
    return true;
}

bool ds3231_modded_set_datetime(ds3231_modded_t *device, const ds3231_modded_datetime_t *datetime) {
    return ds3231_modded_set_date(device, &datetime->date) && ds3231_modded_set_time(device, &datetime->time);
}

// Alarm Functions
bool ds3231_modded_clear_alarm_flags(ds3231_modded_t *device) {
    uint8_t status;
    if (!ds3231_modded_read_reg(device, DS3231_MODDED_REG_STATUS, &status)) {
        printf("ERROR: Failed to read status register\n");
        return false;
    }
    
    printf("Status register before clear: 0x%02X\n", status);
    
    // Clear A1F and A2F bits (bits 0 and 1)
    status &= ~0x03;
    bool result = ds3231_modded_write_reg(device, DS3231_MODDED_REG_STATUS, status);
    
    if (result) {
        printf("Alarm flags cleared (INT/SQW should go HIGH)\n");
    } else {
        printf("ERROR: Failed to clear alarm flags\n");
    }
    
    return result;
}

bool ds3231_modded_enable_alarm1_interrupt(ds3231_modded_t *device) {
    uint8_t control;
    if (!ds3231_modded_read_reg(device, DS3231_MODDED_REG_CONTROL, &control)) {
        printf("ERROR: Failed to read control register\n");
        return false;
    }
    
    printf("Control register before setup: 0x%02X\n", control);
    
    // Set A1IE (bit 0) and INTCN (bit 2), clear SQWE (bit 6)
    control |= 0x05;  // Set bits 0 and 2
    control &= ~0x40; // Clear bit 6
    
    bool result = ds3231_modded_write_reg(device, DS3231_MODDED_REG_CONTROL, control);
    
    if (result) {
        printf("Alarm interrupt enabled (INT/SQW will go LOW on alarm)\n");
        printf("IMPORTANT: Ensure wake pin (GP%d) has software pull-up enabled!\n", device->config.int_pin);
    } else {
        printf("ERROR: Failed to enable alarm interrupt\n");
    }
    
    return result;
}

bool ds3231_modded_disable_alarm1_interrupt(ds3231_modded_t *device) {
    uint8_t control;
    if (!ds3231_modded_read_reg(device, DS3231_MODDED_REG_CONTROL, &control)) {
        printf("ERROR: Failed to read control register\n");
        return false;
    }
    
    control &= ~0x01; // Clear A1IE bit
    bool result = ds3231_modded_write_reg(device, DS3231_MODDED_REG_CONTROL, control);
    
    if (result) {
        printf("Alarm interrupt disabled\n");
    } else {
        printf("ERROR: Failed to disable alarm interrupt\n");
    }
    
    return result;
}

bool ds3231_modded_check_alarm1_triggered(ds3231_modded_t *device) {
    uint8_t status;
    if (!ds3231_modded_read_reg(device, DS3231_MODDED_REG_STATUS, &status)) {
        return false;
    }
    return (status & 0x01) != 0;
}

bool ds3231_modded_set_alarm1_time(ds3231_modded_t *device, const ds3231_modded_time_t *alarm_time, bool ignore_day) {
    printf("Setting Alarm 1 to %02d:%02d:%02d (ignore_day=%s)\n", 
           alarm_time->hours, alarm_time->minutes, alarm_time->seconds,
           ignore_day ? "true" : "false");
    
    // Set alarm seconds register
    if (!ds3231_modded_write_reg(device, DS3231_MODDED_REG_ALARM1_SEC, 
                                ds3231_modded_dec_to_bcd(alarm_time->seconds))) {
        printf("ERROR: Failed to write alarm seconds\n");
        return false;
    }
    
    // Set alarm minutes register
    if (!ds3231_modded_write_reg(device, DS3231_MODDED_REG_ALARM1_MIN, 
                                ds3231_modded_dec_to_bcd(alarm_time->minutes))) {
        printf("ERROR: Failed to write alarm minutes\n");
        return false;
    }
    
    // Set alarm hours register
    if (!ds3231_modded_write_reg(device, DS3231_MODDED_REG_ALARM1_HOUR, 
                                ds3231_modded_dec_to_bcd(alarm_time->hours))) {
        printf("ERROR: Failed to write alarm hours\n");
        return false;
    }
    
    // Set alarm day register (with ignore flag if requested)
    uint8_t day_reg = ignore_day ? 0x80 : 0x01; // DY/DT = 0, AM4 = 1 to ignore day
    if (!ds3231_modded_write_reg(device, DS3231_MODDED_REG_ALARM1_DAY, day_reg)) {
        printf("ERROR: Failed to write alarm day\n");
        return false;
    }
    
    printf("Alarm registers set successfully\n");
    return true;
}

bool ds3231_modded_set_alarm1_in_seconds(ds3231_modded_t *device, uint16_t seconds_from_now) {
    // Read current time
    ds3231_modded_time_t current_time;
    if (!ds3231_modded_read_time(device, &current_time)) {
        printf("ERROR: Failed to read current time\n");
        return false;
    }
    
    printf("Current time: %02d:%02d:%02d\n", 
           current_time.hours, current_time.minutes, current_time.seconds);
    
    // Calculate target time
    uint32_t total_seconds = current_time.hours * 3600 + 
                           current_time.minutes * 60 + 
                           current_time.seconds + 
                           seconds_from_now;
    
    ds3231_modded_time_t target_time;
    target_time.hours = (total_seconds / 3600) % 24;
    target_time.minutes = (total_seconds / 60) % 60;
    target_time.seconds = total_seconds % 60;
    
    printf("Setting alarm for %d seconds from now\n", seconds_from_now);
    
    return ds3231_modded_set_alarm1_time(device, &target_time, true); // Ignore day
}

bool ds3231_modded_set_alarm1_in_minutes(ds3231_modded_t *device, uint8_t minutes_from_now) {
    return ds3231_modded_set_alarm1_in_seconds(device, minutes_from_now * 60);
}

// Status Functions
bool ds3231_modded_read_control_register(ds3231_modded_t *device, uint8_t *control) {
    return ds3231_modded_read_reg(device, DS3231_MODDED_REG_CONTROL, control);
}

bool ds3231_modded_read_status_register(ds3231_modded_t *device, uint8_t *status) {
    return ds3231_modded_read_reg(device, DS3231_MODDED_REG_STATUS, status);
}

// Temperature
bool ds3231_modded_read_temperature(ds3231_modded_t *device, float *temperature) {
    uint8_t temp_msb, temp_lsb;
    
    if (!ds3231_modded_read_reg(device, DS3231_MODDED_REG_TEMP_MSB, &temp_msb)) return false;
    if (!ds3231_modded_read_reg(device, DS3231_MODDED_REG_TEMP_LSB, &temp_lsb)) return false;
    
    // Convert temperature (10-bit value, MSB is integer, LSB bits 7-6 are fractional)
    int16_t temp_int = temp_msb;
    if (temp_msb & 0x80) {
        temp_int = temp_int - 256; // Handle negative temperatures
    }
    
    float temp_frac = (temp_lsb >> 6) * 0.25f; // Each bit represents 0.25°C
    
    *temperature = temp_int + temp_frac;
    return true;
}

// GPIO Helper for INT pin (with software pull-up)
bool ds3231_modded_init_interrupt_pin(ds3231_modded_t *device) {
    printf("Configuring INT/SQW pin (GP%d) with software pull-up...\n", device->config.int_pin);
    gpio_init(device->config.int_pin);
    gpio_set_dir(device->config.int_pin, GPIO_IN);
    gpio_pull_up(device->config.int_pin);  // CRITICAL: Software pull-up (hardware ones removed!)
    
    // Check initial state
    bool initial_state = gpio_get(device->config.int_pin);
    printf("INT/SQW pin initial state: %s\n", initial_state ? "HIGH (good)" : "LOW (check alarm flags)");
    
    if (!initial_state) {
        printf("WARNING: INT/SQW is LOW initially\n");
        printf("This could mean:\n");
        printf("  1. Alarm flags are set (clear them)\n");
        printf("  2. Hardware pull-up resistors not fully removed\n");
        printf("  3. VCC is still connected\n");
    }
    
    return true;
}

bool ds3231_modded_read_interrupt_pin(ds3231_modded_t *device) {
    return gpio_get(device->config.int_pin);
}

// Hardware Mod Specific Functions
bool ds3231_modded_verify_battery_operation(ds3231_modded_t *device) {
    printf("Verifying battery-only operation...\n");
    
    // Try to read temperature - this should work if battery operation is OK
    uint8_t temp_msb, temp_lsb;
    
    if (!_ds3231_modded_read_reg(device, DS3231_MODDED_REG_TEMP_MSB, &temp_msb)) {
        printf("ERROR: Cannot read temperature MSB - battery may be low\n");
        return false;
    }
    if (!_ds3231_modded_read_reg(device, DS3231_MODDED_REG_TEMP_LSB, &temp_lsb)) {
        printf("ERROR: Cannot read temperature LSB - battery may be low\n");
        return false;
    }
    
    // Convert temperature (10-bit value, MSB is integer, LSB bits 7-6 are fractional)
    int16_t temp_int = temp_msb;
    if (temp_msb & 0x80) {
        temp_int = temp_int - 256; // Handle negative temperatures
    }
    
    float temp_frac = (temp_lsb >> 6) * 0.25f; // Each bit represents 0.25°C
    float temp = temp_int + temp_frac;
    
    if (temp < -40.0f || temp > 85.0f) {
        printf("WARNING: Temperature reading unusual (%.1f°C) - check battery\n", temp);
        return false;
    }
    
    printf("Battery operation verified (temp: %.1f°C)\n", temp);
    return true;
}

bool ds3231_modded_test_interrupt_functionality(ds3231_modded_t *device) {
    printf("\n=== Testing INT/SQW Functionality (Hardware Modded) ===\n");
    
    // Initialize interrupt pin
    if (!ds3231_modded_init_interrupt_pin(device)) {
        return false;
    }
    
    // Clear any existing flags
    printf("Clearing existing alarm flags...\n");
    ds3231_modded_clear_alarm_flags(device);
    sleep_ms(100);
    
    // Check initial state
    bool initial_state = ds3231_modded_read_interrupt_pin(device);
    printf("Initial INT/SQW state: %s\n", initial_state ? "HIGH ✓" : "LOW ✗");
    
    if (!initial_state) {
        printf("ERROR: INT/SQW should be HIGH when no alarms are set\n");
        printf("Hardware modification may be incomplete\n");
        return false;
    }
    
    printf("INT/SQW pin appears to be working correctly\n");
    printf("Note: Full test requires setting an alarm and waiting for trigger\n");
    
    return true;
}

void ds3231_modded_print_modification_status(void) {
    printf("\n=== DS3231 Hardware Modification Status ===\n");
    printf("Expected modifications:\n");
    printf("  ✓ Pull-up resistor pack removed (4.7kΩ '472' package)\n");
    printf("  ✓ VCC disconnected (powered only by CR2032)\n");
    printf("  ✓ Charging resistor removed (recommended)\n");
    printf("  ✓ CR2032 battery installed\n");
    printf("  ✓ Software pull-ups enabled in code\n\n");
    
    printf("Expected power consumption:\n");
    printf("  • Active mode: ~26mA (same as before)\n");
    printf("  • Dormant mode: ~0.8mA total\n");
    printf("  • DS3231 contribution to dormant: 0mA (from main supply)\n");
    printf("  • DS3231 battery current: ~3µA standby, ~80µA during I2C\n");
    printf("  • Battery life: 5+ years with 15-minute wake cycles\n\n");
    
    printf("Troubleshooting:\n");
    printf("  • If I2C fails: Check software pull-ups enabled\n");
    printf("  • If INT/SQW always LOW: Ensure pull-up resistors removed\n");
    printf("  • If no communication: Check CR2032 voltage >2.8V\n");
    printf("  • If high dormant current: Ensure VCC not connected\n");
    printf("============================================\n\n");
}