/**
 * @file sh1106_i2c.c
 * @author LVDT Logger Project
 * @brief SH1106 OLED Display I2C Driver Implementation (1.3" 128x64)
 * @version 1.0
 * @date 2025-08-27
 * 
 * Implementation of SH1106 controller driver for 1.3" OLED displays.
 * Handles the differences from SSD1306 including column offset and page addressing.
 * 
 * @copyright Copyright (c) 2025
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "sh1106_font.h"
#include "sh1106_i2c.h"

void sh1106_calc_render_area_buflen(struct sh1106_render_area *area) {
    // calculate how long the flattened buffer will be for a render area
    area->buflen = (area->end_col - area->start_col + 1) * (area->end_page - area->start_page + 1);
}

#ifdef i2c_default

void SH1106_send_cmd(uint8_t cmd) {
    // I2C write process expects a control byte followed by data
    // this "data" can be a command or data to follow up a command
    // Co = 1, D/C = 0 => the driver expects a command
    uint8_t buf[2] = {0x80, cmd};
    i2c_write_blocking(i2c_default, SH1106_I2C_ADDR, buf, 2, false);
}

void SH1106_send_cmd_list(uint8_t *buf, int num) {
    for (int i = 0; i < num; i++)
        SH1106_send_cmd(buf[i]);
}

void SH1106_send_buf(uint8_t buf[], int buflen) {
    // For SH1106, we need to send data differently than SSD1306
    // SH1106 works better with page addressing mode
    
    uint8_t *temp_buf = malloc(buflen + 1);
    temp_buf[0] = 0x40; // Data mode
    memcpy(temp_buf + 1, buf, buflen);
    
    i2c_write_blocking(i2c_default, SH1106_I2C_ADDR, temp_buf, buflen + 1, false);
    
    free(temp_buf);
}

void SH1106_init() {
    printf("Initializing SH1106 1.3\" OLED display...\n");
    
    // SH1106 initialization sequence
    uint8_t cmds[] = {
        SH1106_SET_DISP,               // Display OFF
        SH1106_SET_DISP_CLK_DIV,       // Set display clock divide ratio/oscillator frequency
        0x80,                          // Default setting
        SH1106_SET_MUX_RATIO,          // Set multiplex ratio
        SH1106_HEIGHT - 1,             // 63 for 64 rows
        SH1106_SET_DISP_OFFSET,        // Set display offset
        0x00,                          // No offset
        SH1106_SET_DISP_START_LINE,    // Set start line address (0)
        SH1106_SET_CHARGE_PUMP,        // Charge pump setting
        0x14,                          // Enable charge pump
        SH1106_SET_MEM_MODE,           // Memory addressing mode
        0x00,                          // Horizontal addressing mode (though SH1106 prefers page mode)
        SH1106_SET_SEG_REMAP | 0x01,   // Set segment re-map (column 127 mapped to SEG0)
        SH1106_SET_COM_OUT_DIR | 0x08, // Set COM output scan direction
        SH1106_SET_COM_PIN_CFG,        // Set COM pins hardware configuration
        0x12,                          // Alternative COM pin config for 128x64
        SH1106_SET_CONTRAST,           // Set contrast control
        0xFF,                          // Maximum contrast
        SH1106_SET_PRECHARGE,          // Set pre-charge period
        0xF1,                          // Default
        SH1106_SET_VCOM_DESEL,         // Set VCOMH deselect level
        0x40,                          // 0.77*Vcc (different from SSD1306)
        SH1106_SET_ENTIRE_ON,          // Entire display on (resume to RAM content display)
        SH1106_SET_NORM_DISP,          // Set normal display (not inverted)
        SH1106_SET_PUMP_VOLTAGE,       // Set pump voltage (SH1106 specific)
        SH1106_SET_DISP | 0x01         // Display ON
    };

    SH1106_send_cmd_list(cmds, count_of(cmds));
    
    printf("SH1106 display initialized successfully\n");
}

void SH1106_scroll(bool on) {
    // SH1106 scrolling configuration (similar to SSD1306 but may behave differently)
    uint8_t cmds[] = {
        SH1106_SET_HORIZ_SCROLL | 0x00,
        0x00, // dummy byte
        0x00, // start page 0
        0x00, // time interval
        SH1106_NUM_PAGES - 1, // end page
        0x00, // dummy byte
        0xFF, // dummy byte
        SH1106_SET_SCROLL | (on ? 0x01 : 0) // Start/stop scrolling
    };

    SH1106_send_cmd_list(cmds, count_of(cmds));
}

void sh1106_render(uint8_t *buf, struct sh1106_render_area *area) {
    // SH1106 rendering with column offset
    uint8_t cmds[] = {
        SH1106_SET_COL_ADDR,
        area->start_col + SH1106_COLUMN_OFFSET,  // Add SH1106 column offset
        area->end_col + SH1106_COLUMN_OFFSET,    // Add SH1106 column offset
        SH1106_SET_PAGE_ADDR,
        area->start_page,
        area->end_page
    };

    SH1106_send_cmd_list(cmds, count_of(cmds));
    SH1106_send_buf(buf, area->buflen);
}

void sh1106_render_full_screen(uint8_t *buf) {
    // Optimized full screen rendering using page addressing mode (SH1106's preferred mode)
    for (int page = 0; page < SH1106_NUM_PAGES; page++) {
        // Set page address
        SH1106_send_cmd(SH1106_SET_PAGE_START + page);
        
        // Set column address with offset
        SH1106_send_cmd(SH1106_SET_COL_ADDR_LOW | (SH1106_COLUMN_OFFSET & 0x0F));
        SH1106_send_cmd(SH1106_SET_COL_ADDR_HIGH | ((SH1106_COLUMN_OFFSET >> 4) & 0x0F));
        
        // Send page data
        SH1106_send_buf(&buf[page * SH1106_WIDTH], SH1106_WIDTH);
    }
}

void SH1106_SetPixel(uint8_t *buf, int x, int y, bool on) {
    // Same pixel manipulation as SSD1306 - the buffer format is identical
    if (x < 0 || x >= SH1106_WIDTH || y < 0 || y >= SH1106_HEIGHT) {
        return; // Bounds check
    }

    // The video ram on the SH1106 is split up in to 8 rows, one bit per pixel.
    // Each row is 128 long by 8 pixels high, each byte vertically arranged
    const int BytesPerRow = SH1106_WIDTH;
    
    int byte_idx = (y / 8) * BytesPerRow + x;
    uint8_t byte = buf[byte_idx];

    if (on)
        byte |= 1 << (y % 8);
    else
        byte &= ~(1 << (y % 8));

    buf[byte_idx] = byte;
}

// Basic Bresenhams line algorithm
void SH1106_DrawLine(uint8_t *buf, int x0, int y0, int x1, int y1, bool on) {
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    int e2;

    while (true) {
        SH1106_SetPixel(buf, x0, y0, on);
        if (x0 == x1 && y0 == y1)
            break;
        e2 = 2 * err;

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

void SH1106_WriteChar(uint8_t *buf, int16_t x, int16_t y, uint8_t ch) {
    // Bounds checking
    if (x < 0 || x > SH1106_WIDTH - 8 || y < 0 || y > SH1106_HEIGHT - 8) {
        return;
    }

    // Get font data for this character (reuse existing font)
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
                if (pixel_x < SH1106_WIDTH && pixel_y < SH1106_HEIGHT) {
                    SH1106_SetPixel(buf, pixel_x, pixel_y, true);
                }
            }
        }
    }
}

void SH1106_WriteString(uint8_t *buf, int16_t x, int16_t y, char *str) {
    int current_x = x;
    
    while (*str && current_x < SH1106_WIDTH - 8) {
        SH1106_WriteChar(buf, current_x, y, *str);
        current_x += 8;  // Move to next character position
        str++;
    }
}

void SH1106_WriteLines(uint8_t *buf, int16_t x, int16_t y, char **lines, int line_count, int line_spacing) {
    int current_y = y;
    
    for (int i = 0; i < line_count; i++) {
        if (current_y > SH1106_HEIGHT - 8) break;  // Don't draw outside screen
        
        SH1106_WriteString(buf, x, current_y, lines[i]);
        current_y += line_spacing;
    }
}

void SH1106_WriteCentered(uint8_t *buf, int16_t y, char *str) {
    int str_len = strlen(str);
    int str_width = str_len * 8;  // Each character is 8 pixels wide
    int x = (SH1106_WIDTH - str_width) / 2;
    
    if (x < 0) x = 0;  // Don't go negative
    
    SH1106_WriteString(buf, x, y, str);
}

#endif // i2c_default