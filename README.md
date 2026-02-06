# PickMod_Product_Area_Tracker

Product area tracking system for Waveshare ESP32-S3 Touch LCD 7" (800×480 RGB panel).

## Hardware

- **Board**: Waveshare ESP32-S3 Touch LCD 7"
- **Display**: 800×480 RGB panel (RGB565, 16-bit parallel)
- **Touch**: GT911 capacitive touch controller (via I²C)
- **I/O Expander**: CH422G @ 0x24 (controls GT911 reset via EXIO1)
- **Toolchain**: ESP-IDF 5.2.0
- **Graphics**: LVGL v8

## Project Status

Current milestone: **Milestone 2 - LVGL v8 Minimal Label**

### Milestones
- [x] **Milestone 1**: Panel-only RGB bring-up with test patterns (no LVGL, no touch) ✅
- [ ] **Milestone 2**: LVGL v8 minimal label (no touch) - **IN TESTING**
- [ ] **Milestone 3**: Touch bring-up with CH422G reset sequence
- [ ] **Milestone 4**: EEZ Studio UI integration (Home/Setup screens)

## Build Instructions

### Prerequisites

1. Install ESP-IDF 5.2.0:
   ```bash
   # Follow official ESP-IDF installation guide
   # https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32s3/get-started/
   ```

2. Set up ESP-IDF environment:
   ```bash
   . $HOME/esp/esp-idf/export.sh
   # Or wherever you installed ESP-IDF
   ```

### Build and Flash

```bash
# Configure project (uses sdkconfig.defaults)
idf.py set-target esp32s3

# Build firmware
idf.py build

# Flash to board (replace PORT with your serial port)
idf.py -p /dev/ttyUSB0 flash

# Monitor serial output
idf.py -p /dev/ttyUSB0 monitor

# Or combine flash + monitor
idf.py -p /dev/ttyUSB0 flash monitor
```

### Expected Output (Milestone 2)

The display should show a centered label with:
- Text: "PickMod Product Area Tracker / Milestone 2: LVGL v8 Test / 800x480 RGB Direct-Mode"
- Dark blue-gray background (#2E3440)
- White text (#ECEFF4)
- Montserrat 24pt font

**Expected serial logs:**
```
I (xxx) display: Initializing RGB LCD panel (800x480)
I (xxx) display: Configuring for avoid lcd tearing effect
I (xxx) display: RGB LCD panel initialized successfully
I (xxx) display: Framebuffer @ 0x3fc00000 (in PSRAM)
I (xxx) display: Initializing LVGL v8 with esp_lvgl_port
I (xxx) display: Configuration: direct-mode, avoid lcd tearing effect
I (xxx) display: LVGL port initialized (single task, mutex enabled)
I (xxx) display: LVGL display registered (direct-mode rendering)
I (xxx) display: Buffer size: 40000 pixels (double buffered)
I (xxx) display: LVGL initialization complete
I (xxx) main: LVGL initialized successfully
I (xxx) main: Test label created and centered
I (xxx) main: Status check: 10 seconds elapsed
I (xxx) main: Status check: 60 seconds elapsed
I (xxx) main: 60 seconds completed - Milestone 2 stability test PASSED
```

**Validation criteria:**
- Label remains perfectly stable (no "jumping" text)
- No flicker or tearing
- Text clearly readable and properly centered
- Stable for ≥60 seconds
- Logs show "direct-mode", "avoid lcd tearing", and "single task, mutex enabled"

## Configuration

Key settings in `sdkconfig.defaults`:
- CPU frequency: 240 MHz
- PSRAM: Enabled (OCT mode, 80 MHz)
- Compiler optimization: Performance mode
- LCD RGB ISR: IRAM-safe
- LCD timing: Based on Espressif EV-Board LVGL demo
- LVGL Fonts: Montserrat 14 and 24 enabled

### Troubleshooting

#### Font Configuration Issues

If you encounter the error `'lv_font_montserrat_24' undeclared`, the LVGL font configuration needs to be regenerated. This happens when the `sdkconfig` file doesn't match `sdkconfig.defaults`.

**Solution:**

```bash
# Option 1: Delete sdkconfig and reconfigure (recommended)
rm -f sdkconfig sdkconfig.old
idf.py set-target esp32s3
idf.py build

# Option 2: Force reconfiguration
idf.py reconfigure
idf.py build

# Option 3: Full clean build
idf.py fullclean
idf.py set-target esp32s3
idf.py build
```

The `sdkconfig.defaults` file now explicitly lists all Montserrat font sizes, enabling sizes 14 and 24 while disabling others. This ensures the LVGL library is compiled with the required fonts.

#### Memory Allocation Failure (ESP_ERR_NO_MEM)

If you encounter errors like:
```
E (xxx) lcd_panel.rgb: lcd_rgb_panel_alloc_frame_buffers(xxx): no mem for frame buffer
E (xxx) lcd_panel.rgb: esp_lcd_new_rgb_panel(xxx): alloc frame buffers failed
E (xxx) display: Failed to create RGB panel: ESP_ERR_NO_MEM
```

And you also see a flash size mismatch warning:
```
W (xxx) spi_flash: Detected size(16384k) larger than the size in the binary image header(2048k)
```

**Root Cause:** You have a stale or manually-edited `sdkconfig` file that is missing critical PSRAM and flash configurations from `sdkconfig.defaults`.

**Solution:**

```bash
# Delete the stale sdkconfig file
rm -f sdkconfig sdkconfig.old

# Regenerate from sdkconfig.defaults
idf.py set-target esp32s3

# Build with correct configuration
idf.py build
```

**Important:** Never manually edit the `sdkconfig` file directly. Instead:
- Make changes in `sdkconfig.defaults` (for permanent project defaults)
- Use `idf.py menuconfig` (for temporary local changes)
- Always delete `sdkconfig` after changing `sdkconfig.defaults` to force regeneration

## Project Structure

```
main/
├── app_main.c              # Application entry point
└── hal/
    ├── display.c/.h        # RGB LCD panel driver (esp_lcd)
    └── touch.c/.h          # Touch & CH422G (Milestone 3+)
app/
└── zone_model.c/.h         # Zone tracking logic (Milestone 4+)
net/
├── data_adapter.h          # Data adapter interface (Milestone 4+)
└── data_adapter_stub.c     # Stub implementation (Milestone 4+)
ui/
└── ui_init.c/.h            # EEZ UI integration (Milestone 4+)
```

## References

- [Waveshare ESP32-S3 Touch LCD 7" Wiki](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-7)
- [ESP-IDF v5.2 Programming Guide](https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32s3/)
- [Espressif ESP32-S3 LCD EV-Board Demo](https://github.com/espressif/esp-dev-kits/tree/master/esp32-s3-lcd-ev-board)
- [LVGL v8 Documentation](https://docs.lvgl.io/8.3/)

## License

See repository license.