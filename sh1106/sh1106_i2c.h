/**
 * @file sh1106_i2c.h
 * @author LVDT Logger Project
 * @brief SH1106 OLED Display I2C Driver Header (1.3" 128x64)
 * @version 1.0
 * @date 2025-08-27
 * 
 * Driver for SH1106 controller-based OLED displays (commonly 1.3" 128x64).
 * Similar to SSD1306 but with different addressing and initialization.
 * 
 * @copyright Copyright (c) 2025
 */

#ifndef SH1106_I2C_H
#define SH1106_I2C_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// Display dimensions
#define SH1106_HEIGHT              64
#define SH1106_WIDTH               128

// Default I2C pins (can be overridden in your project)
#ifndef SH1106_I2C_SDA_PIN
#define SH1106_I2C_SDA_PIN         16   // GP16 (pin 16)
#endif

#ifndef SH1106_I2C_SCL_PIN
#define SH1106_I2C_SCL_PIN         17   // GP17 (pin 17)
#endif

#ifndef SH1106_I2C_ADDR
#define SH1106_I2C_ADDR            0x3C
#endif

#define SH1106_I2C_INSTANCE i2c0

// I2C clock frequency (400kHz is standard, can go up to 1MHz for faster updates)
#ifndef SH1106_I2C_CLK
#define SH1106_I2C_CLK             400
#endif

// Display parameters
#define SH1106_PAGE_HEIGHT         8
#define SH1106_NUM_PAGES           (SH1106_HEIGHT / SH1106_PAGE_HEIGHT)
#define SH1106_BUF_LEN             (SH1106_NUM_PAGES * SH1106_WIDTH)

// SH1106 specific constants
#define SH1106_COLUMN_OFFSET       2    ///< SH1106 has a 2-pixel column offset

// SH1106 Commands (many are same as SSD1306, some are different)
#define SH1106_SET_COL_ADDR_LOW    0x00  // Set column address low nibble
#define SH1106_SET_COL_ADDR_HIGH   0x10  // Set column address high nibble
#define SH1106_SET_MEM_MODE        0x20
#define SH1106_SET_COL_ADDR        0x21
#define SH1106_SET_PAGE_ADDR       0x22
#define SH1106_SET_HORIZ_SCROLL    0x26
#define SH1106_SET_SCROLL          0x2E
#define SH1106_SET_PAGE_START      0xB0  // Set page start address (0xB0-0xB7)

#define SH1106_SET_DISP_START_LINE 0x40

#define SH1106_SET_CONTRAST        0x81
#define SH1106_SET_CHARGE_PUMP     0x8D

#define SH1106_SET_SEG_REMAP       0xA0
#define SH1106_SET_ENTIRE_ON       0xA4
#define SH1106_SET_ALL_ON          0xA5
#define SH1106_SET_NORM_DISP       0xA6
#define SH1106_SET_INV_DISP        0xA7
#define SH1106_SET_MUX_RATIO       0xA8
#define SH1106_SET_DISP            0xAE
#define SH1106_SET_COM_OUT_DIR     0xC0
#define SH1106_SET_COM_OUT_DIR_FLIP 0xC0

#define SH1106_SET_DISP_OFFSET     0xD3
#define SH1106_SET_DISP_CLK_DIV    0xD5
#define SH1106_SET_PRECHARGE       0xD9
#define SH1106_SET_COM_PIN_CFG     0xDA
#define SH1106_SET_VCOM_DESEL      0xDB
#define SH1106_SET_PUMP_VOLTAGE    0x30  // SH1106 specific pump voltage

#define SH1106_WRITE_MODE          0xFE
#define SH1106_READ_MODE           0xFF

/**
 * @brief Structure defining a render area on the display
 */
struct sh1106_render_area {
    uint8_t start_col;      ///< Starting column (0-127)
    uint8_t end_col;        ///< Ending column (0-127)
    uint8_t start_page;     ///< Starting page (0-7)
    uint8_t end_page;       ///< Ending page (0-7)
    int buflen;             ///< Buffer length for this area (calculated)
};

// Core SH1106 Functions
/**
 * @brief Send a single command to the SH1106
 * @param cmd Command byte to send
 */
void SH1106_send_cmd(uint8_t cmd);

