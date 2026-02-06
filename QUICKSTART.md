# Quick Verification Guide - Milestones 1 & 2

## How to Build and Flash

```bash
# Set target (first time only)
idf.py set-target esp32s3

# Build
idf.py build

# Flash and monitor
idf.py -p /dev/ttyUSB0 flash monitor
```

---

## Milestone 1: Panel-Only - Quick Check

### What to Look For
✅ Display cycles through 6 test patterns:
1. Solid RED (2s)
2. Solid GREEN (2s)
3. Solid BLUE (2s)
4. Color bars - 8 vertical stripes (4s)
5. Solid WHITE (2s)
6. Solid BLACK (2s)

✅ Repeats every 14 seconds

### Expected Log Snippet
```
I (xxx) display: Initializing RGB LCD panel (800x480)
I (xxx) display: Configuring for avoid lcd tearing effect
I (xxx) display: RGB LCD panel initialized successfully
I (xxx) display: Framebuffer @ 0x3fc00000 (in PSRAM)
I (xxx) main: Display initialized successfully
I (xxx) main: Test cycle 1 - Drawing test patterns...
I (xxx) main:   Pattern: Solid RED
```

### Pass/Fail Criteria
- ✅ PASS: Clean patterns, no flicker, stable ≥60s
- ❌ FAIL: Garbled display, tearing, crashes, reset loops

---

## Milestone 2: LVGL Label - Quick Check

### What to Look For
✅ Display shows centered label:
- 3 lines of white text
- Dark blue-gray background
- Text: "PickMod Product Area Tracker", "Milestone 2: LVGL v8 Test", "800x480 RGB Direct-Mode"

✅ Status check every 10 seconds

✅ "60 seconds completed" message at 60s mark

### Expected Log Snippet
```
I (xxx) display: Initializing LVGL v8 with esp_lvgl_port
I (xxx) display: Configuration: direct-mode, avoid lcd tearing effect
I (xxx) display: LVGL port initialized (single task, mutex enabled)
I (xxx) display: LVGL display registered (direct-mode rendering)
I (xxx) display: Buffer size: 40000 pixels (double buffered)
I (xxx) main: LVGL initialized successfully
I (xxx) main: Test label created and centered
I (xxx) main: Status check: 10 seconds elapsed
I (xxx) main: Status check: 60 seconds elapsed
I (xxx) main: 60 seconds completed - Milestone 2 stability test PASSED
```

### Pass/Fail Criteria
- ✅ PASS: Label centered, text stable (no jumping), no flicker, stable ≥60s, correct logs
- ❌ FAIL: Text jumps/jitters, tearing, crashes, missing "direct-mode" or "avoid lcd tearing" logs

---

## Key Log Messages to Verify

Must be present in serial output:

### Critical Messages (Milestone 1 & 2)
1. `"Configuring for avoid lcd tearing effect"` - Confirms anti-tearing config
2. `"Framebuffer @ 0x3fc00000 (in PSRAM)"` - Confirms PSRAM usage (address may vary)

### Critical Messages (Milestone 2 only)
3. `"direct-mode, avoid lcd tearing effect"` - Confirms LVGL direct-mode
4. `"single task, mutex enabled"` - Confirms single-task architecture
5. `"direct-mode rendering"` - Confirms no bounce buffer
6. `"double buffered"` - Confirms double buffering active

---

## Common Issues

### Build fails: 'lv_font_montserrat_24' undeclared

**Cause:** The LVGL font configuration in `sdkconfig` doesn't match `sdkconfig.defaults`.

**Solution:**
```bash
# Delete old config and rebuild
rm -f sdkconfig sdkconfig.old
idf.py set-target esp32s3
idf.py build
```

### Memory allocation failure (ESP_ERR_NO_MEM)

**Symptoms:**
- Error: `lcd_rgb_panel_alloc_frame_buffers: no mem for frame buffer`
- Error: `Failed to create RGB panel: ESP_ERR_NO_MEM`
- Warning: Flash size mismatch (e.g., `Detected size(16384k) larger than size in binary image header(2048k)`)
- Screen stays black

**Cause:** Stale or manually-edited `sdkconfig` file missing critical PSRAM/flash configurations.

**Solution:**
```bash
# Delete stale config
rm -f sdkconfig sdkconfig.old

# Regenerate from sdkconfig.defaults
idf.py set-target esp32s3

# Build with correct configuration
idf.py build
```

**Prevention:** Never manually edit `sdkconfig`. Use `sdkconfig.defaults` or `idf.py menuconfig` instead.

### Build fails
- Check ESP-IDF version: `idf.py --version` (should be 5.2.x)
- Try: `idf.py fullclean && idf.py build`

### Flash fails
- Check port: `ls /dev/tty*` (Linux) or Device Manager (Windows)
- Try holding BOOT button during flash

### Display blank/garbled
- Check power supply (panel needs adequate current)
- Verify boot logs show successful init
- Check ESP-IDF version matches 5.2.0
- If you see ESP_ERR_NO_MEM errors, see "Memory allocation failure" above

### "Jumping text" (Milestone 2)
- This is a known issue if GT911 touch is initialized without proper reset
- Should NOT occur in Milestone 2 (touch not enabled yet)
- If it occurs, document in detail - may indicate timing issue

---

## Success Criteria Summary

| Milestone | Duration | Visual | Logs | Ready for Next? |
|-----------|----------|--------|------|-----------------|
| M1: Panel | ≥60s | Clean patterns, no artifacts | "avoid lcd tearing" | → M2 |
| M2: LVGL  | ≥60s | Stable label, no jumping | "direct-mode", "single task, mutex" | → M3 (touch) |

---

## Next Milestone (M3) - NOT IMPLEMENTED YET

Will add:
- Touch initialization with CH422G reset sequence
- GT911 driver integration
- Touch coordinate logging
- LVGL indev registration

⚠️ **DO NOT PROCEED** to Milestone 3 until hardware validation confirms M1 and M2 stable for ≥60 seconds.

---

## Quick Reference: Port Names by OS

- **Linux**: `/dev/ttyUSB0`, `/dev/ttyACM0`
- **macOS**: `/dev/cu.usbserial-*`, `/dev/tty.usbserial-*`
- **Windows**: `COM3`, `COM4`, etc.

Use `idf.py -p <PORT>` to specify port.

---

## ESP-IDF Monitor Commands

While monitor is running:
- `Ctrl+]` - Exit monitor
- `Ctrl+T` then `Ctrl+R` - Reset board
- `Ctrl+T` then `Ctrl+H` - Show help

---

## Reporting Results

When reporting test results, include:
1. Which milestone tested (M1 or M2)
2. Duration observed (e.g., "120 seconds")
3. Visual behavior (stable/unstable/artifacts)
4. Copy of relevant logs (at least init sequence and first 60s)
5. Any errors or warnings seen
6. Board power supply voltage/current if available

Format:
```
Milestone X Test Results:
- Duration: XXX seconds
- Visual: [describe what you saw]
- Stability: PASS/FAIL
- Logs: [paste relevant sections]
- Issues: [any problems observed]
```
