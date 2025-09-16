/**
 * @file ssd1306_i2c.h
 * @author your name (you@domain.com)
 * @brief SSD1306 OLED Display I2C Driver Header
 * @version 0.1
 * @date 2025-08-19
 * 
 * @copyright 2021 Raspberry Pi (Trading) Ltd.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SSD1306_I2C_H
#define SSD1306_I2C_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// Display dimensions
#define SSD1306_HEIGHT              64
#define SSD1306_WIDTH               128

// Default I2C pins (can be overridden in your project)
#ifndef SSD1306_I2C_SDA_PIN
#define SSD1306_I2C_SDA_PIN         16   // GP4 (pin 16)
#endif

#ifndef SSD1306_I2C_SCL_PIN
#define SSD1306_I2C_SCL_PIN         17   // GP5 (pin 17)
#endif

#ifndef SSD1306_I2C_ADDR
#define SSD1306_I2C_ADDR            0x3C
#endif

#ifndef SSD1306_I2C_INSTANCE
#define SSD1306_I2C_INSTANCE i2c0
#endif

// I2C clock frequency (400kHz is standard, can go up to 1MHz for faster updates)
#ifndef SSD1306_I2C_CLK
#define SSD1306_I2C_CLK             400
#endif

// Display parameters
#define SSD1306_PAGE_HEIGHT         8
#define SSD1306_NUM_PAGES           (SSD1306_HEIGHT / SSD1306_PAGE_HEIGHT)
#define SSD1306_BUF_LEN             (SSD1306_NUM_PAGES * SSD1306_WIDTH)

// SSD1306 Commands
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
#define SSD1306_SET_ALL_ON          0xA5
#define SSD1306_SET_NORM_DISP       0xA6
#define SSD1306_SET_INV_DISP        0xA7
#define SSD1306_SET_MUX_RATIO       0xA8
#define SSD1306_SET_DISP            0xAE
#define SSD1306_SET_COM_OUT_DIR     0xC0
#define SSD1306_SET_COM_OUT_DIR_FLIP 0xC0

#define SSD1306_SET_DISP_OFFSET     0xD3
#define SSD1306_SET_DISP_CLK_DIV    0xD5
#define SSD1306_SET_PRECHARGE       0xD9
#define SSD1306_SET_COM_PIN_CFG     0xDA
#define SSD1306_SET_VCOM_DESEL      0xDB

#define SSD1306_WRITE_MODE          0xFE
#define SSD1306_READ_MODE           0xFF

/**
 * @brief Structure defining a render area on the display
 */
struct render_area {
    uint8_t start_col;      ///< Starting column (0-127)
    uint8_t end_col;        ///< Ending column (0-127)
    uint8_t start_page;     ///< Starting page (0-7)
    uint8_t end_page;       ///< Ending page (0-7)
    int buflen;             ///< Buffer length for this area (calculated)
};

// Core SSD1306 Functions
/**
 * @brief Send a single command to the SSD1306
 * @param cmd Command byte to send
 */
void SSD1306_send_cmd(uint8_t cmd);

/**
 * @brief Send a list of commands to the SSD1306
 * @param buf Array of command bytes
 * @param num Number of commands in the array
 */
void SSD1306_send_cmd_list(uint8_t *buf, int num);

/**
 * @brief Send a buffer of display data to the SSD1306
 * @param buf Buffer containing display data
 * @param buflen Length of the buffer
 */
void SSD1306_send_buf(uint8_t buf[], int buflen);

/**
 * @brief Initialize the SSD1306 display
 * Must be called before any other display operations
 */
void SSD1306_init(void);

/**
 * @brief Enable or disable horizontal scrolling
 * @param on true to enable scrolling, false to disable
 */
void SSD1306_scroll(bool on);

// Render Functions
/**
 * @brief Calculate the buffer length needed for a render area
 * @param area Pointer to render area structure
 */
void calc_render_area_buflen(struct render_area *area);

/**
 * @brief Render a buffer to a specific area of the display
 * @param buf Buffer containing display data
 * @param area Render area to update
 */
void render(uint8_t *buf, struct render_area *area);

// Graphics Functions
/**
 * @brief Set or clear a pixel in the display buffer
 * @param buf Display buffer
 * @param x X coordinate (0-127)
 * @param y Y coordinate (0-63)
 * @param on true to set pixel, false to clear pixel
 */
void SetPixel(uint8_t *buf, int x, int y, bool on);

/**
 * @brief Draw a line in the display buffer
 * @param buf Display buffer
 * @param x0 Starting X coordinate
 * @param y0 Starting Y coordinate
 * @param x1 Ending X coordinate
 * @param y1 Ending Y coordinate
 * @param on true to draw line, false to erase line
 */
void DrawLine(uint8_t *buf, int x0, int y0, int x1, int y1, bool on);

/**
 * @brief Write a character to the display buffer at any Y position
 * 
 * This improved version allows placing characters at any Y coordinate,
 * not just multiples of 8. It properly handles characters that span
 * across page boundaries.
 * 
 * @param buf Display buffer
 * @param x X coordinate (0 to SSD1306_WIDTH-8)
 * @param y Y coordinate (0 to SSD1306_HEIGHT-8)
 * @param ch Character to write
 */
void WriteChar(uint8_t *buf, int16_t x, int16_t y, uint8_t ch);

/**
 * @brief Write a string to the display buffer at any Y position
 * 
 * @param buf Display buffer
 * @param x Starting X coordinate
 * @param y Y coordinate (can be any value 0 to SSD1306_HEIGHT-8)
 * @param str Null-terminated string to write
 */
void WriteString(uint8_t *buf, int16_t x, int16_t y, char *str);

/**
 * @brief Write centered text
 * 
 * @param buf Display buffer  
 * @param y Y coordinate
 * @param str String to center
 */
void WriteCentered(uint8_t *buf, int16_t y, char *str);

/**
 * @brief Write multiple lines of text with custom line spacing
 * 
 * Convenience function to write multiple lines with specified spacing.
 * 
 * @param buf Display buffer
 * @param x Starting X coordinate
 * @param y Starting Y coordinate
 * @param lines Array of strings (null-terminated)
 * @param line_count Number of lines in the array
 * @param line_spacing Pixels between lines (e.g., 10 for tight spacing, 12 for normal)
 */
void WriteLines(uint8_t *buf, int16_t x, int16_t y, char **lines, int line_count, int line_spacing);

// Helper Macros
/**
 * @brief Create a full-screen render area
 */
#define SSD1306_FULL_SCREEN_AREA() { \
    .start_col = 0, \
    .end_col = SSD1306_WIDTH - 1, \
    .start_page = 0, \
    .end_page = SSD1306_NUM_PAGES - 1, \
    .buflen = 0 \
}

/**
 * @brief Clear entire display buffer
 * @param buf Display buffer to clear
 */
#define SSD1306_CLEAR_BUFFER(buf) memset(buf, 0, SSD1306_BUF_LEN)

/**
 * @brief Fill entire display buffer
 * @param buf Display buffer to fill
 */
#define SSD1306_FILL_BUFFER(buf) memset(buf, 0xFF, SSD1306_BUF_LEN)

#endif // SSD1306_I2C_H