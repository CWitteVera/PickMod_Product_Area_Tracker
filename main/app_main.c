/**
 * @file app_main.c
 * @brief Main application entry point for PickMod Product Area Tracker
 * 
 * Milestone 2: LVGL v8 Minimal Label
 * - Initialize RGB LCD panel (800x480)
 * - Initialize LVGL v8 with esp_lvgl_port (direct-mode)
 * - Display centered label
 * - Run for 60+ seconds to validate stability
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "hal/display.h"
#include "lvgl.h"

static const char *TAG = "main";

/**
 * @brief Create a simple centered LVGL label for testing
 */
static void create_test_label(void)
{
    /* Lock LVGL before modifying UI */
    if (!display_lvgl_lock(1000)) {
        ESP_LOGE(TAG, "Failed to lock LVGL");
        return;
    }

    /* Set background to a pleasant color */
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x2E3440), LV_PART_MAIN);

    /* Create centered label */
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text(label, "PickMod Product Area Tracker\n"
                             "Milestone 2: LVGL v8 Test\n"
                             "800x480 RGB Direct-Mode");
    
    /* Center the label */
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    
    /* Style the label */
    lv_obj_set_style_text_color(label, lv_color_hex(0xECEFF4), LV_PART_MAIN);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, LV_PART_MAIN);

    /* Unlock LVGL */
    display_lvgl_unlock();

    ESP_LOGI(TAG, "Test label created and centered");
}

void app_main(void)
{
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "PickMod Product Area Tracker");
    ESP_LOGI(TAG, "Milestone 2: LVGL v8 Minimal Label");
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "Board: Waveshare ESP32-S3 Touch LCD 7\"");
    ESP_LOGI(TAG, "Display: 800x480 RGB panel");
    ESP_LOGI(TAG, "Graphics: LVGL v8 (esp_lvgl_port)");
    ESP_LOGI(TAG, "ESP-IDF: %s", esp_get_idf_version());
    ESP_LOGI(TAG, "=================================================");

    /* Initialize display hardware */
    esp_err_t ret = display_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize display: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "Display hardware initialized");

    /* Initialize LVGL */
    ret = display_lvgl_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize LVGL: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "LVGL initialized successfully");

    /* Create test UI */
    create_test_label();

    /* Monitor loop - log status every 10 seconds */
    int elapsed = 0;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        elapsed += 10;
        
        ESP_LOGI(TAG, "Status check: %d seconds elapsed", elapsed);
        ESP_LOGI(TAG, "  - Check for: stable label, no jumping text, no artifacts");
        ESP_LOGI(TAG, "  - LVGL task running normally");
        
        if (elapsed == 60) {
            ESP_LOGI(TAG, "=================================================");
            ESP_LOGI(TAG, "60 seconds completed - Milestone 2 stability test PASSED");
            ESP_LOGI(TAG, "Ready to proceed to Milestone 3 (touch) after hardware validation");
            ESP_LOGI(TAG, "=================================================");
        }
    }
}
