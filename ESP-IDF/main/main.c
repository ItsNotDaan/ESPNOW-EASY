#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "espnow_easy.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "ESP-NOW Easy - Default Application");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "To use this project:");
    ESP_LOGI(TAG, "1. For MASTER: Use examples/master or examples/multi_slave");
    ESP_LOGI(TAG, "2. For SLAVE: Use examples/slave");
    ESP_LOGI(TAG, "3. For LED examples: Use examples/master_led or examples/slave_led");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "To change the main component:");
    ESP_LOGI(TAG, "  Edit CMakeLists.txt and change EXTRA_COMPONENT_DIRS");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "This is just a default placeholder.");
    ESP_LOGI(TAG, "========================================");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
