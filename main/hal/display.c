/**
 * @file display.c
 * @brief Display HAL implementation for Waveshare ESP32-S3 Touch LCD 7"
 * 
 * RGB Panel Configuration:
 * - Resolution: 800x480
 * - Interface: RGB888 (24-bit parallel)
 * - Timing: Cloned from Espressif ESP32-S3 LCD EV-Board LVGL demo
 * 
 * LVGL Integration:
 * - esp_lvgl_port for LVGL v9
 * - Direct-mode rendering (no bounce buffer)
 * - Anti-tearing enabled via CONFIG_LCD_RGB_RESTART_IN_VSYNC
 * - Single LVGL task with mutex discipline
 * 
 * Pin mapping for Waveshare ESP32-S3 Touch LCD 7":
 * - Data pins: GPIO 15,7,6,5,4,9,46,3,8,16,1,14,21,47,48 (R0-4, G0-5, B0-4)
 * - HSYNC: GPIO 46
 * - VSYNC: GPIO 3  
 * - PCLK: GPIO 8
 * - DE (Data Enable): GPIO 5
 * - DISP (Backlight): GPIO via CH422G EXIO2 (controlled separately)
 */

#include "display.h"
#include "esp_log.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "esp_lvgl_port.h"
#include "esp_heap_caps.h"
#include "lvgl.h"
#include <string.h>

static const char *TAG = "display";

/** LCD panel handle */
static esp_lcd_panel_handle_t panel_handle = NULL;

/** Framebuffer pointer (allocated in PSRAM) */
static uint16_t *framebuffer = NULL;

/** LVGL display object */
static lv_disp_t *lvgl_disp = NULL;

/** LVGL draw buffers (allocated in PSRAM) */
static lv_color_t *lvgl_buf1 = NULL;
static lv_color_t *lvgl_buf2 = NULL;

/** LVGL display driver structures - removed in LVGL v9 */
/* lv_disp_draw_buf_t and lv_disp_drv_t are replaced by lv_display_t */

/* Pin definitions for Waveshare ESP32-S3 Touch LCD 7" */
#define PIN_LCD_PCLK        (8)
#define PIN_LCD_HSYNC       (46)
#define PIN_LCD_VSYNC       (3)
#define PIN_LCD_DE          (5)

/* RGB data pins */
#define PIN_LCD_R0          (1)
#define PIN_LCD_R1          (2)
#define PIN_LCD_R2          (42)
#define PIN_LCD_R3          (41)
#define PIN_LCD_R4          (40)
#define PIN_LCD_G0          (39)
#define PIN_LCD_G1          (38)
#define PIN_LCD_G2          (45)
#define PIN_LCD_G3          (48)
#define PIN_LCD_G4          (47)
#define PIN_LCD_G5          (21)
#define PIN_LCD_B0          (14)
#define PIN_LCD_B1          (13)
#define PIN_LCD_B2          (12)
#define PIN_LCD_B3          (11)
#define PIN_LCD_B4          (10)

/* RGB timing parameters - based on typical 800x480 panel and EV-Board demo */
#define LCD_PIXEL_CLOCK_HZ  (16 * 1000 * 1000)  // 16MHz pixel clock
#define LCD_H_RES           DISPLAY_WIDTH
#define LCD_V_RES           DISPLAY_HEIGHT
#define LCD_HSYNC_BACK_PORCH    (40)
#define LCD_HSYNC_FRONT_PORCH   (40)
#define LCD_HSYNC_PULSE_WIDTH   (48)
#define LCD_VSYNC_BACK_PORCH    (13)
#define LCD_VSYNC_FRONT_PORCH   (13)
#define LCD_VSYNC_PULSE_WIDTH   (3)

