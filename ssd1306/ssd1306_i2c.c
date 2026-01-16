/**
 * @file ssd1306_i2c.c
 * @brief SSD1306 OLED Display I2C Driver Implementation - Refactored Stateless API
 * @version 2.0
 * @date 2026-01-16
 *
 * Stateless driver implementation matching SH1106 design pattern.
 * 
 * @copyright 2021 Raspberry Pi (Trading) Ltd.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "ssd1306_font.h"
#include "ssd1306_i2c.h"

// ============================================================================
// INTERNAL STATE (Private to this file)
// ============================================================================

static bool ssd1306_initialized = false;

// ============================================================================
// RENDER AREA CALCULATIONS
// ============================================================================

void calc_render_area_buflen(struct render_area *area) {
    // Calculate how long the flattened buffer will be for a render area
    area->buflen = (area->end_col - area->start_col + 1) * (area->end_page - area->start_page + 1);
}

// ============================================================================
// LOW-LEVEL I2C FUNCTIONS
// ============================================================================

void SSD1306_send_cmd(uint8_t cmd) {
    // I2C write process expects a control byte followed by data
    // this "data" can be a command or data to follow up a command
    // Co = 1, D/C = 0 => the driver expects a command
    uint8_t buf[2] = {0x80, cmd};
    i2c_write_blocking(SSD1306_I2C_INSTANCE, SSD1306_I2C_ADDR, buf, 2, false);
}

void SSD1306_send_cmd_list(uint8_t *buf, int num) {
    for (int i = 0; i < num; i++)
        SSD1306_send_cmd(buf[i]);
}

void SSD1306_send_buf(uint8_t buf[], int buflen) {
    // In horizontal addressing mode, the column address pointer auto-increments
    // and then wraps around to the next page, so we can send the entire frame
    // buffer in one go!

    // Copy our frame buffer into a new buffer because we need to add the control byte
    // to the beginning
    uint8_t *temp_buf = malloc(buflen + 1);
    
    temp_buf[0] = 0x40;  // Data mode
    memcpy(temp_buf + 1, buf, buflen);

    i2c_write_blocking(SSD1306_I2C_INSTANCE, SSD1306_I2C_ADDR, temp_buf, buflen + 1, false);

    free(temp_buf);
}

// ============================================================================
// INITIALIZATION AND CONTROL
// ============================================================================

void SSD1306_init(void) {
    // Some of these commands are not strictly necessary as the reset
    // process defaults to some of these but they are shown here
    // to demonstrate what the initialization sequence looks like
    // Some configuration values are recommended by the board manufacturer

    printf("Initializing SSD1306 0.96\" OLED display...\n");
    
    // Initialize I2C
    printf("  I2C: %s @ %d kHz\n", 
           SSD1306_I2C_INSTANCE == i2c0 ? "i2c0" : "i2c1", 
           SSD1306_I2C_CLK);
    i2c_init(SSD1306_I2C_INSTANCE, SSD1306_I2C_CLK * 1000);
    
    // Configure GPIO pins for I2C
    gpio_set_function(SSD1306_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SSD1306_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SSD1306_SDA_PIN);
    gpio_pull_up(SSD1306_SCL_PIN);
    
    printf("  SDA: GP%d\n", SSD1306_SDA_PIN);
    printf("  SCL: GP%d\n", SSD1306_SCL_PIN);
    
    sleep_ms(100); // Wait for the display to power up

    uint8_t cmds[] = {
        SSD1306_SET_DISP,               // Set display off
        /* memory mapping */
        SSD1306_SET_MEM_MODE,           // Set memory address mode
        0x00,                           // Horizontal addressing mode
        /* resolution and layout */
        SSD1306_SET_DISP_START_LINE,    // Set display start line to 0
        SSD1306_SET_SEG_REMAP | 0x01,   // Set segment re-map (column 127 = SEG0)
        SSD1306_SET_MUX_RATIO,          // Set multiplex ratio
        SSD1306_HEIGHT - 1,             // Display height - 1
        SSD1306_SET_COM_OUT_DIR | 0x08, // Set COM output scan direction
        SSD1306_SET_DISP_OFFSET,        // Set display offset
        0x00,                           // No offset
        SSD1306_SET_COM_PIN_CFG,        // Set COM pins hardware configuration
        SSD1306_HEIGHT == 32 ? 0x02 : 0x12,  // 0x02 for 128x32, 0x12 for 128x64
        /* timing and driving scheme */
        SSD1306_SET_DISP_CLK_DIV,       // Set display clock divide ratio
        0x80,                           // Default setting
        SSD1306_SET_PRECHARGE,          // Set pre-charge period
        0xF1,                           // Recommended value
        SSD1306_SET_VCOM_DESEL,         // Set VCOMH deselect level
        0x30,                           // 0.83*Vcc
        /* display */
        SSD1306_SET_CONTRAST,           // Set contrast control
        0xFF,                           // Maximum contrast
        SSD1306_SET_ENTIRE_ON,          // Set entire display on/off
        SSD1306_SET_NORM_DISP,          // Set normal/inverse display
        SSD1306_SET_CHARGE_PUMP,        // Charge pump setting
        0x14,                           // Enable charge pump
        SSD1306_SET_SCROLL | 0x00,      // Deactivate scroll
        SSD1306_SET_DISP | 0x01,        // Turn display on
    };

    SSD1306_send_cmd_list(cmds, count_of(cmds));
    ssd1306_initialized = true;
    printf("  SSD1306 initialized successfully\n");
}

