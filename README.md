# Raspberry Pi Pico Peripheral Libraries

A collection of professional C libraries for common peripherals used with the Raspberry Pi Pico. Each library is designed for easy integration with the Pico SDK and includes proper CMake configuration.

## Libraries Included

| Library | Description | Interface | Status |
|---------|-------------|-----------|---------|
| **ads1115** | 16-bit ADC with programmable gain | I2C | Basic Functionality |
| **ina219** | High-side current/power monitor | I2C | Basic Functionality |
| **battery** | Battery voltage measurement utilities | Analog | Basic Functionality |
| **ds3231** | Real-time clock with temperature sensor | I2C | Basic Functionality |
| **ds3231_modded** | DS3231 RTC with extended features | I2C | Modded/Extended |
| **ssd1306** | 128x64 OLED display with enhanced fonts | I2C | Basic Functionality |
| **sh1106** | 128x64 OLED display with font support | I2C | Basic Functionality |
| **sdcard** | SD card hardware configuration | SPI | Config only |

## Quick Start

### Adding to Your Project as Submodule

```bash
# In your Pico project directory
git submodule add https://github.com/mcanyucel/rpi-pico-libraries.git libs

# Initialize and update submodule
git submodule update --init --recursive
```

### CMake Integration

In your project's `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)

project(my_pico_project)
pico_sdk_init()

# Add only the libraries you need

add_subdirectory(libs/ssd1306)
add_subdirectory(libs/sh1106)
add_subdirectory(libs/ds3231)
add_subdirectory(libs/ds3231_modded)
add_subdirectory(libs/ads1115)
add_subdirectory(libs/ina219)
add_subdirectory(libs/battery)

add_executable(my_project src/main.c)

target_link_libraries(my_project
    ssd1306
    sh1106
    ds3231
    ds3231_modded
    ads1115
    ina219
    battery
    pico_stdlib
)

pico_add_extra_outputs(my_project)
```

### Basic Usage Example

```c
#include "ssd1306_i2c.h"
#include "ds3231.h"
#include "ads1115.h"

#include "ina219_i2c.h"
#include "battery.h"
#include "ds3231_modded.h"

int main() {
    stdio_init_all();
    
    // Initialize displays and sensors
    SSD1306_init();
    ds3231_init();
    ads1115_init();
        ina219_init();
        battery_init();
        ds3231_modded_init();
    
    // Use the libraries...
    
    return 0;
}
```

## Library Details

### SSD1306 OLED Display

**Features:**
- 128x64 pixel resolution
- Enhanced 8x8 bitmap font with punctuation
- Flexible text positioning (any Y coordinate)
- Buffer-based rendering for smooth updates
- I2C interface (400kHz)

**Hardware Connections:**
- **I2C Port:** i2c0
- **SDA:** GPIO 16
- **SCL:** GPIO 17
- **Address:** 0x3C

**Example:**
```c
#include "ssd1306_i2c.h"

// Create display buffer
uint8_t display_buf[SSD1306_BUF_LEN];

// Initialize and use
SSD1306_init();
SSD1306_CLEAR_BUFFER(display_buf);
WriteString(display_buf, 0, 0, "Hello World!");

struct render_area area = SSD1306_FULL_SCREEN_AREA();
calc_render_area_buflen(&area);
render(display_buf, &area);
```

### DS3231 Real-Time Clock

**Features:**
- High-precision RTC with temperature compensation
- Battery backup support
- Temperature sensor (±3°C accuracy)
- Alarm functionality
- I2C interface

**Hardware Connections:**
- **I2C Port:** i2c1
- **SDA:** GPIO 18
- **SCL:** GPIO 19
- **INT/SQW:** GPIO 5 (optional)
- **Address:** 0x68

**Example:**
```c
#include "ds3231.h"

ds3231_datetime_t datetime;
float temperature;

// Initialize
ds3231_init();

