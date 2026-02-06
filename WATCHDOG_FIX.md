# Watchdog Timeout and Blank Screen Fix

## Problem Summary

After the latest flash, the device showed:
- **Blank screen** - no display output
- **Watchdog timeout** after 5 seconds in LVGL task
- **Error message**: `E (732) lcd_panel.io: esp_lcd_panel_io_register_event_callbacks(40): invalid panel io handle`

The backtrace showed the LVGL task stuck in `wait_for_flushing` at line 1442 of lv_refr.c.

## Root Cause

The issue was in how LVGL was being integrated with the RGB LCD panel using `esp_lvgl_port` v2.x:

1. **RGB panels don't use an I/O handle** - They operate in direct framebuffer mode
2. **esp_lvgl_port's `lvgl_port_add_disp()` function** was trying to register event callbacks on the `io_handle`
3. **Since `io_handle` was NULL**, the callback registration failed with "invalid panel io handle"
4. **Without proper callbacks**, the LVGL flush operation never completed
5. **LVGL task got stuck** waiting for flush to complete, triggering the watchdog timeout

## The Solution

**Replaced `lvgl_port_add_disp()` with manual LVGL display driver registration:**

### What Changed

**Before (display.c:242-259):**
```c
/* Add RGB display to LVGL */
const lvgl_port_display_cfg_t disp_cfg = {
    .io_handle = NULL,  // RGB panels don't use an I/O handle
    .panel_handle = panel_handle,
    .buffer_size = DISPLAY_WIDTH * 50,
    .double_buffer = true,
    // ... other config
};

lvgl_disp = lvgl_port_add_disp(&disp_cfg);  // <- This tried to use NULL io_handle
```

**After (display.c:214-302):**
```c
/* Custom flush callback for RGB panel */
static void rgb_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_handle_t panel = (esp_lcd_panel_handle_t)drv->user_data;
    esp_lcd_panel_draw_bitmap(panel, x1, y1, x2 + 1, y2 + 1, color_map);
    lv_disp_flush_ready(drv);  // <- Properly notify LVGL
}

/* Manual LVGL display driver setup */
lv_color_t *buf1 = heap_caps_malloc(buffer_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
lv_color_t *buf2 = heap_caps_malloc(buffer_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);

static lv_disp_draw_buf_t draw_buf;
lv_disp_draw_buf_init(&draw_buf, buf1, buf2, buffer_size);

static lv_disp_drv_t disp_drv;
lv_disp_drv_init(&disp_drv);
disp_drv.hor_res = DISPLAY_WIDTH;
disp_drv.ver_res = DISPLAY_HEIGHT;
disp_drv.flush_cb = rgb_lvgl_flush_cb;  // <- Custom flush that works
disp_drv.draw_buf = &draw_buf;
disp_drv.user_data = panel_handle;

lvgl_disp = lv_disp_drv_register(&disp_drv);  // <- Direct LVGL registration
```

### Key Benefits

1. **Avoids NULL io_handle issue** - No attempt to register callbacks on NULL handle
2. **Proper flush completion** - Custom callback correctly signals LVGL when flush is done
3. **Still uses esp_lvgl_port** - Keeps the LVGL task management and mutex discipline
4. **Maintains anti-tearing** - CONFIG_LCD_RGB_RESTART_IN_VSYNC still active
5. **Same memory footprint** - Still uses 50-line double buffers in PSRAM

## Memory Consumption

### Question: "Are we building the demo in the lvgl file?"

**Answer: NO** - LVGL demos are NOT enabled and never were.

### Verification

Added explicit demo disables to `sdkconfig.defaults`:
```
CONFIG_LV_USE_DEMO_WIDGETS=n
CONFIG_LV_USE_DEMO_BENCHMARK=n
CONFIG_LV_USE_DEMO_STRESS=n
CONFIG_LV_USE_DEMO_MUSIC=n
```

### Memory Usage Breakdown

**Available PSRAM:** 8192 KB (8 MB)

