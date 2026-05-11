
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"  
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_log.h"

/* pinos */
#define POT_ADC_CHANNEL     ADC_CHANNEL_6   /* GPIO34 */
#define POT_ADC_UNIT        ADC_UNIT_1
#define LED_GPIO            10
#define BUTTON_GPIO         15

/* ADC */
#define ADC_MAX_VALUE       4095

/* LEDC PWM */
#define LEDC_TIMER          LEDC_TIMER_0
#define LEDC_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL        LEDC_CHANNEL_0
#define LEDC_DUTY_RES       LEDC_TIMER_13_BIT
#define LEDC_FREQUENCY      5000
#define LEDC_DUTY_MAX       8191

#define PRINT_INTERVAL_MS   500

static const char *TAG = "ADC_PWM";


static void ledc_init(void)
{
    ledc_timer_config_t timer_cfg = {
        .speed_mode      = LEDC_MODE,
        .timer_num       = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz         = LEDC_FREQUENCY,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    ledc_channel_config_t channel_cfg = {
        .speed_mode = LEDC_MODE,
        .channel    = LEDC_CHANNEL,
        .timer_sel  = LEDC_TIMER,
        .intr_type  = LEDC_INTR_DISABLE,
        .gpio_num   = LED_GPIO,
        .duty       = 0,
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_cfg));
}


static void button_init(void)
{
    gpio_config_t io_cfg = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_cfg));
}


static uint32_t adc_to_duty(int raw)
{
    return (uint32_t)((raw * (uint32_t)LEDC_DUTY_MAX) / ADC_MAX_VALUE);
}


static uint32_t adc_to_mv(int raw)
{
    return (uint32_t)((raw * 3300UL) / ADC_MAX_VALUE);
}


void app_main(void)
{
    adc_oneshot_unit_handle_t adc_handle;

    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id  = POT_ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc_handle));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten    = ADC_ATTEN_DB_12,  
        .bitwidth = ADC_BITWIDTH_12, 
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, POT_ADC_CHANNEL, &chan_cfg));

    ledc_init();
    button_init();

    bool hold_mode      = false;
    bool btn_last_state = true;
    int  frozen_raw     = 0;

    TickType_t last_print = xTaskGetTickCount();

    ESP_LOGI(TAG, "Sistema iniciado. GPIO34=POT | GPIO%d=LED | GPIO%d=BTN",
             LED_GPIO, BUTTON_GPIO);

    while (1) {
        /*  Detecção de borda do botão com debounce  */
        bool btn_now = gpio_get_level(BUTTON_GPIO);

        if (btn_last_state == true && btn_now == false) {
            vTaskDelay(pdMS_TO_TICKS(20));
            if (gpio_get_level(BUTTON_GPIO) == false) {
                hold_mode = !hold_mode;
                if (hold_mode) {
                    adc_oneshot_read(adc_handle, POT_ADC_CHANNEL, &frozen_raw);
                    ESP_LOGI(TAG, ">>> MODO HOLD ativado (raw=%d)", frozen_raw);
                } else {
                    ESP_LOGI(TAG, ">>> MODO LIVE ativado");
                }
            }
        }
        btn_last_state = btn_now;

        /*  Leitura ADC  */
        int raw;
        if (hold_mode) {
            raw = frozen_raw;
        } else {
            ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, POT_ADC_CHANNEL, &raw));
        }

        /*   duty  */
        uint32_t duty = adc_to_duty(raw);
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty);
        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);

        /*  print  */
        TickType_t now = xTaskGetTickCount();
        if ((now - last_print) >= pdMS_TO_TICKS(PRINT_INTERVAL_MS)) {
            last_print = now;
            uint32_t voltage_mv = adc_to_mv(raw);
            printf("[%s]  raw=%4d  tensao=%4lu mV  duty=%4lu\n",
                   hold_mode ? "HOLD" : "LIVE",
                   raw,
                   (unsigned long)voltage_mv,
                   (unsigned long)duty);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}