// Set time
datetime.date = (ds3231_date_t){.year = 25, .month = 9, .day = 8, .weekday = 1};
datetime.time = (ds3231_time_t){.hours = 15, .minutes = 30, .seconds = 0};
ds3231_set_datetime(&datetime);

// Read time and temperature
ds3231_read_datetime(&datetime);
ds3231_read_temperature(&temperature);
```

### ADS1115 16-bit ADC

**Features:**
- 16-bit resolution
- Programmable gain amplifier
- 4 single-ended or 2 differential inputs
- I2C interface
- High precision analog readings

**Hardware Connections:**
- **I2C Port:** i2c0 (shared with SSD1306)
- **SDA:** GPIO 4
- **SCL:** GPIO 5
- **Address:** 0x48 (default)

**Example:**
```c
#include "ads1115.h"

// Initialize
ads1115_init();

// Read analog value
int16_t raw_value = ads1115_read_channel(0);
float voltage = ads1115_raw_to_voltage(raw_value);
```

### SH1106 OLED Display

**Features:**
- 128x64 pixel resolution
- Similar to SSD1306 with enhanced driver
- Font support included
- I2C interface

**Hardware Connections:**
- **I2C Port:** i2c0
- **SDA:** GPIO 16
- **SCL:** GPIO 17
- **Address:** 0x3C

## Development Workflow

### Working with Submodules

```bash
# Update libraries to latest version
cd your-project/libs
git pull origin main
cd ..
git add libs
git commit -m "Update libraries to latest version"

# Make changes to libraries during development
cd your-project/libs
# Edit library files as needed
git add .
git commit -m "Fix sensor timing issue"
git push origin main

# Update parent project to use new library version
cd ..
git add libs
git commit -m "Update libs with timing fix"
```

### Testing Your Changes

The repository includes a complete test project that demonstrates:
- Dual I2C setup (two different I2C ports)
- Real-time clock display on OLED
- Temperature monitoring
- Proper error handling

## Requirements

- **Raspberry Pi Pico** or compatible RP2040 board
- **Pico SDK** (latest version recommended)
- **CMake** 3.13 or higher
- **GCC ARM** compiler

## Project Structure

```
rpi-pico-libraries/
├── ads1115/
│   ├── CMakeLists.txt
│   ├── ads1115.c
│   └── include/ads1115.h
├── ina219/
│   ├── CMakeLists.txt
│   ├── ina219_i2c.c
│   └── include/ina219_i2c.h
├── battery/
│   ├── CMakeLists.txt
│   ├── battery.c
│   └── include/battery.h
├── ds3231/
│   ├── CMakeLists.txt
│   ├── ds3231.c
│   └── include/ds3231.h
├── ds3231_modded/
│   ├── CMakeLists.txt
│   ├── ds3231_modded.c
│   └── include/ds3231_modded.h
├── ssd1306/
│   ├── CMakeLists.txt
│   ├── ssd1306_i2c.c
│   └── include/
│       ├── ssd1306_i2c.h
│       └── ssd1306_font.h
├── sh1106/
│   ├── CMakeLists.txt
│   ├── sh1106_i2c.c
│   └── include/
│       ├── sh1106_i2c.h
│       └── sh1106_font.h
├── sdcard/
│   ├── CMakeLists.txt
│   ├── hw_config.c
│   └── include/hw_config.h
└── README.md
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Test your changes thoroughly
4. Submit a pull request

## License

This project is licensed under the MIT License - see individual library files for specific license information.

## Related Projects

- [Test Project Example](https://github.com/yourusername/pico-rtc-display-test) - Complete example using multiple libraries
- [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk) - Official Pico SDK

## Performance Notes

- **SSD1306 & SH1106:** Optimized for 400kHz I2C operation
- **DS3231:** Stable at 100kHz I2C for reliable timekeeping
- **Font Rendering:** Efficient 8x8 bitmap fonts with minimal memory usage
- **Multi-device:** Libraries designed to work together without conflicts

---
*Happy Coding with Raspberry Pi Pico!*