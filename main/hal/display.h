/**
 * @file display.h
 * @brief Display HAL for Waveshare ESP32-S3 Touch LCD 7" (800x480 RGB panel)
 * 
 * Hardware:
 * - Panel: RGB888 interface, 800x480 resolution
 * - Timing: Based on Espressif ESP32-S3 LCD EV-Board demo
 * 
 * This module handles:
 * - RGB panel initialization with esp_lcd
 * - Framebuffer management
 * - Test pattern generation for validation
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_lcd_panel_ops.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Display resolution */
#define DISPLAY_WIDTH   800
#define DISPLAY_HEIGHT  480

/** Color definitions (RGB565) */
#define COLOR_BLACK     0x0000
#define COLOR_WHITE     0xFFFF
#define COLOR_RED       0xF800
#define COLOR_GREEN     0x07E0
#define COLOR_BLUE      0x001F
#define COLOR_YELLOW    0xFFE0
#define COLOR_CYAN      0x07FF
#define COLOR_MAGENTA   0xF81F

/**
 * @brief Initialize the RGB LCD panel
 * 
 * Configures the ESP32-S3 RGB interface for 800x480 display.
 * Uses timing parameters from Espressif EV-Board LVGL demo.
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t display_init(void);

/**
 * @brief Fill entire display with a solid color
 * 
 * @param color RGB565 color value
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t display_fill(uint16_t color);

/**
 * @brief Draw vertical color bars for testing
 * 
 * Draws 8 vertical bars with basic colors for visual validation
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t display_draw_test_bars(void);

/**
 * @brief Get the LCD panel handle
 * 
 * @return LCD panel handle or NULL if not initialized
 */
esp_lcd_panel_handle_t display_get_panel_handle(void);

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_H */
