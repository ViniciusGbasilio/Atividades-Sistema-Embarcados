#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"

// ================= CONFIGURAÇÕES =================
#define LED1_GPIO 2
#define LED2_GPIO 4
#define LED3_GPIO 5
#define LED4_GPIO 18
#define BUZZER_GPIO 13

#define DELAY_MS 50

#define LEDC_TIMER       LEDC_TIMER_0
#define LEDC_MODE        LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES    LEDC_TIMER_10_BIT
#define LEDC_FREQUENCY   1000

// ================= CANAIS =================
#define LED1_CHANNEL LEDC_CHANNEL_0
#define LED2_CHANNEL LEDC_CHANNEL_1
#define LED3_CHANNEL LEDC_CHANNEL_2
#define LED4_CHANNEL LEDC_CHANNEL_3
#define BUZZER_CHANNEL LEDC_CHANNEL_4

// ================= INIT PWM =================
void pwm_init() {

    // Timer
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_MODE,
        .timer_num = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz = LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);

    // LEDs
    int leds_gpio[] = {LED1_GPIO, LED2_GPIO, LED3_GPIO, LED4_GPIO};
    int leds_channel[] = {LED1_CHANNEL, LED2_CHANNEL, LED3_CHANNEL, LED4_CHANNEL};

    for (int i = 0; i < 4; i++) {
        ledc_channel_config_t ch = {
            .gpio_num = leds_gpio[i],
            .speed_mode = LEDC_MODE,
            .channel = leds_channel[i],
            .intr_type = LEDC_INTR_DISABLE,
            .timer_sel = LEDC_TIMER,
            .duty = 0,
            .hpoint = 0
        };
        ledc_channel_config(&ch);
    }

    // Buzzer
    ledc_channel_config_t buzzer = {
        .gpio_num = BUZZER_GPIO,
        .speed_mode = LEDC_MODE,
        .channel = BUZZER_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&buzzer);
}

// ================= CONTROLE LED =================
void set_led_duty(int channel, int duty) {
    ledc_set_duty(LEDC_MODE, channel, duty);
    ledc_update_duty(LEDC_MODE, channel);
}

void apagar_todos_leds() {
    set_led_duty(LED1_CHANNEL, 0);
    set_led_duty(LED2_CHANNEL, 0);
    set_led_duty(LED3_CHANNEL, 0);
    set_led_duty(LED4_CHANNEL, 0);
}

// ================= FASE 1 =================
// Fading
void fase1_fading() {
    for (int duty = 0; duty <= 1023; duty += 20) {
        for (int ch = 0; ch < 4; ch++) {
            set_led_duty(ch, duty);
        }
        vTaskDelay(pdMS_TO_TICKS(DELAY_MS));
    }

    for (int duty = 1023; duty >= 0; duty -= 20) {
        for (int ch = 0; ch < 4; ch++) {
            set_led_duty(ch, duty);
        }
        vTaskDelay(pdMS_TO_TICKS(DELAY_MS));
    }

    apagar_todos_leds();
}

// ================= FASE 2 =================


void acender_led(int channel) {
    apagar_todos_leds();
    set_led_duty(channel, 1023); // 100%
}

void fase2_fading_seq() {

    int channels[] = {LED1_CHANNEL, LED2_CHANNEL, LED3_CHANNEL, LED4_CHANNEL};
    apagar_todos_leds();
    // Ida
    for (int i = 0; i < 4; i++) {

        // Sobe
        for (int duty = 0; duty <= 1023; duty += 20) {
            set_led_duty(channels[i], duty);
            vTaskDelay(pdMS_TO_TICKS(DELAY_MS));
        }

        // Desce
        for (int duty = 1023; duty >= 0; duty -= 20) {
            set_led_duty(channels[i], duty);
            vTaskDelay(pdMS_TO_TICKS(DELAY_MS));
        }
      set_led_duty(channels[i], 0);
    }

    // Volta
    for (int i = 2; i >= 0; i--) {

        // Sobe
        for (int duty = 0; duty <= 1023; duty += 20) {
            set_led_duty(channels[i], duty);
            vTaskDelay(pdMS_TO_TICKS(DELAY_MS));
        }

        // Desce
        for (int duty = 1023; duty >= 0; duty -= 20) {
            set_led_duty(channels[i], duty);
            vTaskDelay(pdMS_TO_TICKS(DELAY_MS));
        }
      set_led_duty(channels[i], 0);
    }
}
// ================= FASE 3 =================
// Buzzer variando frequência
void fase3_buzzer() {

    // Subindo
    for (int freq = 500; freq <= 2000; freq += 50) {
        ledc_set_freq(LEDC_MODE, LEDC_TIMER, freq);
        set_led_duty(BUZZER_CHANNEL, 512);
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    // Descendo
    for (int freq = 2000; freq >= 500; freq -= 50) {
        ledc_set_freq(LEDC_MODE, LEDC_TIMER, freq);
        set_led_duty(BUZZER_CHANNEL, 512);
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    // Desliga buzzer
    set_led_duty(BUZZER_CHANNEL, 0);
}

// ================= MAIN =================
void app_main(void) {

    pwm_init();

    while (1) {

        fase1_fading();
        // Fase 2
        fase2_fading_seq();

        // Fase 3
        fase3_buzzer();
    }
}