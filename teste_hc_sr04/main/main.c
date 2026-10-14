#include <stdbool.h>
#include <stdio.h>
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ===== CONFIGURAÇÃO (ESP32 clássico) =====
#define CFG_PINO_TRIG GPIO_NUM_25
#define CFG_PINO_ECHO GPIO_NUM_26
#define CFG_INTERVALO_DISPARO_MS 100
#define CFG_PULSO_TRIG_US 10
#define CFG_TIMEOUT_ECHO_US 30000
#define CFG_VELOCIDADE_SOM_MM_US 0.343f

static portMUX_TYPE trava_eco = portMUX_INITIALIZER_UNLOCKED;
static volatile bool captura_ativa, subida_recebida, eco_disponivel;
static volatile int64_t inicio_eco_us, duracao_eco_us;

// Serviço de ISR sem ESP_INTR_FLAG_IRAM: não exige gpio_get_level em IRAM.
static void capturar_eco(void *arg)
{
    (void)arg;
    int64_t agora = esp_timer_get_time();
    int nivel = gpio_get_level(CFG_PINO_ECHO);
    portENTER_CRITICAL_ISR(&trava_eco);
    if (captura_ativa) {
        if (nivel && !subida_recebida) {
            inicio_eco_us = agora;
            subida_recebida = true;
        } else if (!nivel && subida_recebida) {
            duracao_eco_us = agora - inicio_eco_us;
            eco_disponivel = true;
            captura_ativa = false;
        }
    }
    portEXIT_CRITICAL_ISR(&trava_eco);
}

void app_main(void)
{
    const gpio_config_t saida = {
        .pin_bit_mask = 1ULL << CFG_PINO_TRIG,
        .mode = GPIO_MODE_OUTPUT,
    };
    const gpio_config_t entrada = {
        .pin_bit_mask = 1ULL << CFG_PINO_ECHO,
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&saida));
    ESP_ERROR_CHECK(gpio_set_level(CFG_PINO_TRIG, 0));
    ESP_ERROR_CHECK(gpio_config(&entrada));
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(CFG_PINO_ECHO, capturar_eco, NULL));
    printf("Teste HC-SR04 | UART0 via USB | 115200 baud, 8N1\n");

    int64_t ultimo_disparo = -CFG_INTERVALO_DISPARO_MS * 1000LL;
    while (1) {
        int64_t agora = esp_timer_get_time();
        if (agora - ultimo_disparo < CFG_INTERVALO_DISPARO_MS * 1000LL) {
            vTaskDelay(pdMS_TO_TICKS(1));
            continue;
        }
        portENTER_CRITICAL(&trava_eco);
        subida_recebida = false;
        eco_disponivel = false;
        captura_ativa = true;
        portEXIT_CRITICAL(&trava_eco);
        ultimo_disparo = esp_timer_get_time();
        // Mantém o pulso curto mesmo se o tick ocorrer durante o disparo.
        portENTER_CRITICAL(&trava_eco);
        gpio_set_level(CFG_PINO_TRIG, 1);
        esp_rom_delay_us(CFG_PULSO_TRIG_US);
        gpio_set_level(CFG_PINO_TRIG, 0);
        portEXIT_CRITICAL(&trava_eco);

        while (1) {
            int64_t largura = 0;
            bool timeout = false;
            portENTER_CRITICAL(&trava_eco);
            bool pronto = eco_disponivel;
            if (pronto) {
                largura = duracao_eco_us;
                eco_disponivel = false;
            } else if (esp_timer_get_time() - ultimo_disparo >= CFG_TIMEOUT_ECHO_US) {
                captura_ativa = false;
                timeout = true;
            }
            portEXIT_CRITICAL(&trava_eco);
            if (pronto) {
                printf("Distância: %.1f mm\n", largura * CFG_VELOCIDADE_SOM_MM_US / 2.0f);
                break;
            }
            if (timeout) {
                printf("Distância indisponível: timeout de eco.\n");
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
}
