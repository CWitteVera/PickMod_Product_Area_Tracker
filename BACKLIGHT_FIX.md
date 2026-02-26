# Backlight Fix - Blank Screen Issue Resolved

## Problem
The display was remaining blank even though the RGB panel initialization and LVGL were working correctly. The terminal output showed successful initialization, but nothing was visible on screen.

## Root Cause
The display backlight (DISP pin) was not being turned on. On the Waveshare ESP32-S3 Touch LCD 7", the backlight is controlled via a CH422G I2C I/O expander at address 0x24, specifically the EXIO2 pin. There was no code to:
1. Initialize the I2C master
2. Communicate with the CH422G
3. Turn on the EXIO2 pin (backlight)

## Solution
Added minimal backlight control code to `main/hal/display.c`:

### Changes Made:
1. **Added I2C driver support**
   - Included `driver/i2c.h`
   - Configured I2C master on I2C_NUM_0 (SDA=GPIO17, SCL=GPIO18)

2. **Added CH422G definitions**
   - I2C address: 0x24
   - Output register: 0x02
   - EXIO bit mapping:
     - EXIO1 (bit 0): TP_RST (touch reset)
     - EXIO2 (bit 1): DISP (backlight) ← **This was the missing piece**
     - EXIO3 (bit 2): LCD_RST (LCD reset)

3. **Implemented helper functions**
   - `i2c_master_init()`: Initializes I2C master
   - `ch422g_write_output()`: Writes to CH422G output register
   - `backlight_init()`: Turns on EXIO2 to enable backlight

4. **Updated display_init()**
   - Calls `i2c_master_init()` first
   - Calls `backlight_init()` to turn on backlight
   - Then proceeds with RGB panel initialization

## How to Build and Test

### Build the firmware:
```bash
# Set target (if not already done)
idf.py set-target esp32s3

# Build firmware
idf.py build

# Flash to board
idf.py -p /dev/ttyUSB0 flash monitor
```

### Expected Output:
You should now see these new log messages before the RGB panel initialization:
```
I (xxx) display: I2C master initialized (SDA=17, SCL=18)
I (xxx) display: Initializing backlight via CH422G EXIO2
I (xxx) display: Backlight enabled (EXIO2=1)
I (xxx) display: Initializing RGB LCD panel (800x480)
```

### Visual Verification:
The display should now show:
- Dark blue-gray background (#2E3440)
- Centered white text with three lines:
  - "PickMod Product Area Tracker"
  - "Milestone 2: LVGL v8 Test"
  - "800x480 RGB Direct-Mode"

## Technical Details

### I2C Configuration:
- **SDA Pin**: GPIO 17
- **SCL Pin**: GPIO 18
- **Frequency**: 100 kHz (standard I2C speed)
- **Pullups**: Enabled (may be redundant with hardware pullups)

### CH422G Communication:
The CH422G is controlled by writing to register 0x02 (output register):
- Each bit controls one EXIO pin (0=low, 1=high)
- Current implementation sets all three EXIOs high for stability:
  - EXIO1=1 (touch reset inactive)
  - EXIO2=1 (backlight ON) ← **Critical for display**
  - EXIO3=1 (LCD reset inactive)

### Backlight Control:
The backlight is turned on by setting bit 1 (EXIO2) in the CH422G output register to 1. This control is separate from the ESP32-S3's RGB interface and must be done via I2C.

## Future Enhancements
While this fix enables the backlight, future improvements could include:
1. **PWM backlight dimming**: Pulse EXIO2 for brightness control
2. **Power management**: Turn off backlight during sleep
3. **Backlight timeout**: Auto-dim after inactivity

However, these are beyond the scope of this minimal fix which just needed to turn the backlight on.

## References
- Waveshare ESP32-S3 Touch LCD 7" documentation
- CH422G I2C I/O Expander datasheet
- ESP-IDF I2C Driver API documentation
- Project's `.copilot-instructions.md` hardware mapping section
