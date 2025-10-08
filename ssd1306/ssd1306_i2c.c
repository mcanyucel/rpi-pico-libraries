/**
 * @file ssd1306_i2c.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2025-08-19
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

ssd1306_i2c_config_t ssd1306_i2c_create_config(uint8_t sda_pin, uint8_t scl_pin, i2c_inst_t *i2c_port)
{
    ssd1306_i2c_config_t config = {
        .i2c_port = i2c_port,
        .i2c_address = 0x3C,
        .sda_pin = sda_pin,
        .scl_pin = scl_pin,
        .baudrate = SSD1306_I2C_CLK * 1000};
    return config;
}

void calc_render_area_buflen(struct render_area *area)
{
    // calculate how long the flattened buffer will be for a render area
    area->buflen = (area->end_col - area->start_col + 1) * (area->end_page - area->start_page + 1);
}

void SSD1306_send_cmd(ssd1306_device_t *device, uint8_t cmd)
{
    // I2C write process expects a control byte followed by data
    // this "data" can be a command or data to follow up a command
    // Co = 1, D/C = 0 => the driver expects a command
    uint8_t buf[2] = {0x80, cmd};
    i2c_write_blocking(device->config.i2c_port, device->config.i2c_address, buf, 2, false);
}

void SSD1306_send_cmd_list(ssd1306_device_t *device, uint8_t *buf, int num)
{
    for (int i = 0; i < num; i++)
        SSD1306_send_cmd(device, buf[i]);
}

void SSD1306_send_buf(ssd1306_device_t *device, uint8_t buf[], int buflen)
{
    // in horizontal addressing mode, the column address pointer auto-increments
    // and then wraps around to the next page, so we can send the entire frame
    // buffer in one go!

    // copy our frame buffer into a new buffer because we need to add the control byte
    // to the beginning

    uint8_t *temp_buf = malloc(buflen + 1);

    temp_buf[0] = 0x40;
    memcpy(temp_buf + 1, buf, buflen);

    i2c_write_blocking(device->config.i2c_port, device->config.i2c_address, temp_buf, buflen + 1, false);

    free(temp_buf);
}

void SSD1306_init(ssd1306_device_t *device, const ssd1306_i2c_config_t *config)
{
    // Some of these commands are not strictly necessary as the reset
    // process defaults to some of these but they are shown here
    // to demonstrate what the initialization sequence looks like
    // Some configuration values are recommended by the board manufacturer

    if (!device || !config)
    {
        printf("ERROR: Invalid device or config pointer\n");
        return;
    }

    // Copy configuration to device
    device->config = *config;
    device->initialized = false;

    // Initialize I2C
    printf("Initializing I2C at %d kHz\n", device->config.baudrate / 1000);
    i2c_init(device->config.i2c_port, device->config.baudrate);
    gpio_set_function(device->config.sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(device->config.scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(device->config.sda_pin);
    gpio_pull_up(device->config.scl_pin);
    printf("I2C pins configured:\n");
    printf("  SDA: GP%d\n", device->config.sda_pin);
    printf("  SCL: GP%d\n", device->config.scl_pin);
    sleep_ms(100); // Wait for the display to power up

    uint8_t cmds[] = {
        SSD1306_SET_DISP, // set display off
        /* memory mapping */
        SSD1306_SET_MEM_MODE, // set memory address mode 0 = horizontal, 1 = vertical, 2 = page
        0x00,                 // horizontal addressing mode
        /* resolution and layout */
        SSD1306_SET_DISP_START_LINE,    // set display start line to 0
        SSD1306_SET_SEG_REMAP | 0x01,   // set segment re-map, column address 127 is mapped to SEG0
        SSD1306_SET_MUX_RATIO,          // set multiplex ratio
        SSD1306_HEIGHT - 1,             // Display height - 1
        SSD1306_SET_COM_OUT_DIR | 0x08, // set COM (common) output scan direction. Scan from bottom up, COM[N-1] to COM0
        SSD1306_SET_DISP_OFFSET,        // set display offset
        0x00,                           // no offset
        SSD1306_SET_COM_PIN_CFG,        // set COM (common) pins hardware configuration. Board specific magic number.
                                        // 0x02 Works for 128x32, 0x12 Possibly works for 128x64. Other options 0x22, 0x32
