# DS3231 Hardware Modifications for True 0.8mA Dormant Mode

See [the original post](https://ghubcoder.github.io/posts/waking-the-pico-external-trigger/).

## The Problem
If you power the DS3231 from VCC during sleep, it draws:
- **1.8mA with LED** ❌ (worse than not using dormant mode!)
- **0.36mA without LED** ❌ (still adds to the 0.8mA dormant current)

## The Solution
Power the DS3231 ONLY from its CR2032 battery, not from Pico's 3.3V. This way:
- **DS3231 draws 0mA from main battery** ✅
- **Only draws ~80µA from CR2032 during I2C communication** ✅
- **CR2032 lasts months or years** ✅

## Required Hardware Modifications

### What You Need to Remove

### **Modification 1: Remove Resistor Pack (REQUIRED)**
- **Location**: Marked as "1" - the 4.7kΩ pull-up resistor pack (usually marked "472")
- **Why**: These resistors pull SQW/SCL/SDA to VCC. When VCC is disconnected, they become pull-downs, breaking I2C and the alarm interrupt
- **How**: 
  - Desolder with soldering iron, OR
  - Carefully break off with tweezers/pencil tip
- **Result**: I2C and SQW will work on battery power

### **Modification 2: Remove Charging Resistor (RECOMMENDED)**
- **Location**: Marked as "2" - the 200Ω resistor near the battery holder
- **Why**: This resistor trickle-charges the battery when VCC is connected (dangerous for non-rechargeable CR2032!)
- **How**: Desolder or break off
- **Result**: Safe to use non-rechargeable CR2032

### **Modification 3: Remove Power LED (OPTIONAL but RECOMMENDED)**
- **Location**: The LED near the VCC pin
- **Why**: Saves additional power if you accidentally connect VCC
- **How**: Desolder or break off
- **Result**: Lower power consumption

## Final Wiring After Modifications

```
DS3231 Module    ->    Raspberry Pi Pico
-----------------------------------------
VCC              ->    NOT CONNECTED! (or GND)
GND              ->    GND
SDA              ->    GP0 (with software pull-up)
SCL              ->    GP1 (with software pull-up)
INT/SQW          ->    GP3 (with software pull-up)
Battery          ->    CR2032 installed
```

## Software Requirements After Modification

Since we removed the hardware pull-up resistors, we MUST enable software pull-ups:

```python
# In your initialization code:
from machine import Pin, I2C

# Configure I2C pins with pull-ups
sda = Pin(0, Pin.OUT, Pin.PULL_UP)
scl = Pin(1, Pin.OUT, Pin.PULL_UP)
i2c = I2C(0, sda=sda, scl=scl)

# Configure wake pin with pull-up (CRITICAL!)
wake_pin = Pin(3, Pin.IN, Pin.PULL_UP)

# Initialize DS3231
from lib.ds3231_utils import DS3231
rtc = DS3231(sda_pin=0, scl_pin=1)
```

## Testing the Modifications

### Step 1: Verify I2C Communication
```python
# With battery installed and VCC disconnected:
i2c = I2C(0, sda=Pin(0, Pin.PULL_UP), scl=Pin(1, Pin.PULL_UP))
devices = i2c.scan()
print(devices)  # Should show [104] (0x68 in decimal)
```

### Step 2: Test Alarm Function
```python
# Set an alarm and check INT/SQW pin
rtc.set_alarm1(1)  # 1 minute alarm
wake_pin = Pin(3, Pin.IN, Pin.PULL_UP)
print(wake_pin.value())  # Should be 1 (HIGH)
# After 1 minute:
print(wake_pin.value())  # Should be 0 (LOW) when alarm triggers
```

## Power Consumption Results

### Before Modifications (VCC Connected):
- Active: ~26mA
- Dormant + DS3231 on VCC: **1.8mA** (with LED) or **0.36mA** (without LED)

### After Modifications (Battery Only):
- Active: ~26mA  
- Dormant + DS3231 on battery: **0.8mA total** ✅
- DS3231 draws nothing from main supply ✅

## CR2032 Battery Life Calculation

DS3231 on battery draws:
- **Standby**: ~3µA (maintaining time)
- **During I2C**: ~80µA (brief pulses)

For 15-minute wake cycles:
- CR2032 capacity: ~220mAh
- Average current: ~5µA
- **Battery life: 5+ years** ✅

## Troubleshooting

### I2C Not Working After Modification
- **Check**: Pull-up resistors removed completely?
- **Fix**: Enable software pull-ups on SDA/SCL pins

### INT/SQW Always LOW
- **Check**: Resistor pack "1" fully removed?
- **Fix**: Ensure complete removal, enable software pull-up on wake pin

### Alarm Not Triggering
- **Check**: CR2032 battery voltage (should be >2.8V)
- **Fix**: Replace battery, verify alarm configuration

## Summary

**Minimal Modifications Required:**
1. ✅ Remove resistor pack "1" (4.7kΩ pull-ups)
2. ✅ Install CR2032 battery
3. ✅ Don't connect VCC (or connect to GND)
4. ✅ Enable software pull-ups in code

**Optional but Recommended:**
- Remove resistor "2" (charging resistor) for safety
- Remove LED to save power if VCC accidentally connected

**Result:** True 0.8mA dormant mode with reliable DS3231 wake-up!