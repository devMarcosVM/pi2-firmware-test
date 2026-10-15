#include <stdio.h>
#include "driver/gpio.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ===== CONFIGURAÇÃO (ESP32 clássico) =====
#define CFG_PINO_SAIDA_DIGITAL GPIO_NUM_36
#define CFG_NIVEL_DETECCAO 0
#define CFG_INTERVALO_LEITURA_MS 50

void app_main(void)
{
    const gpio_config_t entrada = {
        .pin_bit_mask = 1ULL << CFG_PINO_SAIDA_DIGITAL,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&entrada));
    printf("Teste LM393 | UART0 via USB | 115200 baud, 8N1\n");
    while (1) {
        int nivel = gpio_get_level(CFG_PINO_SAIDA_DIGITAL);
        printf("Obstáculo detectado: %s | Saída digital: %s\n",
               nivel == CFG_NIVEL_DETECCAO ? "SIM" : "NÃO",
               nivel == 0 ? "LOW" : "HIGH");
        vTaskDelay(pdMS_TO_TICKS(CFG_INTERVALO_LEITURA_MS));
    }
}
