#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

#define LED_PIN        GPIO_NUM_16
#define BUTTON_PIN     GPIO_NUM_10
#define DEBOUNCE_MS    50
#define TIMEOUT_MS     10000

static const char *TAG = "LED_CTRL";

void app_main(void)
{
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_conf);
    gpio_set_level(LED_PIN, 0);

    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&btn_conf);

    bool    led_state      = false;
    bool    last_button    = true;
    int64_t led_on_time_us = 0;

    ESP_LOGI(TAG, "Sistema iniciado.");

    while (1) {
        bool current_button = (bool) gpio_get_level(BUTTON_PIN);

        if (!current_button && last_button) {
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));

            if (gpio_get_level(BUTTON_PIN) == 0) {
                led_state = !led_state;
                gpio_set_level(LED_PIN, led_state ? 1 : 0);

                if (led_state) {
                    led_on_time_us = esp_timer_get_time();
                    ESP_LOGI(TAG, "LED ligado");
                } else {
                    ESP_LOGI(TAG, "LED apagado");
                }
            }
        }

        last_button = current_button;

        if (led_state) {
            int64_t elapsed_ms = (esp_timer_get_time() - led_on_time_us) / 1000LL;
            if (elapsed_ms >= TIMEOUT_MS) {
                led_state = false;
                gpio_set_level(LED_PIN, 0);
                ESP_LOGI(TAG, "Timeout -> LED apagado automaticamente");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}