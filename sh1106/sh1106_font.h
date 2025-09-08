/**
 * @file ssd1306_font.h
 * @author Mustafa Can Yucel - Claude 4 Sonnet (mustafacan@bridgewiz.com)
 * @brief Enhanced 8x8 bitmap font for SSD1306 displays
 * @version 0.2
 * @date 2025-08-19
 * 
 * @copyright Copyright (c) 2025
 * 
 * Enhanced 8x8 font with better readability and essential punctuation support.
 * Key improvements:
 * - Better distinction between D and O
 * - Slashed zero (0) to distinguish from O
 * - Cleaner, more readable letterforms
 * - Essential punctuation marks for better readability
 * - Better spacing and proportions
 */

#ifndef SSD1306_FONT_H
#define SSD1306_FONT_H

#include <stdint.h>

/**
 * @brief Enhanced 8x8 bitmap font with punctuation
 * 
 * Vertical bitmaps for A-Z, 0-9, and essential punctuation. Each character is 8 pixels high and 8 pixels wide.
 * Characters are defined vertically (each byte is a column) for quick copying to framebuffer.
 * 
 * Format: Each character uses 8 bytes, where:
 * - Byte 0 = leftmost column of pixels
 * - Byte 7 = rightmost column of pixels  
 * - Bit 0 = top pixel, Bit 7 = bottom pixel
 * 
 * Character order: [SPACE], A-Z, 0-9, punctuation
 * Index mapping:
 * - SPACE = 0
 * - A-Z = 1-26  
 * - 0-9 = 27-36
 * - Punctuation = 37-50
 */
static uint8_t font[] = {
    // SPACE (index 0)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    
    // A (index 1) - More pointed top, cleaner lines
    0x7C, 0x12, 0x11, 0x11, 0x11, 0x12, 0x7C, 0x00,
    
    // B (index 2) - Two distinct bumps
    0x7F, 0x49, 0x49, 0x49, 0x49, 0x49, 0x36, 0x00,
    
    // C (index 3) - More curved, open right side
    0x3E, 0x41, 0x41, 0x41, 0x41, 0x41, 0x22, 0x00,
    
    // D (index 4) - Rectangular shape to distinguish from O
    0x7F, 0x41, 0x41, 0x41, 0x41, 0x41, 0x3E, 0x00,
    
    // E (index 5) - Cleaner horizontal lines
    0x7F, 0x49, 0x49, 0x49, 0x49, 0x49, 0x41, 0x00,
    
    // F (index 6) - Like E but no bottom horizontal
    0x7F, 0x09, 0x09, 0x09, 0x09, 0x09, 0x01, 0x00,
    
    // G (index 7) - C with horizontal bar
    0x3E, 0x41, 0x41, 0x49, 0x49, 0x49, 0x3A, 0x00,
    
    // H (index 8) - Two verticals with crossbar
    0x7F, 0x08, 0x08, 0x08, 0x08, 0x08, 0x7F, 0x00,
    
    // I (index 9) - Vertical line with serifs
    0x41, 0x41, 0x41, 0x7F, 0x41, 0x41, 0x41, 0x00,
    
    // J (index 10) - Hook at bottom
    0x20, 0x40, 0x40, 0x40, 0x40, 0x40, 0x3F, 0x00,
    
    // K (index 11) - Diagonal strokes
    0x7F, 0x08, 0x08, 0x14, 0x22, 0x41, 0x00, 0x00,
    
    // L (index 12) - Vertical with bottom horizontal
    0x7F, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x00,
    
    // M (index 13) - Two peaks
    0x7F, 0x02, 0x04, 0x08, 0x04, 0x02, 0x7F, 0x00,
    
    // N (index 14) - Diagonal stroke
    0x7F, 0x02, 0x04, 0x08, 0x10, 0x20, 0x7F, 0x00,
    
    // O (index 15) - Perfect oval to distinguish from D and 0
    0x3E, 0x41, 0x41, 0x41, 0x41, 0x41, 0x3E, 0x00,
    
    // P (index 16) - Top bump only
    0x7F, 0x09, 0x09, 0x09, 0x09, 0x09, 0x06, 0x00,
    
    // Q (index 17) - O with tail
    0x3E, 0x41, 0x41, 0x51, 0x21, 0x41, 0x5E, 0x00,
    
    // R (index 18) - P with leg
    0x7F, 0x09, 0x09, 0x19, 0x29, 0x49, 0x06, 0x00,
    
    // S (index 19) - Curved S shape
    0x26, 0x49, 0x49, 0x49, 0x49, 0x49, 0x32, 0x00,
    
    // T (index 20) - Top bar with center vertical
    0x01, 0x01, 0x01, 0x7F, 0x01, 0x01, 0x01, 0x00,
    
    // U (index 21) - Curved bottom
    0x3F, 0x40, 0x40, 0x40, 0x40, 0x40, 0x3F, 0x00,
    
    // V (index 22) - Angled to point
    0x0F, 0x10, 0x20, 0x40, 0x20, 0x10, 0x0F, 0x00,
    
    // W (index 23) - Double V
    0x3F, 0x40, 0x20, 0x18, 0x20, 0x40, 0x3F, 0x00,
    
    // X (index 24) - Crossing diagonals
    0x41, 0x22, 0x14, 0x08, 0x14, 0x22, 0x41, 0x00,
    
    // Y (index 25) - V shape with vertical
    0x07, 0x08, 0x10, 0x60, 0x10, 0x08, 0x07, 0x00,
    
    // Z (index 26) - Diagonal with horizontals
    0x41, 0x61, 0x51, 0x49, 0x45, 0x43, 0x41, 0x00,
    
    // 0 (index 27) - SLASHED ZERO to distinguish from O
    0x3E, 0x51, 0x49, 0x45, 0x43, 0x41, 0x3E, 0x00,
    
    // 1 (index 28) - Vertical with small serif
    0x00, 0x42, 0x42, 0x7F, 0x40, 0x40, 0x00, 0x00,
    
    // 2 (index 29) - Clear angular shape
    0x42, 0x61, 0x51, 0x49, 0x45, 0x43, 0x42, 0x00,
    
    // 3 (index 30) - Clean two bumps on right
    0x22, 0x41, 0x49, 0x49, 0x49, 0x49, 0x36, 0x00,
    
    // 4 (index 31) - Clear vertical and horizontal
    0x18, 0x14, 0x12, 0x7F, 0x10, 0x10, 0x10, 0x00,
    
    // 5 (index 32) - Top horizontal, bottom curve
    0x27, 0x45, 0x45, 0x45, 0x45, 0x45, 0x39, 0x00,
    
    // 6 (index 33) - Clear bottom loop
    0x3C, 0x4A, 0x49, 0x49, 0x49, 0x49, 0x30, 0x00,
    
    // 7 (index 34) - Top horizontal with diagonal
    0x01, 0x01, 0x71, 0x09, 0x05, 0x03, 0x01, 0x00,
    
    // 8 (index 35) - Two clear loops
    0x36, 0x49, 0x49, 0x49, 0x49, 0x49, 0x36, 0x00,
    
    // 9 (index 36) - Clear top loop
    0x06, 0x49, 0x49, 0x49, 0x49, 0x29, 0x1E, 0x00,

    // === PUNCTUATION MARKS ===
    
    // . (period) (index 37) - Clear dot at bottom
    0x00, 0x00, 0x00, 0x60, 0x60, 0x00, 0x00, 0x00,
    
    // , (comma) (index 38) - Dot with tail
    0x00, 0x00, 0x00, 0xA0, 0x60, 0x00, 0x00, 0x00,
    
    // % (percent) (index 39) - Two circles with diagonal
    0x23, 0x13, 0x08, 0x64, 0x62, 0x36, 0x49, 0x00,
    
    // - (hyphen/minus) (index 40) - Horizontal line in middle
    0x00, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00,
    
    // : (colon) (index 41) - Two dots vertically
    0x00, 0x00, 0x36, 0x36, 0x00, 0x00, 0x00, 0x00,
    
    // ; (semicolon) (index 42) - Dot and comma
    0x00, 0x00, 0x56, 0x36, 0x00, 0x00, 0x00, 0x00,
    
    // ! (exclamation) (index 43) - Vertical line with dot
    0x00, 0x00, 0x5F, 0x00, 0x00, 0x00, 0x00, 0x00,
    
    // ? (question mark) (index 44) - Curved top with dot
    0x02, 0x01, 0x51, 0x09, 0x06, 0x00, 0x00, 0x00,
    
    // / (forward slash) (index 45) - Diagonal line
    0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x00, 0x00,
    
    // ( (left parenthesis) (index 46) - Left curve
    0x00, 0x1C, 0x22, 0x41, 0x00, 0x00, 0x00, 0x00,
    
    // ) (right parenthesis) (index 47) - Right curve  
    0x00, 0x41, 0x22, 0x1C, 0x00, 0x00, 0x00, 0x00,
    
    // + (plus) (index 48) - Horizontal and vertical cross
    0x08, 0x08, 0x3E, 0x08, 0x08, 0x00, 0x00, 0x00,
    
    // = (equals) (index 49) - Two horizontal lines
    0x14, 0x14, 0x14, 0x14, 0x14, 0x00, 0x00, 0x00,
    
    // _ (underscore) (index 50) - Bottom line
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00,
};

