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
 * - esp_lvgl_port for LVGL v8
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
#include "driver/i2c.h"
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

/** LVGL display driver structures */
static lv_disp_draw_buf_t lvgl_draw_buf;
static lv_disp_drv_t lvgl_disp_drv;

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

/* I2C configuration for CH422G I/O expander */
#define I2C_MASTER_NUM          I2C_NUM_0
#define I2C_MASTER_SDA_IO       17
#define I2C_MASTER_SCL_IO       18
#define I2C_MASTER_FREQ_HZ      100000
#define I2C_MASTER_TIMEOUT_MS   1000

/* CH422G I/O expander */
#define CH422G_I2C_ADDR         0x24
#define CH422G_REG_OUT          0x02  // Output register

/* CH422G EXIO bit mapping for Waveshare ESP32-S3 Touch LCD 7" */
#define CH422G_EXIO1_BIT        (1 << 0)  // TP_RST (touch reset)
#define CH422G_EXIO2_BIT        (1 << 1)  // DISP (backlight)
#define CH422G_EXIO3_BIT        (1 << 2)  // LCD_RST (LCD reset)

/**
 * @brief Initialize I2C master for CH422G communication
 */
static esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    esp_err_t ret = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C param config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "I2C master initialized (SDA=%d, SCL=%d)", I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
    return ESP_OK;
}

/**
 * @brief Write to CH422G output register
 * 
 * @param value Output value (bit 0=EXIO1, bit 1=EXIO2, bit 2=EXIO3)
 * @return ESP_OK on success, error code otherwise
 */
static esp_err_t ch422g_write_output(uint8_t value)
{
    uint8_t write_buf[2] = {CH422G_REG_OUT, value};
    
    esp_err_t ret = i2c_master_write_to_device(
        I2C_MASTER_NUM,
        CH422G_I2C_ADDR,
        write_buf,
        sizeof(write_buf),
        pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS)
    );
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "CH422G write failed: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

/**
 * @brief Initialize backlight via CH422G EXIO2
 * 
 * Turns on the display backlight by setting EXIO2 high on the CH422G
 * I/O expander. Must be called after I2C master initialization.
 * 
 * @return ESP_OK on success, error code otherwise
 */
static esp_err_t backlight_init(void)
{
    ESP_LOGI(TAG, "Initializing backlight via CH422G EXIO2");
    
    /* Set EXIO2 (backlight) high, keep EXIO1 and EXIO3 high for stability */
    uint8_t output_value = CH422G_EXIO1_BIT | CH422G_EXIO2_BIT | CH422G_EXIO3_BIT;
    
    esp_err_t ret = ch422g_write_output(output_value);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Backlight enabled (EXIO2=1)");
    } else {
        ESP_LOGE(TAG, "Failed to enable backlight");
    }
    
    return ret;
}

esp_err_t display_init(void)
{
    esp_err_t ret = ESP_OK;

    /* Initialize I2C master for CH422G */
    ret = i2c_master_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C master: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Initialize backlight via CH422G EXIO2 */
    ret = backlight_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to initialize backlight, display may be dark");
        /* Continue anyway - panel may still work */
    }

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

/* Custom flush callback for RGB panel direct-mode rendering */
static void rgb_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_handle_t panel = (esp_lcd_panel_handle_t)drv->user_data;
    
    /* Draw the bitmap to RGB framebuffer */
    int x1 = area->x1;
    int y1 = area->y1;
    int x2 = area->x2;
    int y2 = area->y2;
    
    esp_lcd_panel_draw_bitmap(panel, x1, y1, x2 + 1, y2 + 1, color_map);
    
    /* Notify LVGL that flushing is done */
    lv_disp_flush_ready(drv);
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

    ESP_LOGI(TAG, "Initializing LVGL v8 with esp_lvgl_port");
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

    /* Configure LVGL display manually for RGB panels to avoid io_handle NULL issue */
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
    
    /* Initialize LVGL draw buffer */
    lv_disp_draw_buf_init(&lvgl_draw_buf, lvgl_buf1, lvgl_buf2, buffer_size);
    
    /* Initialize display driver */
    lv_disp_drv_init(&lvgl_disp_drv);
    lvgl_disp_drv.hor_res = DISPLAY_WIDTH;
    lvgl_disp_drv.ver_res = DISPLAY_HEIGHT;
    lvgl_disp_drv.flush_cb = rgb_lvgl_flush_cb;
    lvgl_disp_drv.draw_buf = &lvgl_draw_buf;
    lvgl_disp_drv.user_data = panel_handle;
    
    /* Register the display */
    lvgl_disp = lv_disp_drv_register(&lvgl_disp_drv);
    if (lvgl_disp == NULL) {
        ESP_LOGE(TAG, "Failed to register LVGL display");
        heap_caps_free(lvgl_buf1);
        heap_caps_free(lvgl_buf2);
        lvgl_buf1 = NULL;
        lvgl_buf2 = NULL;
        /* Note: lvgl_port remains initialized. In production, this is acceptable
         * as the application will not continue after display init failure. */
        return ESP_FAIL;
    }

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
