#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

#define LED_PIN         GPIO_NUM_16
#define BUTTON_PIN      GPIO_NUM_10
#define LED_TIMEOUT_MS  10000
#define LONG_PRESS_MS   2000
#define DEBOUNCE_MS     50

static const char *TAG = "LED_CTRL";

static volatile bool    button_isr_flag = false;
static volatile int64_t press_time_us   = 0;
static bool             led_state       = false;

static TimerHandle_t led_timer = NULL;

static void led_timer_callback(TimerHandle_t xTimer)
{
    led_state = false;
    gpio_set_level(LED_PIN, 0);
    ESP_LOGI(TAG, "Timer expirado -> LED apagado");
}

static void IRAM_ATTR button_isr_handler(void *arg)
{
    press_time_us   = esp_timer_get_time();
    button_isr_flag = true;
}

static void button_task(void *arg)
{
    int64_t last_event_us  = 0;
    int64_t press_start_us = 0;
    bool    pressing       = false;

    while (1) {
        if (!button_isr_flag) {
            if (pressing) {
                int64_t held_ms = (esp_timer_get_time() - press_start_us) / 1000LL;

                if (held_ms >= LONG_PRESS_MS) {
                    ESP_LOGI(TAG, "Pressionamento longo -> LED apagado");
                    xTimerStop(led_timer, 0);
                    led_state = false;
                    gpio_set_level(LED_PIN, 0);
                    pressing = false;

                    while (gpio_get_level(BUTTON_PIN) == 0) {
                        vTaskDelay(pdMS_TO_TICKS(10));
                    }
                }
            }
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        button_isr_flag = false;
        int64_t now_us = press_time_us;

        if ((now_us - last_event_us) < (DEBOUNCE_MS * 1000LL)) {
            continue;
        }
        last_event_us = now_us;

        int level = gpio_get_level(BUTTON_PIN);

        if (level == 0) {
            ESP_LOGI(TAG, "Botao pressionado");
            press_start_us = esp_timer_get_time();
            pressing       = true;
        } else {
            if (!pressing) {
                continue;
            }

            int64_t held_ms = (esp_timer_get_time() - press_start_us) / 1000LL;
            pressing = false;
            ESP_LOGI(TAG, "Duracao do clique: %lld ms", held_ms);

            if (held_ms >= LONG_PRESS_MS) {
                /* já tratado no polling acima, ignora */
            } else {
                if (!led_state) {
                    ESP_LOGI(TAG, "Primeiro clique -> LED ligado (10 s)");
                    led_state = true;
                    gpio_set_level(LED_PIN, 1);
                    xTimerStart(led_timer, 0);
                } else {
                    ESP_LOGI(TAG, "Clique subsequente -> timer reiniciado (10 s)");
                    xTimerReset(led_timer, 0);
                }
            }
        }
    }
}

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
        .intr_type    = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&btn_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, NULL);

    led_timer = xTimerCreate(
        "led_timer",
        pdMS_TO_TICKS(LED_TIMEOUT_MS),
        pdFALSE,
        NULL,
        led_timer_callback
    );

    xTaskCreate(button_task, "button_task", 4096, NULL, 10, NULL);

    ESP_LOGI(TAG, "Sistema iniciado. Aguardando eventos...");
}