#if ((SSD1306_WIDTH == 128) && (SSD1306_HEIGHT == 32))
        0x02,
#elif ((SSD1306_WIDTH == 128) && (SSD1306_HEIGHT == 64))
        0x12,
#else
        0x02,
#endif
        /* timing and driving scheme */
        SSD1306_SET_DISP_CLK_DIV, // set display clock divide ratio
        0x80,                     // div ratio of 1, standard freq
        SSD1306_SET_PRECHARGE,    // set pre-charge period
        0xF1,                     // Vcc internally generated on our board
        SSD1306_SET_VCOM_DESEL,   // set VCOMH deselect level
        0x30,                     // 0.83xVcc
        /* display */
        SSD1306_SET_CONTRAST, // set contrast control
        0xFF,
        SSD1306_SET_ENTIRE_ON,     // set entire display on to follow RAM content
        SSD1306_SET_NORM_DISP,     // set normal (not inverted) display
        SSD1306_SET_CHARGE_PUMP,   // set charge pump
        0x14,                      // Vcc internally generated on our board
        SSD1306_SET_SCROLL | 0x00, // deactivate horizontal scrolling if set. This is necessary as memory writes will corrupt if scrolling was enabled
        SSD1306_SET_DISP | 0x01,   // turn display on
    };

    SSD1306_send_cmd_list(device, cmds, count_of(cmds));
    device->initialized = true;
    printf("SSD1306 initialized\n");
}

void SSD1306_deinit(ssd1306_device_t *device)
{
    if (!device || !device->initialized)
        return;

    // Turn off the display
    uint8_t display_off_cmd = 0xAE;
    i2c_write_blocking(device->config.i2c_port, device->config.i2c_address, &display_off_cmd, 1, false);

    // Clear the display buffer
    uint8_t clear_buffer[1025] = {0x40}; // Control byte + 1024 zeros for 128x64
    i2c_write_blocking(device->config.i2c_port, device->config.i2c_address, clear_buffer, sizeof(clear_buffer), false);

    // CRITICAL: Remove I2C function to disable pullups and prevent backfeeding
    gpio_set_function(device->config.sda_pin, GPIO_FUNC_SIO);
    gpio_set_function(device->config.scl_pin, GPIO_FUNC_SIO);

    // Drive I2C pins LOW to eliminate any current path
    gpio_set_dir(device->config.sda_pin, GPIO_OUT);
    gpio_set_dir(device->config.scl_pin, GPIO_OUT);
    gpio_put(device->config.sda_pin, 0);
    gpio_put(device->config.scl_pin, 0);

    // Mark as uninitialized
    device->initialized = false;
}

void SSD1306_scroll(ssd1306_device_t *device, bool on)
{
    // configure horizontal scrolling
    uint8_t cmds[] = {
        SSD1306_SET_HORIZ_SCROLL | 0x00,
        0x00,                                // dummy byte
        0x00,                                // start page 0
        0x00,                                // time interval
        SSD1306_NUM_PAGES - 1,               // end page
        0x00,                                // dummy byte
        0xFF,                                // dummy byte
        SSD1306_SET_SCROLL | (on ? 0x01 : 0) // Start/stop scrolling
    };

    SSD1306_send_cmd_list(device, cmds, count_of(cmds));
}

void SSD1306_display_on(ssd1306_device_t *device, bool on)
{
    // Turn display on (0xAF) or off (0xAE)
    uint8_t cmd = SSD1306_SET_DISP | (on ? 0x01 : 0x00);
    SSD1306_send_cmd(device, cmd);

    if (on)
    {
        printf("SSD1306 display turned ON\n");
    }
    else
    {
        printf("SSD1306 display turned OFF (power saving mode)\n");
    }
}

void SSD1306_display_off(ssd1306_device_t *device)
{
    SSD1306_display_on(device, false);
}

