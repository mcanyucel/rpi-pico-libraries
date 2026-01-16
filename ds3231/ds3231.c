#include "ds3231.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include <stdio.h>

// Helper Functions
uint8_t ds3231_dec_to_bcd(uint8_t dec) {
    return ((dec / 10) << 4) | (dec % 10);
}

uint8_t ds3231_bcd_to_dec(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

// Configuration helper
ds3231_config_t ds3231_create_config(uint8_t sda_pin, uint8_t scl_pin, uint8_t int_pin, i2c_inst_t *i2c_port) {
    ds3231_config_t config = {
        .i2c_port = i2c_port,
        .i2c_address = 0x68,
        .sda_pin = sda_pin,
        .scl_pin = scl_pin,
        .int_pin = int_pin,
        .baudrate = 100000
    };
    return config;
}

// Low-level I2C Functions
bool ds3231_write_reg(ds3231_t *device, uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg, value};
    int result = i2c_write_blocking(device->config.i2c_port, device->config.i2c_address, data, 2, false);
    return result == 2;
}

bool ds3231_read_reg(ds3231_t *device, uint8_t reg, uint8_t *value) {
    if (i2c_write_blocking(device->config.i2c_port, device->config.i2c_address, &reg, 1, true) != 1) {
        return false;
    }
    return i2c_read_blocking(device->config.i2c_port, device->config.i2c_address, value, 1, false) == 1;
}

bool ds3231_read_regs(ds3231_t *device, uint8_t start_reg, uint8_t *buffer, uint8_t count) {
    if (i2c_write_blocking(device->config.i2c_port, device->config.i2c_address, &start_reg, 1, true) != 1) {
        return false;
    }
    return i2c_read_blocking(device->config.i2c_port, device->config.i2c_address, buffer, count, false) == count;
}

