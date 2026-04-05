#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

// ==========================
// Pinos
// ==========================
#define LED1_GPIO 4
#define LED2_GPIO 5
#define LED3_GPIO 6
#define LED4_GPIO 7

#define BUTTON1_GPIO 36  // Realiza a contagem
#define BUTTON2_GPIO 37  // Altera incrementador

#define DEBOUNCE_US 200000  // 200 ms de debounce

static const char *TAG = "CONTADOR";

static uint8_t contador = 0;
static uint8_t incremento = 1;
static int64_t last_press_A = 0;
static int64_t last_press_B = 0;


void config_leds() {
  // Configura os leds como saida

    gpio_config_t config;
    config.intr_type = GPIO_INTR_DISABLE;
    config.mode = GPIO_MODE_OUTPUT;
    config.pin_bit_mask = (1ULL << LED1_GPIO) |
                          (1ULL << LED2_GPIO) |
                          (1ULL << LED3_GPIO) |
                          (1ULL << LED4_GPIO);
    config.pull_down_en = 0;
    config.pull_up_en = 0;
    gpio_config(&config);
    // Inicializa LEDs apagados
    gpio_set_level(LED1_GPIO, 0);
    gpio_set_level(LED2_GPIO, 0);
    gpio_set_level(LED3_GPIO, 0);
    gpio_set_level(LED4_GPIO, 0);
}

void config_botoes() {
  // Configura otões como entrada pull-up

    gpio_config_t config;
    config.intr_type = GPIO_INTR_DISABLE;
    config.mode = GPIO_MODE_INPUT;
    config.pin_bit_mask = (1ULL << BUTTON1_GPIO) | (1ULL << BUTTON2_GPIO);
    config.pull_up_en = 0;     // sem pull-up
    config.pull_down_en = 0;   // Sem pull-down
    gpio_config(&config);
}

void atualiza_leds(uint8_t valor) {
    gpio_set_level(LED4_GPIO, (valor >> 0) & 0x01);
    gpio_set_level(LED3_GPIO, (valor >> 1) & 0x01);
    gpio_set_level(LED2_GPIO, (valor >> 2) & 0x01);
    gpio_set_level(LED1_GPIO, (valor >> 3) & 0x01);
}

void checa_botoes(void) {
    int estado_A = gpio_get_level(BUTTON1_GPIO);  // 0 quando pressionado
    int estado_B = gpio_get_level(BUTTON2_GPIO);  // 0 quando pressionado
    int64_t agora = esp_timer_get_time();

    // Botão A: incrementa contador
    if (estado_A == 0 && (agora - last_press_A) > DEBOUNCE_US) {
        last_press_A = agora;
        contador = (contador + incremento) & 0x0F;  // mantém em 4 bits
        atualiza_leds(contador);
    }

    // Botão B: alterna unidade de incremento
    if (estado_B == 0 && (agora - last_press_B) > DEBOUNCE_US) {
        last_press_B = agora;
        incremento = (incremento == 1) ? 2 : 1;
    }
}

void app_main(void) {
    config_leds();
    config_botoes();

    while (1) {
        checa_botoes();
        vTaskDelay(pdMS_TO_TICKS(50));  // leitura periódica
    }
}