/**
 * @file app_main.c
 * @brief Main application entry point for PickMod Product Area Tracker
 *
 * Milestone 2: LVGL v8 Demo Widgets
 * - Initialize RGB LCD panel (800x480)
 * - Initialize LVGL v8 with esp_lvgl_port (direct-mode)
 * - Run the built-in LVGL widgets demo (lv_demo_widgets)
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "hal/display.h"
#include "lvgl.h"
#include "demos/widgets/lv_demo_widgets.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "PickMod Product Area Tracker");
    ESP_LOGI(TAG, "Milestone 2: LVGL v8 Demo Widgets");
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "Board: Waveshare ESP32-S3 Touch LCD 7\"");
    ESP_LOGI(TAG, "Display: 800x480 RGB panel");
    ESP_LOGI(TAG, "Graphics: LVGL v8 (esp_lvgl_port) - lv_demo_widgets");
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

    /* Launch LVGL widgets demo */
    if (!display_lvgl_lock(1000)) {
        ESP_LOGE(TAG, "Failed to lock LVGL");
        return;
    }
    lv_demo_widgets();
    display_lvgl_unlock();
    ESP_LOGI(TAG, "LVGL widgets demo started");

    /* Keep the application running */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        ESP_LOGI(TAG, "Running - LVGL widgets demo active");
    }
}