/**
 * @brief Send a list of commands to the SH1106
 * @param buf Array of command bytes
 * @param num Number of commands in the array
 */
void SH1106_send_cmd_list(uint8_t *buf, int num);

/**
 * @brief Send a buffer of display data to the SH1106
 * @param buf Buffer containing display data
 * @param buflen Length of the buffer
 */
void SH1106_send_buf(uint8_t buf[], int buflen);

/**
 * @brief Initialize the SH1106 display
 * Must be called before any other display operations
 */
void SH1106_init(void);

/**
 * @brief Enable or disable horizontal scrolling
 * @param on true to enable scrolling, false to disable
 */
void SH1106_scroll(bool on);

// Render Functions
/**
 * @brief Calculate the buffer length needed for a render area
 * @param area Pointer to render area structure
 */
void sh1106_calc_render_area_buflen(struct sh1106_render_area *area);

/**
 * @brief Render a buffer to a specific area of the display
 * @param buf Buffer containing display data
 * @param area Render area to update
 */
void sh1106_render(uint8_t *buf, struct sh1106_render_area *area);

/**
 * @brief Render full screen buffer (optimized for SH1106 page mode)
 * @param buf Display buffer
 */
void sh1106_render_full_screen(uint8_t *buf);

// Graphics Functions (same interface as SSD1306 for compatibility)
/**
 * @brief Set or clear a pixel in the display buffer
 * @param buf Display buffer
 * @param x X coordinate (0-127)
 * @param y Y coordinate (0-63)
 * @param on true to set pixel, false to clear pixel
 */
void SH1106_SetPixel(uint8_t *buf, int x, int y, bool on);

/**
 * @brief Draw a line in the display buffer
 * @param buf Display buffer
 * @param x0 Starting X coordinate
 * @param y0 Starting Y coordinate
 * @param x1 Ending X coordinate
 * @param y1 Ending Y coordinate
 * @param on true to draw line, false to erase line
 */
void SH1106_DrawLine(uint8_t *buf, int x0, int y0, int x1, int y1, bool on);

/**
 * @brief Write a character to the display buffer at any Y position
 * @param buf Display buffer
 * @param x X coordinate (0 to SH1106_WIDTH-8)
 * @param y Y coordinate (0 to SH1106_HEIGHT-8)
 * @param ch Character to write
 */
void SH1106_WriteChar(uint8_t *buf, int16_t x, int16_t y, uint8_t ch);

/**
 * @brief Write a string to the display buffer at any Y position
 * @param buf Display buffer
 * @param x Starting X coordinate
 * @param y Y coordinate (can be any value 0 to SH1106_HEIGHT-8)
 * @param str Null-terminated string to write
 */
void SH1106_WriteString(uint8_t *buf, int16_t x, int16_t y, char *str);

/**
 * @brief Write centered text
 * @param buf Display buffer  
 * @param y Y coordinate
 * @param str String to center
 */
void SH1106_WriteCentered(uint8_t *buf, int16_t y, char *str);

/**
 * @brief Write multiple lines of text with custom line spacing
 * @param buf Display buffer
 * @param x Starting X coordinate
 * @param y Starting Y coordinate
 * @param lines Array of strings (null-terminated)
 * @param line_count Number of lines in the array
 * @param line_spacing Pixels between lines (e.g., 10 for tight spacing, 12 for normal)
 */
void SH1106_WriteLines(uint8_t *buf, int16_t x, int16_t y, char **lines, int line_count, int line_spacing);

// Helper Macros
/**
 * @brief Create a full-screen render area
 */
#define SH1106_FULL_SCREEN_AREA() { \
    .start_col = 0, \
    .end_col = SH1106_WIDTH - 1, \
    .start_page = 0, \
    .end_page = SH1106_NUM_PAGES - 1, \
    .buflen = 0 \
}

/**
 * @brief Clear entire display buffer
 * @param buf Display buffer to clear
 */
#define SH1106_CLEAR_BUFFER(buf) memset(buf, 0, SH1106_BUF_LEN)

/**
 * @brief Fill entire display buffer
 * @param buf Display buffer to fill
 */
#define SH1106_FILL_BUFFER(buf) memset(buf, 0xFF, SH1106_BUF_LEN)

#endif // SH1106_I2C_H