/**
 * @file ssd1306.c
 * @brief SSD1306 OLED display driver implementation
 * @version 3.0
 */

#include "ssd1306.h"
#include "ssd1306_font.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// ============================================================================
// INTERNAL: LOW-LEVEL I2C
// ============================================================================

static void send_cmd(ssd1306_t *dev, uint8_t cmd) {
    uint8_t buf[2] = {0x80, cmd};
    i2c_write_blocking(dev->config.i2c, dev->config.address, buf, 2, false);
}

static void send_cmd_list(ssd1306_t *dev, uint8_t *cmds, int count) {
    for (int i = 0; i < count; i++) {
        send_cmd(dev, cmds[i]);
    }
}

static void send_buf(ssd1306_t *dev, uint8_t *buf, int buflen) {
    // Prepend the data control byte (0x40) without heap allocation.
    // SSD1306_BUF_LEN + 1 is the worst case; render areas are always <= that.
    uint8_t tmp[SSD1306_BUF_LEN + 1];
    tmp[0] = 0x40;
    memcpy(tmp + 1, buf, buflen);
    i2c_write_blocking(dev->config.i2c, dev->config.address, tmp, buflen + 1, false);
}

// ============================================================================
// INITIALIZATION
// ============================================================================

bool ssd1306_init(ssd1306_t *dev, const ssd1306_config_t *config) {
    if (!dev || !config) return false;

    dev->config      = *config;
    dev->initialized = false;

    i2c_init(dev->config.i2c, dev->config.baudrate);
    gpio_set_function(dev->config.sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(dev->config.scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(dev->config.sda_pin);
    gpio_pull_up(dev->config.scl_pin);

    sleep_ms(10);

    if (!ssd1306_is_present(dev)) return false;

    uint8_t cmds[] = {
        SSD1306_SET_DISP,
        SSD1306_SET_MEM_MODE,           0x00,
        SSD1306_SET_DISP_START_LINE,
        SSD1306_SET_SEG_REMAP | 0x01,
        SSD1306_SET_MUX_RATIO,          SSD1306_HEIGHT - 1,
        SSD1306_SET_COM_OUT_DIR | 0x08,
        SSD1306_SET_DISP_OFFSET,        0x00,
        SSD1306_SET_COM_PIN_CFG,        0x12,
        SSD1306_SET_DISP_CLK_DIV,       0x80,
        SSD1306_SET_PRECHARGE,          0xF1,
        SSD1306_SET_VCOM_DESEL,         0x30,
        SSD1306_SET_CONTRAST,           0xFF,
        SSD1306_SET_ENTIRE_ON,
        SSD1306_SET_NORM_DISP,
        SSD1306_SET_CHARGE_PUMP,        0x14,
        SSD1306_SET_SCROLL | 0x00,
        SSD1306_SET_DISP | 0x01,
    };

    send_cmd_list(dev, cmds, sizeof(cmds));

    dev->initialized = true;
    return true;
}

void ssd1306_deinit(ssd1306_t *dev) {
    if (!dev || !dev->initialized) return;

    send_cmd(dev, SSD1306_SET_DISP);    // Display off

    gpio_set_function(dev->config.sda_pin, GPIO_FUNC_SIO);
    gpio_set_function(dev->config.scl_pin, GPIO_FUNC_SIO);
    gpio_set_dir(dev->config.sda_pin, GPIO_OUT);
    gpio_set_dir(dev->config.scl_pin, GPIO_OUT);
    gpio_put(dev->config.sda_pin, 0);
    gpio_put(dev->config.scl_pin, 0);

    dev->initialized = false;
}

bool ssd1306_is_present(ssd1306_t *dev) {
    // Attempt a zero-byte write — ACK means device is present
    return i2c_write_blocking(dev->config.i2c, dev->config.address, NULL, 0, false) >= 0;
}

// ============================================================================
// DISPLAY CONTROL
// ============================================================================

void ssd1306_display_on(ssd1306_t *dev, bool on) {
    send_cmd(dev, SSD1306_SET_DISP | (on ? 0x01 : 0x00));
}

void ssd1306_scroll(ssd1306_t *dev, bool on) {
    uint8_t cmds[] = {
        SSD1306_SET_HORIZ_SCROLL | 0x00,
        0x00,
        0x00,
        0x00,
        SSD1306_NUM_PAGES - 1,
        0x00,
        0xFF,
        SSD1306_SET_SCROLL | (on ? 0x01 : 0x00)
    };
    send_cmd_list(dev, cmds, sizeof(cmds));
}

// ============================================================================
// RENDERING
// ============================================================================

void ssd1306_calc_render_area_buflen(ssd1306_render_area_t *area) {
    area->buflen = (area->end_col - area->start_col + 1)
                 * (area->end_page - area->start_page + 1);
}

void ssd1306_render(ssd1306_t *dev, uint8_t *buf, ssd1306_render_area_t *area) {
    uint8_t cmds[] = {
        SSD1306_SET_COL_ADDR,  area->start_col,  area->end_col,
        SSD1306_SET_PAGE_ADDR, area->start_page, area->end_page
    };
    send_cmd_list(dev, cmds, sizeof(cmds));
    send_buf(dev, buf, area->buflen);
}

// ============================================================================
// GRAPHICS
// ============================================================================

void ssd1306_set_pixel(ssd1306_t *dev, uint8_t *buf, int x, int y, bool on) {
    (void)dev;
    if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= SSD1306_HEIGHT) return;

    int idx = (y / 8) * SSD1306_WIDTH + x;
    if (on)
        buf[idx] |=  (1 << (y % 8));
    else
        buf[idx] &= ~(1 << (y % 8));
}

void ssd1306_draw_line(ssd1306_t *dev, uint8_t *buf, int x0, int y0, int x1, int y1, bool on) {
    int dx =  abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true) {
        ssd1306_set_pixel(dev, buf, x0, y0, on);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void ssd1306_write_char(ssd1306_t *dev, uint8_t *buf, int16_t x, int16_t y, uint8_t ch) {
    if (x < 0 || x > SSD1306_WIDTH - 8 || y < 0 || y > SSD1306_HEIGHT - 8) return;

    ch = toupper(ch);
    int font_idx = GetFontIndex(ch);

    for (int col = 0; col < 8; col++) {
        uint8_t col_data = font[font_idx * 8 + col];
        for (int row = 0; row < 8; row++) {
            if (col_data & (1 << row)) {
                ssd1306_set_pixel(dev, buf, x + col, y + row, true);
            }
        }
    }
}

void ssd1306_write_string(ssd1306_t *dev, uint8_t *buf, int16_t x, int16_t y, const char *str) {
    while (*str && x <= SSD1306_WIDTH - 8) {
        ssd1306_write_char(dev, buf, x, y, (uint8_t)*str++);
        x += 8;
    }
}

void ssd1306_write_centered(ssd1306_t *dev, uint8_t *buf, int16_t y, const char *str) {
    int16_t x = (SSD1306_WIDTH - (int16_t)(strlen(str) * 8)) / 2;
    if (x < 0) x = 0;
    ssd1306_write_string(dev, buf, x, y, str);
}