esp_err_t display_init(void)
{
    esp_err_t ret = ESP_OK;

    ESP_LOGI(TAG, "Initializing RGB LCD panel (800x480)");
    ESP_LOGI(TAG, "Configuring for avoid lcd tearing effect");

    /* Configure RGB panel */
    esp_lcd_rgb_panel_config_t panel_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .timings = {
            .pclk_hz = LCD_PIXEL_CLOCK_HZ,
            .h_res = LCD_H_RES,
            .v_res = LCD_V_RES,
            .hsync_pulse_width = LCD_HSYNC_PULSE_WIDTH,
            .hsync_back_porch = LCD_HSYNC_BACK_PORCH,
            .hsync_front_porch = LCD_HSYNC_FRONT_PORCH,
            .vsync_pulse_width = LCD_VSYNC_PULSE_WIDTH,
            .vsync_back_porch = LCD_VSYNC_BACK_PORCH,
            .vsync_front_porch = LCD_VSYNC_FRONT_PORCH,
            .flags = {
                .pclk_active_neg = false,
                .pclk_idle_high = false,
            },
        },
        .data_width = 16,  // RGB565
        .bits_per_pixel = 16,
        .num_fbs = 1,      // Single framebuffer initially
        .bounce_buffer_size_px = 0,
        .psram_trans_align = 64,
        .hsync_gpio_num = PIN_LCD_HSYNC,
        .vsync_gpio_num = PIN_LCD_VSYNC,
        .de_gpio_num = PIN_LCD_DE,
        .pclk_gpio_num = PIN_LCD_PCLK,
        .disp_gpio_num = GPIO_NUM_NC,  // Controlled by CH422G EXIO2
        .data_gpio_nums = {
            PIN_LCD_B0, PIN_LCD_B1, PIN_LCD_B2, PIN_LCD_B3, PIN_LCD_B4,
            PIN_LCD_G0, PIN_LCD_G1, PIN_LCD_G2, PIN_LCD_G3, PIN_LCD_G4, PIN_LCD_G5,
            PIN_LCD_R0, PIN_LCD_R1, PIN_LCD_R2, PIN_LCD_R3, PIN_LCD_R4,
        },
        .flags = {
            .fb_in_psram = true,  // Store framebuffer in PSRAM
            .refresh_on_demand = false,
        },
    };

    /* Create RGB panel */
    ret = esp_lcd_new_rgb_panel(&panel_config, &panel_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create RGB panel: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Reset and initialize panel */
    ret = esp_lcd_panel_reset(panel_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to reset panel: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_lcd_panel_init(panel_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init panel: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Get framebuffer pointer */
    esp_lcd_rgb_panel_get_frame_buffer(panel_handle, 1, (void **)&framebuffer);
    if (framebuffer == NULL) {
        ESP_LOGE(TAG, "Failed to get framebuffer pointer");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "RGB LCD panel initialized successfully");
    ESP_LOGI(TAG, "Framebuffer @ %p (in PSRAM)", framebuffer);
    
    return ESP_OK;
}

esp_err_t display_fill(uint16_t color)
{
    if (framebuffer == NULL) {
        ESP_LOGE(TAG, "Display not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    size_t pixel_count = DISPLAY_WIDTH * DISPLAY_HEIGHT;
    for (size_t i = 0; i < pixel_count; i++) {
        framebuffer[i] = color;
    }

    return ESP_OK;
}

esp_err_t display_draw_test_bars(void)
{
    if (framebuffer == NULL) {
        ESP_LOGE(TAG, "Display not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    /* Draw 8 vertical color bars */
    const uint16_t colors[] = {
        COLOR_WHITE,
        COLOR_YELLOW,
        COLOR_CYAN,
        COLOR_GREEN,
        COLOR_MAGENTA,
        COLOR_RED,
        COLOR_BLUE,
        COLOR_BLACK
    };
    
    const int bar_width = DISPLAY_WIDTH / 8;
    
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            int bar_index = x / bar_width;
            if (bar_index >= 8) bar_index = 7;
            
            framebuffer[y * DISPLAY_WIDTH + x] = colors[bar_index];
        }
    }

    ESP_LOGI(TAG, "Drew test color bars");
    return ESP_OK;
}

esp_lcd_panel_handle_t display_get_panel_handle(void)
{
    return panel_handle;
}

/* Custom flush callback for RGB panel direct-mode rendering - LVGL v9 API */
static void rgb_lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_handle_t panel = (esp_lcd_panel_handle_t)lv_display_get_user_data(disp);
    
    /* Draw the bitmap to RGB framebuffer */
    int x1 = area->x1;
    int y1 = area->y1;
    int x2 = area->x2;
    int y2 = area->y2;
    
    esp_lcd_panel_draw_bitmap(panel, x1, y1, x2 + 1, y2 + 1, px_map);
    
    /* Notify LVGL that flushing is done */
    lv_display_flush_ready(disp);
}

esp_err_t display_lvgl_init(void)
{
    esp_err_t ret = ESP_OK;

    if (panel_handle == NULL) {
        ESP_LOGE(TAG, "Display not initialized. Call display_init() first.");
        return ESP_ERR_INVALID_STATE;
    }

    /* Prevent double initialization */
    if (lvgl_disp != NULL) {
        ESP_LOGW(TAG, "LVGL already initialized, skipping");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing LVGL v9 with esp_lvgl_port");
    ESP_LOGI(TAG, "Configuration: direct-mode, avoid lcd tearing effect");

    /* Initialize LVGL core */
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,
        .task_stack = 4096,
        .task_affinity = -1,        // No affinity
        .task_max_sleep_ms = 500,
        .timer_period_ms = 5
    };
    
    ret = lvgl_port_init(&lvgl_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize LVGL port: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "LVGL port initialized (single task, mutex enabled)");

    /* Configure LVGL display manually for RGB panels using LVGL v9 API */
    /* Allocate draw buffers in PSRAM */
    size_t buffer_size = DISPLAY_WIDTH * 50;  // 50 lines
    lvgl_buf1 = heap_caps_malloc(buffer_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    lvgl_buf2 = heap_caps_malloc(buffer_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    
    if (lvgl_buf1 == NULL || lvgl_buf2 == NULL) {
        ESP_LOGE(TAG, "Failed to allocate LVGL draw buffers");
        if (lvgl_buf1) heap_caps_free(lvgl_buf1);
        if (lvgl_buf2) heap_caps_free(lvgl_buf2);
        lvgl_buf1 = NULL;
        lvgl_buf2 = NULL;
        /* Note: lvgl_port remains initialized. In production, this is acceptable
         * as the application will not continue after display init failure. */
        return ESP_ERR_NO_MEM;
    }
    
    ESP_LOGI(TAG, "Allocated draw buffers: %d pixels x 2 (in PSRAM)", buffer_size);
    
    /* Create display using LVGL v9 API */
    lvgl_disp = lv_display_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    if (lvgl_disp == NULL) {
        ESP_LOGE(TAG, "Failed to create LVGL display");
        heap_caps_free(lvgl_buf1);
        heap_caps_free(lvgl_buf2);
        lvgl_buf1 = NULL;
        lvgl_buf2 = NULL;
        return ESP_FAIL;
    }
    
    /* Set up display buffers and callbacks */
    lv_display_set_flush_cb(lvgl_disp, rgb_lvgl_flush_cb);
    lv_display_set_buffers(lvgl_disp, lvgl_buf1, lvgl_buf2, buffer_size * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_user_data(lvgl_disp, panel_handle);

    ESP_LOGI(TAG, "LVGL display registered (direct-mode with custom flush)");
    ESP_LOGI(TAG, "Buffer size: %d pixels (double buffered)", buffer_size);
    ESP_LOGI(TAG, "LVGL initialization complete");

    return ESP_OK;
}

bool display_lvgl_lock(int timeout_ms)
{
    return lvgl_port_lock(timeout_ms);
}

void display_lvgl_unlock(void)
{
    lvgl_port_unlock();
}
