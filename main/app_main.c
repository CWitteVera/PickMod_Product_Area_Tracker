/**
 * @file app_main.c
 * @brief Main application entry point for PickMod Product Area Tracker
 * 
 * Milestone 1: Panel-Only Validation
 * - Initialize RGB LCD panel (800x480)
 * - Draw test patterns (color bars)
 * - Run for 60+ seconds to validate stability
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "hal/display.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "PickMod Product Area Tracker");
    ESP_LOGI(TAG, "Milestone 1: Panel-Only Validation");
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "Board: Waveshare ESP32-S3 Touch LCD 7\"");
    ESP_LOGI(TAG, "Display: 800x480 RGB panel");
    ESP_LOGI(TAG, "ESP-IDF: %s", esp_get_idf_version());
    ESP_LOGI(TAG, "=================================================");

    /* Initialize display */
    esp_err_t ret = display_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize display: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "Display initialized successfully");
    
    /* Test sequence: cycle through different patterns */
    int cycle = 0;
    while (1) {
        cycle++;
        ESP_LOGI(TAG, "Test cycle %d - Drawing test patterns...", cycle);
        
        /* Pattern 1: Solid RED */
        ESP_LOGI(TAG, "  Pattern: Solid RED");
        display_fill(COLOR_RED);
        vTaskDelay(pdMS_TO_TICKS(2000));
        
        /* Pattern 2: Solid GREEN */
        ESP_LOGI(TAG, "  Pattern: Solid GREEN");
        display_fill(COLOR_GREEN);
        vTaskDelay(pdMS_TO_TICKS(2000));
        
        /* Pattern 3: Solid BLUE */
        ESP_LOGI(TAG, "  Pattern: Solid BLUE");
        display_fill(COLOR_BLUE);
        vTaskDelay(pdMS_TO_TICKS(2000));
        
        /* Pattern 4: Color bars */
        ESP_LOGI(TAG, "  Pattern: Color bars (8 vertical)");
        display_draw_test_bars();
        vTaskDelay(pdMS_TO_TICKS(4000));
        
        /* Pattern 5: WHITE */
        ESP_LOGI(TAG, "  Pattern: Solid WHITE");
        display_fill(COLOR_WHITE);
        vTaskDelay(pdMS_TO_TICKS(2000));
        
        /* Pattern 6: BLACK */
        ESP_LOGI(TAG, "  Pattern: Solid BLACK");
        display_fill(COLOR_BLACK);
        vTaskDelay(pdMS_TO_TICKS(2000));
        
        ESP_LOGI(TAG, "Cycle %d complete (%.1f seconds elapsed)", 
                 cycle, (float)(cycle * 14));
        ESP_LOGI(TAG, "Check for: no flicker, no tearing, no garbled frames");
    }
}