void SSD1306_display_on_simple(ssd1306_device_t *device)
{
    SSD1306_display_on(device, true);
}

void render(ssd1306_device_t *device, uint8_t *buf, struct render_area *area)
{
    // update a portion of the display with a render area
    uint8_t cmds[] = {
        SSD1306_SET_COL_ADDR,
        area->start_col,
        area->end_col,
        SSD1306_SET_PAGE_ADDR,
        area->start_page,
        area->end_page};

    SSD1306_send_cmd_list(device, cmds, count_of(cmds));
    SSD1306_send_buf(device, buf, area->buflen);
}

void SetPixel(uint8_t *buf, int x, int y, bool on)
{
    assert(x >= 0 && x < SSD1306_WIDTH && y >= 0 && y < SSD1306_HEIGHT);

    // The calculation to determine the correct bit to set depends on which address
    // mode we are in. This code assumes horizontal

    // The video ram on the SSD1306 is split up in to 8 rows, one bit per pixel.
    // Each row is 128 long by 8 pixels high, each byte vertically arranged, so byte 0 is x=0, y=0->7,
    // byte 1 is x = 1, y=0->7 etc

    // This code could be optimised, but is like this for clarity. The compiler
    // should do a half decent job optimising it anyway.

    const int BytesPerRow = SSD1306_WIDTH; // x pixels, 1bpp, but each row is 8 pixel high, so (x / 8) * 8

    int byte_idx = (y / 8) * BytesPerRow + x;
    uint8_t byte = buf[byte_idx];

    if (on)
        byte |= 1 << (y % 8);
    else
        byte &= ~(1 << (y % 8));

    buf[byte_idx] = byte;
}
// Basic Bresenhams.
void DrawLine(uint8_t *buf, int x0, int y0, int x1, int y1, bool on)
{

    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    int e2;

    while (true)
    {
        SetPixel(buf, x0, y0, on);
        if (x0 == x1 && y0 == y1)
            break;
        e2 = 2 * err;

        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

void WriteChar(uint8_t *buf, int16_t x, int16_t y, uint8_t ch)
{
    // Bounds checking
    if (x < 0 || x > SSD1306_WIDTH - 8 || y < 0 || y > SSD1306_HEIGHT - 8)
    {
        return;
    }

    // Get font data for this character
    ch = toupper(ch);
    int font_idx = GetFontIndex(ch);

    // Process each column of the 8x8 character
    for (int col = 0; col < 8; col++)
    {
        uint8_t font_col = font[font_idx * 8 + col]; // Get font column data

        // Process each pixel in this column
        for (int row = 0; row < 8; row++)
        {
            if (font_col & (1 << row))
            { // If this pixel should be set
                int pixel_x = x + col;
                int pixel_y = y + row;

                // Make sure pixel is within bounds
                if (pixel_x < SSD1306_WIDTH && pixel_y < SSD1306_HEIGHT)
                {
                    SetPixel(buf, pixel_x, pixel_y, true);
                }
            }
        }
    }
}

void WriteString(uint8_t *buf, int16_t x, int16_t y, const char *str)
{
    int current_x = x;

    while (*str && current_x < SSD1306_WIDTH - 8)
    {
        WriteChar(buf, current_x, y, *str);
        current_x += 8; // Move to next character position
        str++;
    }
}

void WriteLines(uint8_t *buf, int16_t x, int16_t y, char **lines, int line_count, int line_spacing)
{
    int current_y = y;

    for (int i = 0; i < line_count; i++)
    {
        if (current_y > SSD1306_HEIGHT - 8)
            break; // Don't draw outside screen

        WriteString(buf, x, current_y, lines[i]);
        current_y += line_spacing;
    }
}

void WriteCentered(uint8_t *buf, int16_t y, char *str)
{
    int str_len = strlen(str);
    int str_width = str_len * 8; // Each character is 8 pixels wide
    int x = (SSD1306_WIDTH - str_width) / 2;

    if (x < 0)
        x = 0; // Don't go negative

    WriteString(buf, x, y, str);
}