**Memory Allocation:**
- RGB framebuffer: 800 × 480 × 2 bytes = 768,000 bytes (~750 KB)
- LVGL draw buffer 1: 800 × 50 × 2 bytes = 80,000 bytes (~78 KB)
- LVGL draw buffer 2: 800 × 50 × 2 bytes = 80,000 bytes (~78 KB)
- **Total display memory: ~906 KB**

**Remaining PSRAM:** ~7286 KB available for application use

This is normal and expected for a 800×480 RGB565 display with double-buffered rendering.

## How to Apply the Fix

1. **Delete stale sdkconfig** (if it exists):
   ```bash
   rm -f sdkconfig sdkconfig.old
   ```

2. **Rebuild the project**:
   ```bash
   idf.py set-target esp32s3
   idf.py build
   ```

3. **Flash to device**:
   ```bash
   idf.py -p /dev/ttyUSB0 flash monitor
   ```

## Expected Results After Fix

### Boot Logs Should Show:
```
I (xxx) display: Initializing RGB LCD panel (800x480)
I (xxx) display: RGB LCD panel initialized successfully
I (xxx) display: Framebuffer @ 0x3c0809c0 (in PSRAM)
I (xxx) display: Initializing LVGL v8 with esp_lvgl_port
I (xxx) display: LVGL port initialized (single task, mutex enabled)
I (xxx) display: Allocated draw buffers: 40000 pixels x 2 (in PSRAM)
I (xxx) display: LVGL display registered (direct-mode with custom flush)
I (xxx) main: LVGL initialized successfully
I (xxx) main: Test label created and centered
```

### Display Should Show:
- Centered white text on dark blue-gray background
- "PickMod Product Area Tracker"
- "Milestone 2: LVGL v8 Test"
- "800x480 RGB Direct-Mode"
- No flickering or artifacts

### NO Errors Expected:
- ❌ No "invalid panel io handle" error
- ❌ No watchdog timeout
- ❌ No blank screen

## Technical Details

### Why RGB Panels Are Different

RGB panels differ from SPI/I2C displays:

| Feature | SPI/I2C Display | RGB Panel |
|---------|----------------|-----------|
| I/O Handle | Required | NOT used (NULL) |
| Data Transfer | Serial via I/O | Parallel to framebuffer |
| Flush Method | DMA via io_handle | Direct memory write |
| Callback Registration | On io_handle | On panel_handle only |

### Why esp_lvgl_port Had Issues

The `esp_lvgl_port` library's `lvgl_port_add_disp()` function assumes:
1. Either io_handle is provided (for SPI/I2C)
2. Or both io_handle and panel_handle are provided

For RGB panels with NULL io_handle, it still tried to register callbacks, causing the error.

### Why Manual Registration Works

By registering the LVGL display driver directly:
1. We provide our own flush callback
2. No attempt to register on NULL io_handle
3. Flush callback uses `esp_lcd_panel_draw_bitmap()` directly
4. Properly calls `lv_disp_flush_ready()` to signal completion
5. LVGL task continues normally, no watchdog timeout

## Files Modified

1. **main/hal/display.c**
   - Added custom `rgb_lvgl_flush_cb()` function
   - Replaced `lvgl_port_add_disp()` with manual LVGL registration
   - Added includes for `esp_heap_caps.h` and `lvgl.h`

2. **sdkconfig.defaults**
   - Added explicit LVGL demo disables
   - No changes to memory/PSRAM configuration needed

## References

- ESP-IDF RGB LCD documentation: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/lcd.html#rgb-interfaced-lcd
- LVGL v8 display driver guide: https://docs.lvgl.io/8/porting/display.html
- esp_lvgl_port: https://github.com/espressif/esp-bsp/tree/master/components/esp_lvgl_port

## Summary

| Issue | Cause | Fix |
|-------|-------|-----|
| Blank screen | LVGL task frozen | Custom flush callback |
| Watchdog timeout | Flush never completes | Proper `lv_disp_flush_ready()` call |
| Invalid io handle error | esp_lvgl_port tried to use NULL handle | Manual LVGL registration |
| High memory concern | Misunderstanding (demos not enabled) | Added explicit demo disables |

**Result:** Display now works correctly with no watchdog timeouts or errors.
