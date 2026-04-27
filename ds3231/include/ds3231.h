/**
 * @file ds3231.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2026-02-19
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#ifndef DS3231_H
#define DS3231_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"

// ============================================================================
// REGISTER ADDRESSES
// ============================================================================

#define DS3231_REG_SECONDS      0x00
#define DS3231_REG_MINUTES      0x01
#define DS3231_REG_HOURS        0x02
#define DS3231_REG_WEEKDAY      0x03
#define DS3231_REG_DAY          0x04
#define DS3231_REG_MONTH        0x05
#define DS3231_REG_YEAR         0x06
#define DS3231_REG_ALARM1_SEC   0x07
#define DS3231_REG_ALARM1_MIN   0x08
#define DS3231_REG_ALARM1_HOUR  0x09
#define DS3231_REG_ALARM1_DAY   0x0A
#define DS3231_REG_ALARM2_MIN   0x0B
#define DS3231_REG_ALARM2_HOUR  0x0C
#define DS3231_REG_ALARM2_DAY   0x0D
#define DS3231_REG_CONTROL      0x0E
#define DS3231_REG_STATUS       0x0F
#define DS3231_REG_TEMP_MSB     0x11
#define DS3231_REG_TEMP_LSB     0x12

#define DS3231_DEFAULT_ADDRESS  0x68
#define DS3231_DEFAULT_BAUDRATE 100000
#define DS3231_INT_PIN_NONE     0xFF    ///< Pass when INT pin is not wired

// ============================================================================
// DATA STRUCTURES
// ============================================================================

typedef struct {
    i2c_inst_t *i2c;
    uint8_t     address;
    uint8_t     sda_pin;
    uint8_t     scl_pin;
    uint8_t     int_pin;    ///< GPIO for INT/SQW, or DS3231_INT_PIN_NONE
    uint32_t    baudrate;
} ds3231_config_t;

typedef struct {
    ds3231_config_t config;
    bool            initialized;
} ds3231_t;

typedef struct {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
} ds3231_time_t;

typedef struct {
    uint8_t year;       ///< Years since 2000
    uint8_t month;      ///< 1–12
    uint8_t day;        ///< 1–31
    uint8_t weekday;    ///< 1–7
} ds3231_date_t;

typedef struct {
    ds3231_date_t date;
    ds3231_time_t time;
} ds3231_datetime_t;

// ============================================================================
// CONVENIENCE INITIALIZER
// ============================================================================

static inline ds3231_config_t ds3231_create_config(
        i2c_inst_t *i2c, uint8_t sda_pin, uint8_t scl_pin, uint8_t int_pin) {
    ds3231_config_t cfg = {
        .i2c      = i2c,
        .address  = DS3231_DEFAULT_ADDRESS,
        .sda_pin  = sda_pin,
        .scl_pin  = scl_pin,
        .int_pin  = int_pin,
        .baudrate = DS3231_DEFAULT_BAUDRATE
    };
    return cfg;
}

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

// Initialization
bool ds3231_init(ds3231_t *dev, const ds3231_config_t *config);
bool ds3231_is_present(ds3231_t *dev);

// Time
bool ds3231_read_time(ds3231_t *dev, ds3231_time_t *time);
bool ds3231_read_date(ds3231_t *dev, ds3231_date_t *date);
bool ds3231_read_datetime(ds3231_t *dev, ds3231_datetime_t *datetime);
bool ds3231_set_time(ds3231_t *dev, const ds3231_time_t *time);
bool ds3231_set_date(ds3231_t *dev, const ds3231_date_t *date);
bool ds3231_set_datetime(ds3231_t *dev, const ds3231_datetime_t *datetime);

// Temperature
bool ds3231_read_temperature(ds3231_t *dev, float *temperature);

// Alarm
bool ds3231_clear_alarm_flags(ds3231_t *dev);
bool ds3231_enable_alarm1_interrupt(ds3231_t *dev);
bool ds3231_disable_alarm1_interrupt(ds3231_t *dev);
bool ds3231_check_alarm1_triggered(ds3231_t *dev);
bool ds3231_set_alarm1_time(ds3231_t *dev, const ds3231_time_t *alarm_time, bool ignore_day);
bool ds3231_set_alarm1_in_seconds(ds3231_t *dev, uint16_t seconds_from_now);
bool ds3231_set_alarm1_in_minutes(ds3231_t *dev, uint8_t minutes_from_now);

// INT pin
bool ds3231_init_interrupt_pin(ds3231_t *dev);
bool ds3231_read_interrupt_pin(ds3231_t *dev);

// Status registers (raw access for diagnostics)
bool ds3231_read_control_register(ds3231_t *dev, uint8_t *control);
bool ds3231_read_status_register(ds3231_t *dev, uint8_t *status);
bool ds3231_write_status_register(ds3231_t *dev, uint8_t status);

#endif // DS3231_H