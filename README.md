# PickMod_Product_Area_Tracker

Product area tracking system for Waveshare ESP32-S3 Touch LCD 7" (800×480 RGB panel).

## 🔧 Quick Fixes

**Just got a build error after changing menuconfig?** → See [BUILD_ERROR_FIX_SUMMARY.md](BUILD_ERROR_FIX_SUMMARY.md)

**Common Issues:**
- Build error: `invalid use of incomplete typedef 'lv_event_t'` → [Component version fix](BUILD_ERROR_FIX_SUMMARY.md)
- Font error: `'lv_font_montserrat_24' undeclared` → [Font configuration](#font-configuration-issues)
- Watchdog timeout: LVGL task frozen → [Watchdog fix](WATCHDOG_FIX.md)
- Memory error: `ESP_ERR_NO_MEM` → [Memory allocation](#memory-allocation-failure-esp_err_no_mem)

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

### Verify Configuration (Optional but Recommended)

Before building, you can run the configuration checker to verify your setup:

```bash
# Make the script executable (first time only)
chmod +x check_config.sh

# Run the configuration checker
./check_config.sh
```

This will verify:
- sdkconfig.defaults is present and has required settings
- PSRAM configuration is correct
- Flash size is configured properly
- Montserrat 24 font is enabled
- No stale sdkconfig files are present
- ESP-IDF environment is activated

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

#### Watchdog Timeout and Blank Screen

If you encounter:
```
E (xxx) lcd_panel.io: esp_lcd_panel_io_register_event_callbacks(40): invalid panel io handle
E (xxxx) task_wdt: Task watchdog got triggered. The following tasks/users did not reset the watchdog in time:
E (xxxx) task_wdt:  - IDLE0 (CPU 0)
E (xxxx) task_wdt: Tasks currently running:
E (xxxx) task_wdt: CPU 0: taskLVGL
```

With a blank screen and the LVGL task stuck in `wait_for_flushing`.

**Root Cause:** The `esp_lvgl_port` library tries to register event callbacks on a NULL `io_handle` for RGB panels, which causes the LVGL flush operation to never complete.

**Solution:** This has been fixed in the latest code. The fix:
1. Uses manual LVGL display driver registration instead of `lvgl_port_add_disp()`
2. Provides a custom flush callback that works with RGB panels
3. Properly signals LVGL when flush operations complete

To apply the fix:
```bash
# Pull latest changes
git pull

# Clean build
rm -f sdkconfig sdkconfig.old
idf.py set-target esp32s3
idf.py build

# Flash
idf.py -p /dev/ttyUSB0 flash monitor
```

For detailed technical information about this fix, see [WATCHDOG_FIX.md](WATCHDOG_FIX.md).

**Memory Consumption Note:** LVGL demos are explicitly disabled. The ~900KB memory usage is normal for an 800×480 RGB565 display with double-buffered rendering.

#### Component Version Conflict (LVGL v8/v9 Incompatibility)

If you encounter a build error after changing menuconfig settings (e.g., unchecking LVGL demos):

```
error: invalid use of incomplete typedef 'lv_event_t' {aka 'struct _lv_event_t'}
  347 |     lvgl_port_display_ctx_t *disp_ctx = (lvgl_port_display_ctx_t *)e->user_data;
      |                                                                     ^~
```

**Root Cause:** The ESP-IDF component manager resolved to a version of `esp_lvgl_port` that includes LVGL v9 code paths, but the code has an API incompatibility with LVGL v9.

**Solution:** This has been fixed by pinning exact component versions. To apply:

```bash
# Pull latest changes
git pull

# Clean managed components and build cache
rm -rf managed_components/
rm -rf build/

# Reconfigure (downloads components with new version constraints)
idf.py set-target esp32s3

# Build
idf.py build
```

The fix ensures:
- `lvgl/lvgl` is pinned to v8.4.x (explicitly prevents v9)
- `esp_lvgl_port` is pinned to exactly v2.1.0 (prevents version drift)

For detailed technical information, see [COMPONENT_VERSION_FIX.md](COMPONENT_VERSION_FIX.md).

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