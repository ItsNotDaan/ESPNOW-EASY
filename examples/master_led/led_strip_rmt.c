#include "led_strip_rmt.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "esp_log.h"
#include "driver/rmt_tx.h"

static const char *TAG = "LED_STRIP";

// WS2812B timing (in nanoseconds)
#define WS2812_T0H_NS 350
#define WS2812_T0L_NS 900
#define WS2812_T1H_NS 900
#define WS2812_T1L_NS 350
#define WS2812_RESET_NS 50000

// RMT resolution
#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz, 100ns per tick

// LED strip encoder
typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t *bytes_encoder;
    rmt_encoder_t *copy_encoder;
    int state;
    rmt_symbol_word_t reset_code;
} rmt_led_strip_encoder_t;

static size_t rmt_encode_led_strip(rmt_encoder_t *encoder, rmt_channel_handle_t channel,
                                   const void *primary_data, size_t data_size,
                                   rmt_encode_state_t *ret_state)
{
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_encoder_handle_t bytes_encoder = led_encoder->bytes_encoder;
    rmt_encoder_handle_t copy_encoder = led_encoder->copy_encoder;
    rmt_encode_state_t session_state = RMT_ENCODING_RESET;
    rmt_encode_state_t state = RMT_ENCODING_RESET;
    size_t encoded_symbols = 0;

    switch (led_encoder->state) {
    case 0: // send RGB data
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, primary_data, data_size, &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            led_encoder->state = 1; // switch to next state when current encoding session finished
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state |= RMT_ENCODING_MEM_FULL;
            goto out; // yield if there's no free space to put other encoding artifacts
        }
    // fall-through
    case 1: // send reset code
        encoded_symbols += copy_encoder->encode(copy_encoder, channel, &led_encoder->reset_code,
                                                sizeof(led_encoder->reset_code), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            led_encoder->state = RMT_ENCODING_RESET; // back to the initial encoding session
            state |= RMT_ENCODING_COMPLETE;
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state |= RMT_ENCODING_MEM_FULL;
            goto out;
        }
    }
out:
    *ret_state = state;
    return encoded_symbols;
}

static esp_err_t rmt_del_led_strip_encoder(rmt_encoder_t *encoder)
{
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_del_encoder(led_encoder->bytes_encoder);
    rmt_del_encoder(led_encoder->copy_encoder);
    free(led_encoder);
    return ESP_OK;
}

static esp_err_t rmt_led_strip_encoder_reset(rmt_encoder_t *encoder)
{
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_encoder_reset(led_encoder->bytes_encoder);
    rmt_encoder_reset(led_encoder->copy_encoder);
    led_encoder->state = RMT_ENCODING_RESET;
    return ESP_OK;
}

static esp_err_t rmt_new_led_strip_encoder(rmt_encoder_handle_t *ret_encoder)
{
    esp_err_t ret = ESP_OK;
    rmt_led_strip_encoder_t *led_encoder = NULL;
    led_encoder = calloc(1, sizeof(rmt_led_strip_encoder_t));
    if (!led_encoder) {
        return ESP_ERR_NO_MEM;
    }
    led_encoder->base.encode = rmt_encode_led_strip;
    led_encoder->base.del = rmt_del_led_strip_encoder;
    led_encoder->base.reset = rmt_led_strip_encoder_reset;

    // Create bytes encoder for RGB data
    rmt_bytes_encoder_config_t bytes_encoder_config = {
        .bit0 = {
            .level0 = 1,
            .duration0 = WS2812_T0H_NS / 100,
            .level1 = 0,
            .duration1 = WS2812_T0L_NS / 100,
        },
        .bit1 = {
            .level0 = 1,
            .duration0 = WS2812_T1H_NS / 100,
            .level1 = 0,
            .duration1 = WS2812_T1L_NS / 100,
        },
        .flags.msb_first = 1,
    };
    ret = rmt_new_bytes_encoder(&bytes_encoder_config, &led_encoder->bytes_encoder);
    if (ret != ESP_OK) {
        goto err;
    }

    // Create copy encoder for reset code
    rmt_copy_encoder_config_t copy_encoder_config = {};
    ret = rmt_new_copy_encoder(&copy_encoder_config, &led_encoder->copy_encoder);
    if (ret != ESP_OK) {
        goto err;
    }

    // Create reset code
    uint32_t reset_ticks = RMT_LED_STRIP_RESOLUTION_HZ / 1000000 * WS2812_RESET_NS / 1000;
    led_encoder->reset_code = (rmt_symbol_word_t) {
        .level0 = 0,
        .duration0 = reset_ticks,
        .level1 = 0,
        .duration1 = reset_ticks,
    };

    *ret_encoder = &led_encoder->base;
    return ESP_OK;

err:
    if (led_encoder) {
        if (led_encoder->bytes_encoder) {
            rmt_del_encoder(led_encoder->bytes_encoder);
        }
        if (led_encoder->copy_encoder) {
            rmt_del_encoder(led_encoder->copy_encoder);
        }
        free(led_encoder);
    }
    return ret;
}

