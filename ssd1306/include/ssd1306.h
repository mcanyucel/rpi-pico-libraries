/**
 * @file ssd1306.h
 * @brief SSD1306 OLED display driver
 * @version 3.0
 *
 * Instance-based driver. All state lives in ssd1306_t.
 * No global variables, no compile-time pin configuration.
 */

#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "hardware/i2c.h"

// ============================================================================
// DISPLAY PARAMETERS
// ============================================================================

#define SSD1306_HEIGHT          64
#define SSD1306_WIDTH           128
#define SSD1306_PAGE_HEIGHT     8
#define SSD1306_NUM_PAGES       (SSD1306_HEIGHT / SSD1306_PAGE_HEIGHT)
#define SSD1306_BUF_LEN         (SSD1306_NUM_PAGES * SSD1306_WIDTH)

#define SSD1306_DEFAULT_ADDRESS 0x3C
#define SSD1306_DEFAULT_BAUDRATE 400000

// ============================================================================
// COMMANDS
// ============================================================================

#define SSD1306_SET_MEM_MODE        0x20
#define SSD1306_SET_COL_ADDR        0x21
#define SSD1306_SET_PAGE_ADDR       0x22
#define SSD1306_SET_HORIZ_SCROLL    0x26
#define SSD1306_SET_SCROLL          0x2E
#define SSD1306_SET_DISP_START_LINE 0x40
#define SSD1306_SET_CONTRAST        0x81
#define SSD1306_SET_CHARGE_PUMP     0x8D
#define SSD1306_SET_SEG_REMAP       0xA0
#define SSD1306_SET_ENTIRE_ON       0xA4
#define SSD1306_SET_NORM_DISP       0xA6
#define SSD1306_SET_MUX_RATIO       0xA8
#define SSD1306_SET_DISP            0xAE
#define SSD1306_SET_COM_OUT_DIR     0xC0
#define SSD1306_SET_DISP_OFFSET     0xD3
#define SSD1306_SET_DISP_CLK_DIV    0xD5
#define SSD1306_SET_PRECHARGE       0xD9
#define SSD1306_SET_COM_PIN_CFG     0xDA
#define SSD1306_SET_VCOM_DESEL      0xDB

// ============================================================================
// RENDER AREA
// ============================================================================

typedef struct {
    uint8_t start_col;
    uint8_t end_col;
    uint8_t start_page;
    uint8_t end_page;
    int     buflen;
} ssd1306_render_area_t;

#define SSD1306_FULL_SCREEN_AREA() ((ssd1306_render_area_t){ \
    .start_col  = 0,                    \
    .end_col    = SSD1306_WIDTH - 1,    \
    .start_page = 0,                    \
    .end_page   = SSD1306_NUM_PAGES - 1,\
    .buflen     = SSD1306_BUF_LEN       \
})

// ============================================================================
// BUFFER MACROS
// ============================================================================

#define SSD1306_CLEAR_BUFFER(buf) memset(buf, 0x00, SSD1306_BUF_LEN)
#define SSD1306_FILL_BUFFER(buf)  memset(buf, 0xFF, SSD1306_BUF_LEN)

// ============================================================================
// CONFIG AND DEVICE STRUCTS
// ============================================================================

typedef struct {
    i2c_inst_t *i2c;
    uint8_t     sda_pin;
    uint8_t     scl_pin;
    uint8_t     address;
    uint32_t    baudrate;
} ssd1306_config_t;

typedef struct {
    ssd1306_config_t config;
    bool             initialized;
} ssd1306_t;

// ============================================================================
// CONVENIENCE INITIALIZER
// ============================================================================

static inline ssd1306_config_t ssd1306_create_config(
        i2c_inst_t *i2c, uint8_t sda_pin, uint8_t scl_pin) {
    ssd1306_config_t cfg = {
        .i2c      = i2c,
        .sda_pin  = sda_pin,
        .scl_pin  = scl_pin,
        .address  = SSD1306_DEFAULT_ADDRESS,
        .baudrate = SSD1306_DEFAULT_BAUDRATE
    };
    return cfg;
}

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

// Initialization
bool ssd1306_init(ssd1306_t *dev, const ssd1306_config_t *config);
void ssd1306_deinit(ssd1306_t *dev);
bool ssd1306_is_present(ssd1306_t *dev);

// Display control
void ssd1306_display_on(ssd1306_t *dev, bool on);
void ssd1306_scroll(ssd1306_t *dev, bool on);

// Rendering
void ssd1306_calc_render_area_buflen(ssd1306_render_area_t *area);
void ssd1306_render(ssd1306_t *dev, uint8_t *buf, ssd1306_render_area_t *area);

// Graphics
void ssd1306_set_pixel(ssd1306_t *dev, uint8_t *buf, int x, int y, bool on);
void ssd1306_draw_line(ssd1306_t *dev, uint8_t *buf, int x0, int y0, int x1, int y1, bool on);
void ssd1306_write_char(ssd1306_t *dev, uint8_t *buf, int16_t x, int16_t y, uint8_t ch);
void ssd1306_write_string(ssd1306_t *dev, uint8_t *buf, int16_t x, int16_t y, const char *str);
void ssd1306_write_centered(ssd1306_t *dev, uint8_t *buf, int16_t y, const char *str);

#endif // SSD1306_H