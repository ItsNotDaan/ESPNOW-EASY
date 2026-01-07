#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "espnow_easy.h"

static const char *TAG = "SLAVE";

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
    ESP_LOGI(TAG, "ESP-NOW Slave Example");
    ESP_LOGI(TAG, "========================================");

    // Initialize ESP-NOW
    if (!espnow_easy_init(SLAVE, DEBUG_ON)) {
        ESP_LOGE(TAG, "ESP-NOW initialization failed!");
        esp_restart();
    }

    ESP_LOGI(TAG, "Slave initialized successfully");
    ESP_LOGI(TAG, "Waiting for master to pair...");

    espnow_easy_set_print_received(true);

    uint64_t last_status_print = 0;
    uint64_t last_response = 0;
    uint8_t response_counter = 0;

    while (1) {
        // Check pairing status
        espnow_easy_check_pairing_status(5000);

        espnow_status_t status = espnow_easy_get_status();

        uint64_t current_time = esp_timer_get_time();

        // Print status every 10 seconds
        if (current_time - last_status_print > 10000000) {
            ESP_LOGI(TAG, "--- Slave Status ---");
            ESP_LOGI(TAG, "Paired to Master: %s", status.isPaired ? "Yes" : "No");
            ESP_LOGI(TAG, "Pairing Active: %s", status.isPairingActive ? "Yes" : "No");
            ESP_LOGI(TAG, "-------------------");
            
            last_status_print = current_time;
        }

        // Send response to master every 8 seconds if paired
        if (current_time - last_response > 8000000 && status.isPaired) {
            char message[32];
            snprintf(message, sizeof(message), "Response #%d", response_counter++);
            
            ESP_LOGI(TAG, ">>> Sending to Master: %s", message);
            espnow_easy_broadcast(DATA, message, response_counter);
            
            last_response = current_time;
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // Small delay to prevent tight loop
    }
}
