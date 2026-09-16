#include "eeprom_24xx.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include <string.h>

// Transfers of many bytes need more than the fixed timeout allows
static uint32_t xfer_timeout_us(size_t len) {
  return EEPROM_24XX_XFER_TIMEOUT_US +
         (uint32_t)len *
             100; // 100 µs per byte is a safe upper bound for a busy bus
}

bool eeprom_24xx_wait_ready(eeprom_24xx_t *dev, uint32_t timeout_ms) {
  if (!dev)
    return false;

  uint64_t deadline = time_us_64() + (uint64_t)timeout_ms * 1000;

  for (;;) {
    // The device stops ACKing its address while the write cycle runs
    uint8_t probe;

    if (i2c_read_timeout_us(dev->config.i2c, dev->config.address, &probe, 1,
                            false, EEPROM_24XX_XFER_TIMEOUT_US) == 1) {
      return true;
    }
    if (time_us_64() > deadline) {
      return false;
    }
    sleep_us(200);
  }
}

bool eeprom_24xx_is_present(eeprom_24xx_t *dev) {
  if (!dev)
    return false;

  uint8_t probe;
  return i2c_read_timeout_us(dev->config.i2c, dev->config.address, &probe, 1,
                             false, EEPROM_24XX_XFER_TIMEOUT_US) == 1;
}

bool eeprom_24xx_init(eeprom_24xx_t *dev, const eeprom_24xx_config_t *config) {
  if (!dev || !config)
    return false;
  if (config->page_size == 0 || config->page_size > EEPROM_24XX_MAX_PAGE_SIZE)
    return false;
  if (config->size_bytes == 0)
    return false;

  dev->config = *config;
  dev->initialized = false;

  if (dev->config.init_bus) {
    i2c_init(dev->config.i2c, dev->config.baudrate);
    gpio_set_function(dev->config.sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(dev->config.scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(dev->config.sda_pin);
    gpio_pull_up(dev->config.scl_pin);
    sleep_ms(10); // allow bus to settle
  }

  // A write left running by a reset still blocks the bus; wait it out
  if (!eeprom_24xx_wait_ready(dev, dev->config.write_cycle_ms * 4 + 5))
    return false;

  dev->initialized = true;
  return true;
}

static bool write_page(eeprom_24xx_t *dev, uint16_t addr, const void *data,
                       size_t len) {
  uint8_t buf[EEPROM_24XX_MAX_PAGE_SIZE + 2];
  buf[0] = (uint8_t)(addr >> 8);
  buf[1] = (uint8_t)(addr & 0xFF);
  memcpy(&buf[2], data, len);

  if (i2c_write_timeout_us(dev->config.i2c, dev->config.address, buf, len + 2,
                           false, xfer_timeout_us(len + 2)) != (int)(len + 2)) {
    return false;
  }

  // Poll instead of sleeping for the datasheet time; a write usually finishes
  // sooner
  return eeprom_24xx_wait_ready(dev, dev->config.write_cycle_ms * 4 + 5);
}

bool eeprom_24xx_write(eeprom_24xx_t *dev, uint16_t addr, const void *data,
                       size_t len) {
  if (!dev || !dev->initialized || (!data && len))
    return false;
  if ((uint32_t)addr + len > dev->config.size_bytes)
    return false;

  const uint8_t *src = (const uint8_t *)data;
  while (len) {
    // A write past a page boundary wraps to the start of the same page instead
    // of carrying on, so split here
    size_t room = dev->config.page_size - (addr % dev->config.page_size);
    size_t chunk = (len < room) ? len : room;

    if (!write_page(dev, addr, src, chunk))
      return false;

    addr += (uint16_t)chunk;
    src += chunk;
    len -= chunk;
  }
  return true;
}

bool eeprom_24xx_read(eeprom_24xx_t *dev, uint16_t addr, void *data,
                      size_t len) {
  if (!dev || !dev->initialized || (!data && len))
    return false;
  if ((uint32_t)addr + len > dev->config.size_bytes)
    return false;
  if (len == 0)
    return true;

  uint8_t word[2] = {(uint8_t)(addr >> 8), (uint8_t)(addr & 0xFF)};

  // Repeated START: no STOP between the address and the read
  if (i2c_write_timeout_us(dev->config.i2c, dev->config.address, word, 2, true,
                           EEPROM_24XX_XFER_TIMEOUT_US) != 2) {
    return false;
  }

  return i2c_read_timeout_us(dev->config.i2c, dev->config.address,
                             (uint8_t *)data, len, false,
                             xfer_timeout_us(len)) == (int)len;
}

bool eeprom_24xx_write_verify(eeprom_24xx_t *dev, uint16_t addr,
                              const void *data, size_t len) {
  if (!eeprom_24xx_write(dev, addr, data, len))
    return false;

  const uint8_t *src = (const uint8_t *)data;
  uint8_t chunk[EEPROM_24XX_MAX_PAGE_SIZE];
  size_t done = 0;

  while (done < len) {
    size_t n = (len - done < sizeof(chunk)) ? len - done : sizeof(chunk);
    if (!eeprom_24xx_read(dev, (uint16_t)(addr + done), chunk, n))
      return false;
    if (memcmp(chunk, src + done, n) != 0)
      return false;
    done += n;
  }
  return true;
}
