#include "ds3231.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

static uint8_t dec_to_bcd(uint8_t dec) {
    return ((dec / 10) << 4) | (dec % 10);
}

static uint8_t bcd_to_dec(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

static bool write_reg(ds3231_t *dev, uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg, value};
    return i2c_write_blocking(dev->config.i2c, dev->config.address, data, 2, false) == 2;
}

static bool read_reg(ds3231_t *dev, uint8_t reg, uint8_t *value) {
    if (i2c_write_blocking(dev->config.i2c, dev->config.address, &reg, 1, true) != 1) {
        return false;
    }
    return i2c_read_blocking(dev->config.i2c, dev->config.address, value, 1, false) == 1;
}

static bool read_regs(ds3231_t *dev, uint8_t start_reg, uint8_t *buf, uint8_t count) {
    if (i2c_write_blocking(dev->config.i2c, dev->config.address, &start_reg, 1, true) != 1) {
        return false;
    }
    return i2c_read_blocking(dev->config.i2c, dev->config.address, buf, count, false) == count;
}

// ============================================================================
// INITIALIZATION
// ============================================================================

bool ds3231_init(ds3231_t *dev, const ds3231_config_t *config) {
    if (!dev || !config) return false;

    dev->config      = *config;
    dev->initialized = false;

    i2c_init(dev->config.i2c, dev->config.baudrate);

    gpio_set_function(dev->config.sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(dev->config.scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(dev->config.sda_pin);
    gpio_pull_up(dev->config.scl_pin);

    sleep_ms(10);

    uint8_t control;
    if (!read_reg(dev, DS3231_REG_CONTROL, &control)) {
        return false;
    }

    dev->initialized = true;
    return true;
}

bool ds3231_is_present(ds3231_t *dev) {
    uint8_t val;
    return read_reg(dev, DS3231_REG_CONTROL, &val);
}

// ============================================================================
// TIME
// ============================================================================

bool ds3231_read_time(ds3231_t *dev, ds3231_time_t *time) {
    uint8_t raw[3];
    if (!read_regs(dev, DS3231_REG_SECONDS, raw, 3)) return false;

    time->seconds = bcd_to_dec(raw[0] & 0x7F);
    time->minutes = bcd_to_dec(raw[1]);
    time->hours   = bcd_to_dec(raw[2] & 0x3F);
    return true;
}

bool ds3231_read_date(ds3231_t *dev, ds3231_date_t *date) {
    uint8_t raw[4];
    if (!read_regs(dev, DS3231_REG_WEEKDAY, raw, 4)) return false;

    date->weekday = raw[0] & 0x07;
    date->day     = bcd_to_dec(raw[1]);
    date->month   = bcd_to_dec(raw[2] & 0x1F);
    date->year    = bcd_to_dec(raw[3]);
    return true;
}

bool ds3231_read_datetime(ds3231_t *dev, ds3231_datetime_t *datetime) {
    return ds3231_read_date(dev, &datetime->date) &&
           ds3231_read_time(dev, &datetime->time);
}

bool ds3231_set_time(ds3231_t *dev, const ds3231_time_t *time) {
    if (!write_reg(dev, DS3231_REG_SECONDS, dec_to_bcd(time->seconds))) return false;
    if (!write_reg(dev, DS3231_REG_MINUTES, dec_to_bcd(time->minutes))) return false;
    if (!write_reg(dev, DS3231_REG_HOURS,   dec_to_bcd(time->hours)))   return false;
    return true;
}

bool ds3231_set_date(ds3231_t *dev, const ds3231_date_t *date) {
    if (!write_reg(dev, DS3231_REG_WEEKDAY, date->weekday))                return false;
    if (!write_reg(dev, DS3231_REG_DAY,     dec_to_bcd(date->day)))        return false;
    if (!write_reg(dev, DS3231_REG_MONTH,   dec_to_bcd(date->month)))      return false;
    if (!write_reg(dev, DS3231_REG_YEAR,    dec_to_bcd(date->year)))       return false;
    return true;
}

bool ds3231_set_datetime(ds3231_t *dev, const ds3231_datetime_t *datetime) {
    return ds3231_set_date(dev, &datetime->date) &&
           ds3231_set_time(dev, &datetime->time);
}

// ============================================================================
// TEMPERATURE
// ============================================================================

bool ds3231_read_temperature(ds3231_t *dev, float *temperature) {
    uint8_t msb, lsb;
    if (!read_reg(dev, DS3231_REG_TEMP_MSB, &msb)) return false;
    if (!read_reg(dev, DS3231_REG_TEMP_LSB, &lsb)) return false;

    int16_t raw = (msb & 0x80) ? (int16_t)msb - 256 : msb;
    *temperature = raw + (lsb >> 6) * 0.25f;
    return true;
}

// ============================================================================
// ALARM
// ============================================================================

bool ds3231_clear_alarm_flags(ds3231_t *dev) {
    uint8_t status;
    if (!read_reg(dev, DS3231_REG_STATUS, &status)) return false;
    return write_reg(dev, DS3231_REG_STATUS, status & ~0x03);
}

bool ds3231_enable_alarm1_interrupt(ds3231_t *dev) {
    uint8_t control;
    if (!read_reg(dev, DS3231_REG_CONTROL, &control)) return false;
    control |=  0x05;   // Set A1IE (bit 0) and INTCN (bit 2)
    control &= ~0x40;   // Clear SQWE (bit 6)
    return write_reg(dev, DS3231_REG_CONTROL, control);
}

bool ds3231_disable_alarm1_interrupt(ds3231_t *dev) {
    uint8_t control;
    if (!read_reg(dev, DS3231_REG_CONTROL, &control)) return false;
    return write_reg(dev, DS3231_REG_CONTROL, control & ~0x01);
}

bool ds3231_check_alarm1_triggered(ds3231_t *dev) {
    uint8_t status;
    if (!read_reg(dev, DS3231_REG_STATUS, &status)) return false;
    return (status & 0x01) != 0;
}

bool ds3231_set_alarm1_time(ds3231_t *dev, const ds3231_time_t *alarm_time, bool ignore_day) {
    if (!ds3231_clear_alarm_flags(dev))                                                          return false;
    if (!write_reg(dev, DS3231_REG_ALARM1_SEC,  dec_to_bcd(alarm_time->seconds)))               return false;
    if (!write_reg(dev, DS3231_REG_ALARM1_MIN,  dec_to_bcd(alarm_time->minutes)))               return false;
    if (!write_reg(dev, DS3231_REG_ALARM1_HOUR, dec_to_bcd(alarm_time->hours)))                 return false;
    if (!write_reg(dev, DS3231_REG_ALARM1_DAY,  ignore_day ? 0x80 : 0x01))                      return false;
    return true;
}

bool ds3231_set_alarm1_in_seconds(ds3231_t *dev, uint16_t seconds_from_now) {
    ds3231_time_t now;
    if (!ds3231_read_time(dev, &now)) return false;

    uint32_t total = (uint32_t)now.hours * 3600
                   + (uint32_t)now.minutes * 60
                   + now.seconds
                   + seconds_from_now;

    ds3231_time_t target = {
        .hours   = (total / 3600) % 24,
        .minutes = (total / 60)   % 60,
        .seconds =  total         % 60
    };

    return ds3231_set_alarm1_time(dev, &target, true);
}

bool ds3231_set_alarm1_in_minutes(ds3231_t *dev, uint8_t minutes_from_now) {
    return ds3231_set_alarm1_in_seconds(dev, (uint16_t)minutes_from_now * 60);
}

// ============================================================================
// STATUS REGISTERS
// ============================================================================

bool ds3231_read_control_register(ds3231_t *dev, uint8_t *control) {
    return read_reg(dev, DS3231_REG_CONTROL, control);
}

bool ds3231_read_status_register(ds3231_t *dev, uint8_t *status) {
    return read_reg(dev, DS3231_REG_STATUS, status);
}

// ============================================================================
// INT PIN
// ============================================================================

bool ds3231_init_interrupt_pin(ds3231_t *dev) {
    if (dev->config.int_pin == DS3231_INT_PIN_NONE) return false;

    gpio_init(dev->config.int_pin);
    gpio_set_dir(dev->config.int_pin, GPIO_IN);
    gpio_pull_up(dev->config.int_pin);

    // If pin is already low at init, a stale alarm flag is asserting it — clear it
    if (!gpio_get(dev->config.int_pin)) {
        ds3231_clear_alarm_flags(dev);
    }

    return true;
}

bool ds3231_read_interrupt_pin(ds3231_t *dev) {
    return gpio_get(dev->config.int_pin);
}