#ifndef LED_STRIP_RMT_H
#define LED_STRIP_RMT_H

#include <stdint.h>
#include "driver/rmt_tx.h"

#ifdef __cplusplus
extern "C" {
#endif

// RGB color structure
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb_t;

// LED strip handle
typedef struct {
    rmt_channel_handle_t channel;
    rmt_encoder_handle_t encoder;
    uint16_t num_leds;
    rgb_t *leds;
} led_strip_t;

/**
 * @brief Initialize LED strip with RMT
 * 
 * @param gpio_num GPIO number for LED data pin
 * @param num_leds Number of LEDs in the strip
 * @return led_strip_t* Pointer to LED strip handle, or NULL on error
 */
led_strip_t* led_strip_init(uint32_t gpio_num, uint16_t num_leds);

/**
 * @brief Set RGB color for a specific LED
 * 
 * @param strip LED strip handle
 * @param index LED index (0-based)
 * @param r Red value (0-255)
 * @param g Green value (0-255)
 * @param b Blue value (0-255)
 */
void led_strip_set_pixel(led_strip_t *strip, uint16_t index, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Set HSV color for a specific LED
 * 
 * @param strip LED strip handle
 * @param index LED index (0-based)
 * @param h Hue value (0-359)
 * @param s Saturation value (0-100)
 * @param v Value/brightness (0-100)
 */
void led_strip_set_pixel_hsv(led_strip_t *strip, uint16_t index, uint16_t h, uint8_t s, uint8_t v);

/**
 * @brief Clear all LEDs (set to black)
 * 
 * @param strip LED strip handle
 */
void led_strip_clear(led_strip_t *strip);

/**
 * @brief Update the LED strip (send data to LEDs)
 * 
 * @param strip LED strip handle
 * @return true if successful, false otherwise
 */
bool led_strip_refresh(led_strip_t *strip);

/**
 * @brief Free LED strip resources
 * 
 * @param strip LED strip handle
 */
void led_strip_free(led_strip_t *strip);

#ifdef __cplusplus
}
#endif

#endif // LED_STRIP_RMT_H