// HSV to RGB conversion
static void hsv_to_rgb(uint16_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b)
{
    h = h % 360;
    float sat = s / 100.0f;
    float val = v / 100.0f;

    float c = val * sat;
    float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
    float m = val - c;

    float r_prime, g_prime, b_prime;

    if (h >= 0 && h < 60) {
        r_prime = c; g_prime = x; b_prime = 0;
    } else if (h >= 60 && h < 120) {
        r_prime = x; g_prime = c; b_prime = 0;
    } else if (h >= 120 && h < 180) {
        r_prime = 0; g_prime = c; b_prime = x;
    } else if (h >= 180 && h < 240) {
        r_prime = 0; g_prime = x; b_prime = c;
    } else if (h >= 240 && h < 300) {
        r_prime = x; g_prime = 0; b_prime = c;
    } else {
        r_prime = c; g_prime = 0; b_prime = x;
    }

    *r = (uint8_t)((r_prime + m) * 255);
    *g = (uint8_t)((g_prime + m) * 255);
    *b = (uint8_t)((b_prime + m) * 255);
}

led_strip_t* led_strip_init(uint32_t gpio_num, uint16_t num_leds)
{
    led_strip_t *strip = calloc(1, sizeof(led_strip_t));
    if (!strip) {
        ESP_LOGE(TAG, "Failed to allocate LED strip");
        return NULL;
    }

    strip->num_leds = num_leds;
    strip->leds = calloc(num_leds, sizeof(rgb_t));
    if (!strip->leds) {
        ESP_LOGE(TAG, "Failed to allocate LED buffer");
        free(strip);
        return NULL;
    }

    // Configure RMT TX channel
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = gpio_num,
        .mem_block_symbols = 64,
        .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
        .trans_queue_depth = 4,
        .flags.invert_out = false,
        .flags.with_dma = false,
    };

    esp_err_t ret = rmt_new_tx_channel(&tx_chan_config, &strip->channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create RMT TX channel: %s", esp_err_to_name(ret));
        free(strip->leds);
        free(strip);
        return NULL;
    }

    // Create LED strip encoder
    ret = rmt_new_led_strip_encoder(&strip->encoder);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create LED strip encoder: %s", esp_err_to_name(ret));
        rmt_del_channel(strip->channel);
        free(strip->leds);
        free(strip);
        return NULL;
    }

    // Enable RMT channel
    ret = rmt_enable(strip->channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable RMT channel: %s", esp_err_to_name(ret));
        rmt_del_encoder(strip->encoder);
        rmt_del_channel(strip->channel);
        free(strip->leds);
        free(strip);
        return NULL;
    }

    ESP_LOGI(TAG, "LED strip initialized: %d LEDs on GPIO %lu", num_leds, gpio_num);
    return strip;
}

void led_strip_set_pixel(led_strip_t *strip, uint16_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if (!strip || index >= strip->num_leds) {
        return;
    }

    strip->leds[index].r = r;
    strip->leds[index].g = g;
    strip->leds[index].b = b;
}

void led_strip_set_pixel_hsv(led_strip_t *strip, uint16_t index, uint16_t h, uint8_t s, uint8_t v)
{
    if (!strip || index >= strip->num_leds) {
        return;
    }

    uint8_t r, g, b;
    hsv_to_rgb(h, s, v);
    led_strip_set_pixel(strip, index, r, g, b);
}

void led_strip_clear(led_strip_t *strip)
{
    if (!strip) {
        return;
    }

    memset(strip->leds, 0, strip->num_leds * sizeof(rgb_t));
}

bool led_strip_refresh(led_strip_t *strip)
{
    if (!strip) {
        return false;
    }

    // For WS2812B, the order is GRB not RGB
    uint8_t *grb_data = malloc(strip->num_leds * 3);
    if (!grb_data) {
        ESP_LOGE(TAG, "Failed to allocate GRB buffer");
        return false;
    }

    for (uint16_t i = 0; i < strip->num_leds; i++) {
        grb_data[i * 3 + 0] = strip->leds[i].g;
        grb_data[i * 3 + 1] = strip->leds[i].r;
        grb_data[i * 3 + 2] = strip->leds[i].b;
    }

    rmt_transmit_config_t tx_config = {
        .loop_count = 0,
    };

    esp_err_t ret = rmt_transmit(strip->channel, strip->encoder, grb_data, 
                                 strip->num_leds * 3, &tx_config);
    
    free(grb_data);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to transmit LED data: %s", esp_err_to_name(ret));
        return false;
    }

    // Wait for transmission to complete
    ret = rmt_tx_wait_all_done(strip->channel, 100);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to wait for transmission: %s", esp_err_to_name(ret));
        return false;
    }

    return true;
}

void led_strip_free(led_strip_t *strip)
{
    if (!strip) {
        return;
    }

    if (strip->channel) {
        rmt_disable(strip->channel);
        rmt_del_channel(strip->channel);
    }

    if (strip->encoder) {
        rmt_del_encoder(strip->encoder);
    }

    if (strip->leds) {
        free(strip->leds);
    }

    free(strip);
}
