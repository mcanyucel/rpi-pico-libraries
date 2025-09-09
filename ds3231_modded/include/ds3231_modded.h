#pragma once

#include <stdint.h>
#include <stdbool.h>

// DS3231 Hardware-Modified I2C Configuration
#define DS3231_MODDED_I2C_ADDRESS 0x68
#define DS3231_MODDED_I2C_PORT i2c0
#define DS3231_MODDED_SDA_PIN 0
#define DS3231_MODDED_SCL_PIN 1
#define DS3231_MODDED_INT_PIN 5  // INT/SQW pin connected to GP5

// DS3231 Register Addresses (same as original)
#define DS3231_MODDED_REG_SECONDS 0x00
#define DS3231_MODDED_REG_MINUTES 0x01
#define DS3231_MODDED_REG_HOURS 0x02
#define DS3231_MODDED_REG_WEEKDAY 0x03
#define DS3231_MODDED_REG_DAY 0x04
#define DS3231_MODDED_REG_MONTH 0x05
#define DS3231_MODDED_REG_YEAR 0x06
#define DS3231_MODDED_REG_ALARM1_SEC 0x07
#define DS3231_MODDED_REG_ALARM1_MIN 0x08
#define DS3231_MODDED_REG_ALARM1_HOUR 0x09
#define DS3231_MODDED_REG_ALARM1_DAY 0x0A
#define DS3231_MODDED_REG_ALARM2_MIN 0x0B
#define DS3231_MODDED_REG_ALARM2_HOUR 0x0C
#define DS3231_MODDED_REG_ALARM2_DAY 0x0D
#define DS3231_MODDED_REG_CONTROL 0x0E
#define DS3231_MODDED_REG_STATUS 0x0F
#define DS3231_MODDED_REG_TEMP_MSB 0x11
#define DS3231_MODDED_REG_TEMP_LSB 0x12

// DS3231 Time Structures (same as original for compatibility)
typedef struct {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
} ds3231_modded_time_t;

typedef struct {
    uint8_t year;       // years since 2000
    uint8_t month;      // 1-12
    uint8_t day;        // 1-31
    uint8_t weekday;    // 1-7 (Monday = 1)
} ds3231_modded_date_t;

typedef struct {
    ds3231_modded_date_t date;
    ds3231_modded_time_t time;
} ds3231_modded_datetime_t;

// Helper Functions
uint8_t ds3231_modded_dec_to_bcd(uint8_t dec);
uint8_t ds3231_modded_bcd_to_dec(uint8_t bcd);

// Low-level I2C Functions
bool ds3231_modded_write_reg(uint8_t reg, uint8_t value);
bool ds3231_modded_read_reg(uint8_t reg, uint8_t *value);
bool ds3231_modded_read_regs(uint8_t start_reg, uint8_t *buffer, uint8_t count);

// Initialization
bool ds3231_modded_init(void);
bool ds3231_modded_is_present(void);
void ds3231_modded_print_status(void);

// Time Functions
bool ds3231_modded_read_time(ds3231_modded_time_t *time);
bool ds3231_modded_read_date(ds3231_modded_date_t *date);
bool ds3231_modded_read_datetime(ds3231_modded_datetime_t *datetime);
bool ds3231_modded_set_time(const ds3231_modded_time_t *time);
bool ds3231_modded_set_date(const ds3231_modded_date_t *date);
bool ds3231_modded_set_datetime(const ds3231_modded_datetime_t *datetime);

// Alarm Functions
bool ds3231_modded_clear_alarm_flags(void);
bool ds3231_modded_enable_alarm1_interrupt(void);
bool ds3231_modded_disable_alarm1_interrupt(void);
bool ds3231_modded_check_alarm1_triggered(void);
bool ds3231_modded_set_alarm1_time(const ds3231_modded_time_t *alarm_time, bool ignore_day);
bool ds3231_modded_set_alarm1_in_seconds(uint16_t seconds_from_now);
bool ds3231_modded_set_alarm1_in_minutes(uint8_t minutes_from_now);

// Status Functions
bool ds3231_modded_read_control_register(uint8_t *control);
bool ds3231_modded_read_status_register(uint8_t *status);

// Temperature
bool ds3231_modded_read_temperature(float *temperature);

// GPIO Helper for INT Pin (with software pull-up)
bool ds3231_modded_init_interrupt_pin(void);
bool ds3231_modded_read_interrupt_pin(void);

// Hardware Mod Specific Functions
bool ds3231_modded_verify_battery_operation(void);
bool ds3231_modded_test_interrupt_functionality(void);
void ds3231_modded_print_modification_status(void);