void SSD1306_deinit(void) {
    if (!ssd1306_initialized)
        return;

    // Turn off the display
    uint8_t display_off_cmd = 0xAE;
    i2c_write_blocking(SSD1306_I2C_INSTANCE, SSD1306_I2C_ADDR, &display_off_cmd, 1, false);

    // Clear the display buffer
    uint8_t clear_buffer[1025] = {0x40}; // Control byte + 1024 zeros for 128x64
    i2c_write_blocking(SSD1306_I2C_INSTANCE, SSD1306_I2C_ADDR, clear_buffer, sizeof(clear_buffer), false);

    // CRITICAL: Remove I2C function to disable pullups and prevent backfeeding
    gpio_set_function(SSD1306_SDA_PIN, GPIO_FUNC_SIO);
    gpio_set_function(SSD1306_SCL_PIN, GPIO_FUNC_SIO);

    // Drive I2C pins LOW to eliminate any current path
    gpio_set_dir(SSD1306_SDA_PIN, GPIO_OUT);
    gpio_set_dir(SSD1306_SCL_PIN, GPIO_OUT);
    gpio_put(SSD1306_SDA_PIN, 0);
    gpio_put(SSD1306_SCL_PIN, 0);

    // Mark as uninitialized
    ssd1306_initialized = false;
}

void SSD1306_scroll(bool on) {
    // Configure horizontal scrolling
    uint8_t cmds[] = {
        SSD1306_SET_HORIZ_SCROLL | 0x00,
        0x00,                                // Dummy byte
        0x00,                                // Start page 0
        0x00,                                // Time interval
        SSD1306_NUM_PAGES - 1,               // End page
        0x00,                                // Dummy byte
        0xFF,                                // Dummy byte
        SSD1306_SET_SCROLL | (on ? 0x01 : 0) // Start/stop scrolling
    };

    SSD1306_send_cmd_list(cmds, count_of(cmds));
}

void SSD1306_display_on(bool on) {
    // Turn display on (0xAF) or off (0xAE)
    uint8_t cmd = SSD1306_SET_DISP | (on ? 0x01 : 0x00);
    SSD1306_send_cmd(cmd);

    if (on) {
        printf("SSD1306 display turned ON\n");
    } else {
        printf("SSD1306 display turned OFF (power saving mode)\n");
    }
}

void SSD1306_display_off(void) {
    SSD1306_display_on(false);
}

void SSD1306_display_on_simple(void) {
    SSD1306_display_on(true);
}

// ============================================================================
// RENDER FUNCTION
// ============================================================================

void render(uint8_t *buf, struct render_area *area) {
    // Update a portion of the display with a render area
    uint8_t cmds[] = {
        SSD1306_SET_COL_ADDR,
        area->start_col,
        area->end_col,
        SSD1306_SET_PAGE_ADDR,
        area->start_page,
        area->end_page
    };

    SSD1306_send_cmd_list(cmds, count_of(cmds));
    SSD1306_send_buf(buf, area->buflen);
}

// ============================================================================
// GRAPHICS FUNCTIONS
// ============================================================================

void SetPixel(uint8_t *buf, int x, int y, bool on) {
    assert(x >= 0 && x < SSD1306_WIDTH && y >= 0 && y < SSD1306_HEIGHT);

    // The calculation to determine the correct bit to set depends on which address
    // mode we are in. This code assumes horizontal

    // The video ram on the SSD1306 is split up in to 8 rows, one bit per pixel.
    // Each row is 128 long by 8 pixels high, each byte vertically arranged, so byte 0 is x=0, y=0->7,
    // byte 1 is x = 1, y=0->7 etc

    const int BytesPerRow = SSD1306_WIDTH; // x pixels, 1bpp, but each row is 8 pixel high

    int byte_idx = (y / 8) * BytesPerRow + x;
    uint8_t byte = buf[byte_idx];

    if (on)
        byte |= 1 << (y % 8);
    else
        byte &= ~(1 << (y % 8));

    buf[byte_idx] = byte;
}

// Basic Bresenham's line algorithm
void DrawLine(uint8_t *buf, int x0, int y0, int x1, int y1, bool on) {
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true) {
        SetPixel(buf, x0, y0, on);
        
        if (x0 == x1 && y0 == y1)
            break;
            
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void WriteChar(uint8_t *buf, int16_t x, int16_t y, uint8_t ch) {
    // Bounds checking
    if (x < 0 || x > SSD1306_WIDTH - 8 || y < 0 || y > SSD1306_HEIGHT - 8) {
        return;
    }

    // Get font data for this character
    ch = toupper(ch);
    int font_idx = GetFontIndex(ch);
    
    // Process each column of the 8x8 character
    for (int col = 0; col < 8; col++) {
        uint8_t font_col = font[font_idx * 8 + col];  // Get font column data
        
        // Process each pixel in this column
        for (int row = 0; row < 8; row++) {
            if (font_col & (1 << row)) {  // If this pixel should be set
                int pixel_x = x + col;
                int pixel_y = y + row;
                
                // Make sure pixel is within bounds
                if (pixel_x < SSD1306_WIDTH && pixel_y < SSD1306_HEIGHT) {
                    SetPixel(buf, pixel_x, pixel_y, true);
                }
            }
        }
    }
}

void WriteString(uint8_t *buf, int16_t x, int16_t y, const char *str) {
    int current_x = x;
    
    while (*str && current_x < SSD1306_WIDTH - 8) {
        WriteChar(buf, current_x, y, *str);
        current_x += 8;  // Move to next character position
        str++;
    }
}

void WriteCentered(uint8_t *buf, int16_t y, char *str) {
    size_t len = strlen(str);
    int16_t x = (SSD1306_WIDTH - (len * 8)) / 2;
    
    if (x < 0)
        x = 0;
        
    WriteString(buf, x, y, str);
}