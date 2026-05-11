#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"

#define PINO_LED        GPIO_NUM_3
#define UART_NUM        UART_NUM_2
#define PINO_TX         GPIO_NUM_17
#define PINO_RX         GPIO_NUM_16
#define BAUD_RATE       115200
#define TAMANHO_BUF     256

static const char *TAG = "UART_LOOPBACK";

static const char *MSG_LIGAR    = "LIGAR";
static const char *MSG_DESLIGAR = "DESLIGAR";

static void tarefa_uart(void *arg)
{
    uint8_t buffer[TAMANHO_BUF];
    bool estado_led = false;

    while (1) {
        const char *mensagem = estado_led ? MSG_DESLIGAR : MSG_LIGAR;

        uart_write_bytes(UART_NUM, mensagem, strlen(mensagem));
        ESP_LOGI(TAG, "Enviado: %s", mensagem);

        vTaskDelay(pdMS_TO_TICKS(500));

        int bytes_lidos = uart_read_bytes(UART_NUM, buffer, TAMANHO_BUF - 1, pdMS_TO_TICKS(1000));

        if (bytes_lidos > 0) {
            buffer[bytes_lidos] = '\0';
            ESP_LOGI(TAG, "Recebido: %s", (char *)buffer);

            if (strncmp((char *)buffer, MSG_LIGAR, strlen(MSG_LIGAR)) == 0) {
                estado_led = true;
                gpio_set_level(PINO_LED, 1);
                ESP_LOGI(TAG, "LED ligado");
            } else if (strncmp((char *)buffer, MSG_DESLIGAR, strlen(MSG_DESLIGAR)) == 0) {
                estado_led = false;
                gpio_set_level(PINO_LED, 0);
                ESP_LOGI(TAG, "LED apagado");
            }
        } else {
            ESP_LOGW(TAG, "Nenhum dado recebido");
        }

        vTaskDelay(pdMS_TO_TICKS(1500));
    }
}

void app_main(void)
{
    gpio_config_t conf_led = {
        .pin_bit_mask = (1ULL << PINO_LED),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&conf_led);
    gpio_set_level(PINO_LED, 0);

    uart_config_t conf_uart = {
        .baud_rate  = BAUD_RATE,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(UART_NUM, &conf_uart);
    uart_set_pin(UART_NUM, PINO_TX, PINO_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM, TAMANHO_BUF * 2, TAMANHO_BUF * 2, 0, NULL, 0);

    ESP_LOGI(TAG, "Sistema iniciado. UART2 configurada em 115200 bps.");

    xTaskCreate(tarefa_uart, "tarefa_uart", 4096, NULL, 5, NULL);
}