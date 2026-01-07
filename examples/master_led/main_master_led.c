#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "espnow_easy.h"
#include "led_strip_rmt.h"

static const char *TAG = "MASTER_LED";

// LED configuration
#define LED_GPIO 5
#define NUM_LEDS 1

static led_strip_t *g_led_strip = NULL;

// LED color based on slave count using HSV gradient
static void set_led_color_for_slave_count(uint8_t slave_count)
{
    if (!g_led_strip) {
        return;
    }

    // Create a gradient effect based on slave count
    // Cycles through different colors as more slaves are added
    // Hue range: 0-359 degrees
    // 0=red, 120=green, 240=blue
    uint16_t hue = (slave_count * 36) % 360; // 36 degrees per slave, wraps at 10 slaves

    // Set HSV color (Hue, Saturation 100%, Value/Brightness 100%)
    uint8_t r, g, b;
    
    // Simple HSV to RGB conversion
    float h = hue / 60.0f;
    float c = 1.0f;
    float x = c * (1.0f - fabsf(fmodf(h, 2.0f) - 1.0f));
    
    float r1, g1, b1;
    if (h >= 0 && h < 1) {
        r1 = c; g1 = x; b1 = 0;
    } else if (h >= 1 && h < 2) {
        r1 = x; g1 = c; b1 = 0;
    } else if (h >= 2 && h < 3) {
        r1 = 0; g1 = c; b1 = x;
    } else if (h >= 3 && h < 4) {
        r1 = 0; g1 = x; b1 = c;
    } else if (h >= 4 && h < 5) {
        r1 = x; g1 = 0; b1 = c;
    } else {
        r1 = c; g1 = 0; b1 = x;
    }
    
    r = (uint8_t)(r1 * 255);
    g = (uint8_t)(g1 * 255);
    b = (uint8_t)(b1 * 255);

    led_strip_set_pixel(g_led_strip, 0, r, g, b);
    led_strip_refresh(g_led_strip);
}

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
    ESP_LOGI(TAG, "ESP-NOW Master with LED Status");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "LED Colors:");
    ESP_LOGI(TAG, "  Color gradient based on slave count");
    ESP_LOGI(TAG, "  0 slaves  = Red");
    ESP_LOGI(TAG, "  3 slaves  = Yellow/Green");
    ESP_LOGI(TAG, "  6 slaves  = Cyan");
    ESP_LOGI(TAG, "  10 slaves = Purple/Pink");
    ESP_LOGI(TAG, "========================================");

    // Initialize LED strip
    g_led_strip = led_strip_init(LED_GPIO, NUM_LEDS);
    if (!g_led_strip) {
        ESP_LOGE(TAG, "Failed to initialize LED strip!");
        esp_restart();
    }

    // Start with red (no slaves)
    set_led_color_for_slave_count(0);

    // Initialize ESP-NOW
    if (!espnow_easy_init(MASTER, DEBUG_ON)) {
        ESP_LOGE(TAG, "ESP-NOW initialization failed!");
        esp_restart();
    }

    ESP_LOGI(TAG, "Master initialized - Ready to pair");

    // Start pairing first slave
    espnow_easy_start_pairing();

    espnow_easy_set_print_received(true);

    uint32_t target_slaves = 4;
    uint8_t last_slave_count = 0;
    uint64_t last_timeout_check = 0;
    uint64_t last_status_print = 0;
    uint64_t last_broadcast = 0;
    uint8_t broadcast_counter = 0;

    while (1) {
        espnow_easy_check_pairing_status(5000);

        espnow_status_t status = espnow_easy_get_status();

        uint64_t current_time = esp_timer_get_time();

        // Update LED based on slave count
        if (status.slavesCount != last_slave_count) {
            set_led_color_for_slave_count(status.slavesCount);
            ESP_LOGI(TAG, ">>> LED updated: %d slaves paired", status.slavesCount);
            last_slave_count = status.slavesCount;
        }

        // Auto-pair up to target number of slaves
        if (status.slavesCount < target_slaves && !status.isPairingActive) {
            vTaskDelay(pdMS_TO_TICKS(2000));
            ESP_LOGI(TAG, "Pairing slave #%d", status.slavesCount);
            espnow_easy_start_pairing();
        }

        // Check for inactive slaves and remove them (30 second timeout)
        if (current_time - last_timeout_check > 15000000) { // Check every 15 seconds
            uint8_t removed = espnow_easy_remove_inactive_slaves(30000000); // 30 second timeout
            if (removed > 0) {
                ESP_LOGW(TAG, "[TIMEOUT] Removed %d inactive slave(s)", removed);
                // LED will update automatically in next loop via last_slave_count check
            }
            last_timeout_check = current_time;
        }

        // Print status every 15 seconds
        if (current_time - last_status_print > 15000000) {
            espnow_easy_print_slaves_status();
            last_status_print = current_time;
        }

        // Broadcast to all slaves every 5 seconds
        if (current_time - last_broadcast > 5000000 && status.slavesCount > 0) {
            espnow_easy_broadcast(DATA, "Heartbeat", broadcast_counter++);
            last_broadcast = current_time;
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // Small delay to prevent tight loop
    }
}