/**
 * @brief Get the font index for a character
 * 
 * @param ch Character to get index for
 * @return int Font array index (0 for space/unknown characters)
 */
static inline int GetFontIndex(uint8_t ch) {
    if (ch >= 'A' && ch <= 'Z') {
        return ch - 'A' + 1;  // A=1, B=2, ..., Z=26
    }
    else if (ch >= 'a' && ch <= 'z') {
        return ch - 'a' + 1;  // Convert lowercase to uppercase
    }
    else if (ch >= '0' && ch <= '9') {
        return ch - '0' + 27; // 0=27, 1=28, ..., 9=36
    }
    else if (ch == ' ') {
        return 0;             // Space character
    }
    // Punctuation marks
    else if (ch == '.') return 37;  // Period
    else if (ch == ',') return 38;  // Comma
    else if (ch == '%') return 39;  // Percent
    else if (ch == '-') return 40;  // Hyphen/minus
    else if (ch == ':') return 41;  // Colon
    else if (ch == ';') return 42;  // Semicolon
    else if (ch == '!') return 43;  // Exclamation
    else if (ch == '?') return 44;  // Question mark
    else if (ch == '/') return 45;  // Forward slash
    else if (ch == '(') return 46;  // Left parenthesis
    else if (ch == ')') return 47;  // Right parenthesis
    else if (ch == '+') return 48;  // Plus
    else if (ch == '=') return 49;  // Equals
    else if (ch == '_') return 50;  // Underscore
    else {
        return 0;             // Unknown character = space
    }
}

/**
 * @brief Character support summary
 * 
 * Supported characters (total: 51 characters):
 * - Letters: A-Z (case insensitive)
 * - Numbers: 0-9 (with slashed zero)
 * - Punctuation: . , % - : ; ! ? / ( ) + = _
 * - Special: SPACE
 * 
 * Perfect for displaying:
 * - Battery voltages: "3.7V"
 * - Percentages: "85%"  
 * - Status messages: "Battery: OK"
 * - Time formats: "12:34"
 * - Mathematical expressions: "V = 3.7V (85%)"
 */

#endif // SSD1306_FONT_H