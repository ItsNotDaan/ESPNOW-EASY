#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "espnow_easy.h"
#include "led_strip_rmt.h"

static const char *TAG = "SLAVE_LED";

// LED configuration
#define LED_GPIO 5
#define NUM_LEDS 1

static led_strip_t *g_led_strip = NULL;

// LED states
typedef enum {
    LED_UNPAIRED,   // Red - Not paired
    LED_PAIRING,    // Yellow - Pairing in progress
    LED_PAIRED      // Green - Successfully paired
} led_state_t;

static led_state_t g_current_led_state = LED_UNPAIRED;

static void set_led_state(led_state_t state)
{
    if (!g_led_strip) {
        return;
    }

    g_current_led_state = state;

    switch (state) {
    case LED_UNPAIRED:
        // Red - Not paired
        led_strip_set_pixel(g_led_strip, 0, 255, 0, 0);
        led_strip_refresh(g_led_strip);
        break;

    case LED_PAIRING:
        // Yellow - Pairing (will blink in main loop)
        led_strip_set_pixel(g_led_strip, 0, 255, 255, 0);
        led_strip_refresh(g_led_strip);
        break;

    case LED_PAIRED:
        // Green - Paired
        led_strip_set_pixel(g_led_strip, 0, 0, 255, 0);
        led_strip_refresh(g_led_strip);
        break;
    }
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
    ESP_LOGI(TAG, "ESP-NOW Slave with LED Status");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "LED States:");
    ESP_LOGI(TAG, "  Red           = Not paired");
    ESP_LOGI(TAG, "  Yellow (blink)= Pairing...");
    ESP_LOGI(TAG, "  Green         = Paired");
    ESP_LOGI(TAG, "========================================");

    // Initialize LED strip
    g_led_strip = led_strip_init(LED_GPIO, NUM_LEDS);
    if (!g_led_strip) {
        ESP_LOGE(TAG, "Failed to initialize LED strip!");
        esp_restart();
    }

    // Start with red (not paired)
    set_led_state(LED_UNPAIRED);

    // Initialize ESP-NOW
    if (!espnow_easy_init(SLAVE, DEBUG_ON)) {
        ESP_LOGE(TAG, "ESP-NOW initialization failed!");
        esp_restart();
    }

    ESP_LOGI(TAG, "Slave initialized - Waiting for master");

    espnow_easy_set_print_received(true);

    bool was_paired = false;
    bool was_pairing = false;
    uint64_t last_status_print = 0;
    uint64_t last_heartbeat = 0;
    uint64_t last_blink = 0;
    bool blink_state = false;
    uint8_t heartbeat_counter = 0;

    while (1) {
        // Check pairing status
        espnow_easy_check_pairing_status(5000);

        espnow_status_t status = espnow_easy_get_status();

        uint64_t current_time = esp_timer_get_time();

        // Update LED based on pairing status
        if (status.isPairingActive && !was_pairing) {
            set_led_state(LED_PAIRING);
            ESP_LOGI(TAG, ">>> LED: Pairing in progress (Yellow)");
            was_pairing = true;
        }
        else if (status.isPaired && !was_paired) {
            set_led_state(LED_PAIRED);
            ESP_LOGI(TAG, ">>> LED: Successfully paired! (Green)");
            was_paired = true;
            was_pairing = false;
        }
        else if (!status.isPaired && !status.isPairingActive && was_paired) {
            set_led_state(LED_UNPAIRED);
            ESP_LOGI(TAG, ">>> LED: Connection lost (Red)");
            was_paired = false;
            was_pairing = false;
        }

        // Blink LED during pairing
        if (g_current_led_state == LED_PAIRING && current_time - last_blink > 500000) {
            blink_state = !blink_state;
            if (blink_state) {
                led_strip_set_pixel(g_led_strip, 0, 255, 255, 0); // Yellow
            } else {
                led_strip_set_pixel(g_led_strip, 0, 0, 0, 0); // Black
            }
            led_strip_refresh(g_led_strip);
            last_blink = current_time;
        }

        // Print status every 10 seconds
        if (current_time - last_status_print > 10000000) {
            ESP_LOGI(TAG, "--- Slave Status ---");
            ESP_LOGI(TAG, "Paired: %s", status.isPaired ? "Yes" : "No");
            ESP_LOGI(TAG, "Pairing Active: %s", status.isPairingActive ? "Yes" : "No");
            ESP_LOGI(TAG, "-------------------");
            
            last_status_print = current_time;
        }

        // Send heartbeat to master every 8 seconds if paired
        if (current_time - last_heartbeat > 8000000 && status.isPaired) {
            espnow_easy_broadcast(DATA, "Heartbeat", heartbeat_counter++);
            last_heartbeat = current_time;
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // Small delay to prevent tight loop
    }
}