// Initialization
bool ds3231_init(ds3231_t *device, const ds3231_config_t *config) {
    if (!device || !config) {
        printf("ERROR: Invalid device or config pointer\n");
        return false;
    }

    // copy configuration to device
    device->config = *config;
    device->initialized = false;

    printf("Initializing DS3231...\n");

    // Initialize I2C with hardware pull-ups
    i2c_init(device->config.i2c_port, device->config.baudrate);

    // Configure pins
    gpio_set_function(device->config.sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(device->config.scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(device->config.sda_pin);
    gpio_pull_up(device->config.scl_pin);

    printf("I2C pins configured:\n");
    printf("  SDA: GP%d\n", device->config.sda_pin);
    printf("  SCL: GP%d\n", device->config.scl_pin);

    // Give I2C some time to stabilize
    sleep_ms(100);

    // Check if DS3231 is present
    bool found = false;
    for (uint8_t addr = 0x08; addr < 0x78; ++addr)
    {
        uint8_t rxdata;
        if (i2c_read_blocking(device->config.i2c_port, addr, &rxdata, 1, false) >= 0)
        {
            printf("  Found device at 0x%02X", addr);
            if (addr == device->config.i2c_address)
            {
                printf(" <- DS3231!");
                found = true;
            }
            printf("\n");
        }
    }

    if (!found)
    {
        printf("ERROR: DS3231 not found on I2C bus!\n");
        return false;
    }

    // Test basic communication with a control register
    uint8_t control_reg;
    if (!ds3231_read_reg(device, DS3231_REG_CONTROL, &control_reg))
    {
        printf("ERROR: Cannot read DS3231 control register\n");
        return false;
    }

    // Set the initialize flag BEFORE trying to read time otherwise it will stuck in a loop
    device->initialized = true;

    // Read and display current time (if available)
    ds3231_time_t current_time;
    if (ds3231_read_time(device, &current_time))
    {
        printf("Current time: %02d:%02d:%02d\n",
               current_time.hours, current_time.minutes, current_time.seconds);
    }
    else
    {
        printf("Note: Could not read current time (normal for first setup)\n");
    }
    // Read temperature
    float temperature;
    if (ds3231_read_temperature(device, &temperature))
    {
        printf("Current temperature: %.1f°C\n", temperature);   
    }

    printf("DS3231 initialization complete\n");
    return true;

}

bool ds3231_is_present(ds3231_t *device) {
    if (!device || !device->initialized) {
        printf("ERROR: DS3231 not initialized\n");
        return false;
    }
    uint8_t test_val;
    return ds3231_read_reg(device, DS3231_REG_CONTROL, &test_val);
}

// Time Functions
bool ds3231_read_time(ds3231_t *device, ds3231_time_t *time) {
    uint8_t raw_sec, raw_min, raw_hour;

    if (!ds3231_read_reg(device, DS3231_REG_SECONDS, &raw_sec)) return false;
    if (!ds3231_read_reg(device, DS3231_REG_MINUTES, &raw_min)) return false;
    if (!ds3231_read_reg(device, DS3231_REG_HOURS, &raw_hour)) return false;

    time->seconds = ds3231_bcd_to_dec(raw_sec & 0x7F);  // Mask out oscillator bit
    time->minutes = ds3231_bcd_to_dec(raw_min);
    time->hours = ds3231_bcd_to_dec(raw_hour & 0x3F);   // Mask out 12/24 hour bit
    
    return true;
}

bool ds3231_read_date(ds3231_t *device, ds3231_date_t *date) {
    uint8_t raw_day, raw_month, raw_year, raw_weekday;

    if (!ds3231_read_reg(device, DS3231_REG_WEEKDAY, &raw_weekday)) return false;
    if (!ds3231_read_reg(device, DS3231_REG_DAY, &raw_day)) return false;
    if (!ds3231_read_reg(device, DS3231_REG_MONTH, &raw_month)) return false;
    if (!ds3231_read_reg(device, DS3231_REG_YEAR, &raw_year)) return false;

    date->weekday = raw_weekday & 0x07;
    date->day = ds3231_bcd_to_dec(raw_day);
    date->month = ds3231_bcd_to_dec(raw_month & 0x1F);  // Mask out century bit
    date->year = ds3231_bcd_to_dec(raw_year);
    
    return true;
}

bool ds3231_read_datetime(ds3231_t *device, ds3231_datetime_t *datetime) {
    return ds3231_read_date(device, &datetime->date) && ds3231_read_time(device, &datetime->time);
}

bool ds3231_set_time(ds3231_t *device, const ds3231_time_t *time) {
    if (!ds3231_write_reg(device, DS3231_REG_SECONDS, ds3231_dec_to_bcd(time->seconds))) return false;
    if (!ds3231_write_reg(device, DS3231_REG_MINUTES, ds3231_dec_to_bcd(time->minutes))) return false;
    if (!ds3231_write_reg(device, DS3231_REG_HOURS, ds3231_dec_to_bcd(time->hours))) return false;

    return true;
}

bool ds3231_set_date(ds3231_t *device, const ds3231_date_t *date) {
    if (!ds3231_write_reg(device, DS3231_REG_WEEKDAY, date->weekday)) return false;
    if (!ds3231_write_reg(device, DS3231_REG_DAY, ds3231_dec_to_bcd(date->day))) return false;
    if (!ds3231_write_reg(device, DS3231_REG_MONTH, ds3231_dec_to_bcd(date->month))) return false;
    if (!ds3231_write_reg(device, DS3231_REG_YEAR, ds3231_dec_to_bcd(date->year))) return false;

    return true;
}

bool ds3231_set_datetime(ds3231_t *device, const ds3231_datetime_t *datetime) {
    return ds3231_set_date(device, &datetime->date) && ds3231_set_time(device, &datetime->time);
}

// Alarm Functions
bool ds3231_clear_alarm_flags(ds3231_t *device) {
    uint8_t status;
    if (!ds3231_read_reg(device, DS3231_REG_STATUS, &status)) {
        printf("ERROR: Failed to read status register\n");
        return false;
    }
    
    printf("Status register before clear: 0x%02X\n", status);
    
    // Clear A1F and A2F bits (bits 0 and 1)
    status &= ~0x03;
    bool result = ds3231_write_reg(device, DS3231_REG_STATUS, status);
    
    if (result) {
        printf("Alarm flags cleared successfully\n");
    } else {
        printf("ERROR: Failed to clear alarm flags\n");
    }
    
    return result;
}

bool ds3231_enable_alarm1_interrupt(ds3231_t *device) {
    uint8_t control;
    if (!ds3231_read_reg(device, DS3231_REG_CONTROL, &control)) {
        printf("ERROR: Failed to read control register\n");
        return false;
    }
    
    printf("Control register before setup: 0x%02X\n", control);
    
    // Set A1IE (bit 0) and INTCN (bit 2), clear SQWE (bit 6)
    control |= 0x05;  // Set bits 0 and 2
    control &= ~0x40; // Clear bit 6 (disable square wave)

    bool result = ds3231_write_reg(device, DS3231_REG_CONTROL, control);

    if (result) {
        printf("Alarm interrupt enabled (control: 0x%02X)\n", control);
    } else {
        printf("ERROR: Failed to enable alarm interrupt\n");
    }
    
    return result;
}

bool ds3231_disable_alarm1_interrupt(ds3231_t *device) {
    uint8_t control;
    if (!ds3231_read_reg(device, DS3231_REG_CONTROL, &control)) {
        return false;
    }
    
    // Clear A1IE (bit 0)
    control &= ~0x01;
    return ds3231_write_reg(device, DS3231_REG_CONTROL, control);
}

bool ds3231_check_alarm1_triggered(ds3231_t *device) {
    uint8_t status;
    if (!ds3231_read_reg(device, DS3231_REG_STATUS, &status)) {
        return false;
    }
    
    // Check A1F bit (bit 0)
    return (status & 0x01) != 0;
}

bool ds3231_set_alarm1_time(ds3231_t *device, const ds3231_time_t *alarm_time, bool ignore_day) {
    // Clear alarm flags first
    if (!ds3231_clear_alarm_flags(device)) {
        return false;
    }
    
    printf("Setting alarm for: %02d:%02d:%02d\n", 
           alarm_time->hours, alarm_time->minutes, alarm_time->seconds);
    
    // Set alarm registers (match on seconds, minutes, hours)
    if (!ds3231_write_reg(device, DS3231_REG_ALARM1_SEC, ds3231_dec_to_bcd(alarm_time->seconds))) {
        printf("ERROR: Failed to write alarm seconds\n");
        return false;
    }
    if (!ds3231_write_reg(device, DS3231_REG_ALARM1_MIN, ds3231_dec_to_bcd(alarm_time->minutes))) {
        printf("ERROR: Failed to write alarm minutes\n");
        return false;
    }
    if (!ds3231_write_reg(device, DS3231_REG_ALARM1_HOUR, ds3231_dec_to_bcd(alarm_time->hours))) {
        printf("ERROR: Failed to write alarm hours\n");
        return false;
    }
    
    // Day register - ignore day if requested
    uint8_t day_reg = ignore_day ? 0x80 : 0x01; // DY/DT = 0, AM4 = 1 to ignore day
    if (!ds3231_write_reg(device, DS3231_REG_ALARM1_DAY, day_reg)) {
        printf("ERROR: Failed to write alarm day\n");
        return false;
    }
    
    printf("Alarm registers set successfully\n");
    return true;
}

bool ds3231_set_alarm1_in_seconds(ds3231_t *device, uint16_t seconds_from_now) {
    // Read current time
    ds3231_time_t current_time;
    if (!ds3231_read_time(device, &current_time)) {
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
    
    ds3231_time_t target_time;
    target_time.hours = (total_seconds / 3600) % 24;
    target_time.minutes = (total_seconds / 60) % 60;
    target_time.seconds = total_seconds % 60;
    
    printf("Setting alarm for %d seconds from now\n", seconds_from_now);
    
    return ds3231_set_alarm1_time(device, &target_time, true); // Ignore day
}

bool ds3231_set_alarm1_in_minutes(ds3231_t *device, uint8_t minutes_from_now) {
    return ds3231_set_alarm1_in_seconds(device, minutes_from_now * 60);
}

// Status Functions
bool ds3231_read_control_register(ds3231_t *device, uint8_t *control) {
    return ds3231_read_reg(device, DS3231_REG_CONTROL, control);
}

bool ds3231_read_status_register(ds3231_t *device, uint8_t *status) {
    return ds3231_read_reg(device, DS3231_REG_STATUS, status);
}

// Temperature
bool ds3231_read_temperature(ds3231_t *device, float *temperature) {
    uint8_t temp_msb, temp_lsb;

    if (!ds3231_read_reg(device, DS3231_REG_TEMP_MSB, &temp_msb)) return false;
    if (!ds3231_read_reg(device, DS3231_REG_TEMP_LSB, &temp_lsb)) return false;

    // Convert temperature (10-bit value, MSB is integer, LSB bits 7-6 are fractional)
    int16_t temp_int = temp_msb;
    if (temp_msb & 0x80) {
        temp_int = temp_int - 256; // Handle negative temperatures
    }
    
    float temp_frac = (temp_lsb >> 6) * 0.25f; // Each bit represents 0.25°C
    
    *temperature = temp_int + temp_frac;
    return true;
}

// GPIO Helper for INT pin
bool ds3231_init_interrupt_pin(ds3231_t *device) {
    printf("Configuring INT/SQW pin (GP%d)...\n", device->config.int_pin);
    gpio_init(device->config.int_pin);
    gpio_set_dir(device->config.int_pin, GPIO_IN);
    gpio_pull_up(device->config.int_pin);  // INT/SQW is open-drain, needs pull-up

    // Check initial state
    bool initial_state = gpio_get(device->config.int_pin);
    printf("INT/SQW pin initial state: %s\n", initial_state ? "HIGH" : "LOW");
    
    if (!initial_state) {
        printf("WARNING: INT pin is LOW initially - clearing alarm flags...\n");
        ds3231_clear_alarm_flags(device);
        sleep_ms(100);
        initial_state = gpio_get(device->config.int_pin);
        printf("After clearing flags, INT pin: %s\n", initial_state ? "HIGH" : "LOW");
    }
    
    return true;
}

bool ds3231_read_interrupt_pin(ds3231_t *device) {
    return gpio_get(device->config.int_pin);
}