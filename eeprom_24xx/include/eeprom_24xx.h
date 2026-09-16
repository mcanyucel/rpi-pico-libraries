/**
 * @file eeprom_24xx.h
 * @author Mustafa Can Yucel
 * @brief  Driver for 24xx series EEPROMs (e.g. 24LC256) using I2C interface.
 * 
 * Handles the two-byte word address, page-boundary splitting and write-cycle polling. Parts with a one-byte word 
 * address (e.g. 24C16 and smaller) are not supported.
 * @note   This driver is designed for the Raspberry Pi Pico and uses the Pico SDK.
 * @version 0.1
 * @date 2026-09-16
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "hardware/i2c.h"

#define EEPROM_24XX_DEFAULT_ADDRESS 0x50
#define EEPROM_24XX_DEFAULT_BAUDRATE 100000

// Largest page this driver buffers. 24LC256/512 use 64 and 128 bytes, respectively. 
#define EEPROM_24XX_MAX_PAGE_SIZE 128

// 24LC256: 32 KB, 64-byte pages, 5 ms write cycle
#define EEPROM_24LC256_SIZE      32768
#define EEPROM_24LC256_PAGE_SIZE 64
#define EEPROM_24LC256_WRITE_MS  5

// Guards against a stuck bus. Long transfers add time per byte
#define EEPROM_24XX_XFER_TIMEOUT_US 10000

typedef struct {
    i2c_inst_t *i2c;
    uint8_t address;
    uint8_t sda_pin;
    uint8_t scl_pin;
    uint32_t baudrate;
    bool init_bus;           ///< false when another driver has already initialized the bus, true to init it here
    uint32_t size_bytes;
    uint16_t page_size;
    uint32_t write_cycle_ms; ///< datasheet write time; polling usually completes faster, but this is a safe upper bound
} eeprom_24xx_config_t;

typedef struct {
    eeprom_24xx_config_t config;
    bool initialized;
} eeprom_24xx_t;

static inline eeprom_24xx_config_t eeprom_24xx_create_config(i2c_inst_t *i2c, uint8_t sda_pin, uint8_t scl_pin, uint8_t address) {
    eeprom_24xx_config_t cfg = {
        .i2c            = i2c,
        .address        = address,
        .sda_pin        = sda_pin,
        .scl_pin        = scl_pin,
        .baudrate       = EEPROM_24XX_DEFAULT_BAUDRATE,
        .init_bus       = true,
        .size_bytes     = EEPROM_24LC256_SIZE,
        .page_size      = EEPROM_24LC256_PAGE_SIZE,
        .write_cycle_ms = EEPROM_24LC256_WRITE_MS
    };
    return cfg;
}

bool eeprom_24xx_init(eeprom_24xx_t *dev, const eeprom_24xx_config_t *config);
bool eeprom_24xx_is_present(eeprom_24xx_t *dev);

/**
 * @brief Sequential read of @p len bytes from @p addr into @p data. 
 */
bool eeprom_24xx_read(eeprom_24xx_t *dev, uint16_t addr, void *data, size_t len);

/** @brief Write @p len bytes, splitting on page boundaries and polling each write. */
bool eeprom_24xx_write(eeprom_24xx_t *dev, uint16_t addr, const void *data, size_t len);

/** @brief Write, then read back and compare. Use where a torn write must be caught. */
bool eeprom_24xx_write_verify(eeprom_24xx_t *dev, uint16_t addr, const void *data, size_t len);

/** @brief Block until the device ACKs again, or @p timeout_ms passes. */
bool eeprom_24xx_wait_ready(eeprom_24xx_t *dev, uint32_t timeout_ms);