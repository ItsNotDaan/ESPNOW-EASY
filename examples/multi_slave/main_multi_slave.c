#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "espnow_easy.h"

static const char *TAG = "MULTI_SLAVE";

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
    ESP_LOGI(TAG, "ESP-NOW Multi-Slave Example");
    ESP_LOGI(TAG, "========================================");

    // Initialize ESP-NOW
    if (!espnow_easy_init(MASTER, DEBUG_ON)) {
        ESP_LOGE(TAG, "ESP-NOW initialization failed!");
        esp_restart();
    }

    ESP_LOGI(TAG, "ESP-NOW initialized successfully");
    ESP_LOGI(TAG, "Ready to pair slaves...");

    // Start pairing first slave
    espnow_easy_start_pairing();

    espnow_easy_set_print_received(true);

    uint32_t target_slaves = 3;
    uint64_t last_status_print = 0;
    uint64_t last_broadcast = 0;
    uint64_t last_targeted = 0;
    uint8_t broadcast_counter = 0;

    while (1) {
        // Check pairing mode status
        espnow_easy_check_pairing_status(5000);

        espnow_status_t status = espnow_easy_get_status();

        // Auto-pair up to target_slaves for demonstration
        if (status.slavesCount < target_slaves && !status.isPairingActive) {
            vTaskDelay(pdMS_TO_TICKS(2000)); // Short delay between pairings
            ESP_LOGI(TAG, "Pairing next slave (#%d)...", status.slavesCount);
            espnow_easy_start_pairing();
        }

        uint64_t current_time = esp_timer_get_time();

        // Print detailed slave status every 15 seconds
        if (current_time - last_status_print > 15000000) {
            espnow_easy_print_slaves_status();
            last_status_print = current_time;
        }

        // Broadcast message to all slaves every 5 seconds
        if (current_time - last_broadcast > 5000000 && status.slavesCount > 0) {
            char message[32];
            snprintf(message, sizeof(message), "Broadcast #%d", broadcast_counter++);
            
            espnow_easy_broadcast(DATA, message, broadcast_counter);
            ESP_LOGI(TAG, ">>> Broadcasted message #%d", broadcast_counter);
            
            last_broadcast = current_time;
        }

        // Send targeted message to specific slave every 7 seconds
        if (current_time - last_targeted > 7000000 && status.slavesCount > 0) {
            // Send to slave 0
            espnow_easy_send_to_slave(0, DATA, "Private msg", 42);
            ESP_LOGI(TAG, ">>> Sent private message to Slave #0");
            
            last_targeted = current_time;
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // Small delay to prevent tight loop
    }
}
