#include <cstdio>
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "VL53L0X.h"

// ===== CONFIGURAÇÃO (ESP32 clássico) =====
static constexpr gpio_num_t CFG_PINO_SDA = GPIO_NUM_21;
static constexpr gpio_num_t CFG_PINO_SCL = GPIO_NUM_22;
static constexpr gpio_num_t CFG_PINO_XSHUT = GPIO_NUM_17;
static constexpr gpio_num_t CFG_OUTROS_XSHUT[] = {GPIO_NUM_16, GPIO_NUM_18};
static constexpr uint8_t CFG_ENDERECO_PADRAO = 0x29;
static constexpr uint8_t CFG_ENDERECO_I2C = 0x31;
static constexpr uint32_t CFG_FREQUENCIA_I2C_HZ = 400000;
static constexpr uint32_t CFG_ORCAMENTO_MEDICAO_US = 33000;
static constexpr int CFG_TIMEOUT_I2C_MS = 50;
static constexpr uint16_t CFG_TIMEOUT_SENSOR_MS = 100;
static constexpr int CFG_INTERVALO_LEITURA_MS = 50;
static constexpr int CFG_ESPERA_XSHUT_MS = 10;
static constexpr int CFG_INTERVALO_ERRO_MS = 1000;

static VL53L0X sensor;
static i2c_master_bus_handle_t barramento;

static void desligar_sensor(gpio_num_t pino)
{
    // XSHUT é drenado para GND e liberado pelo pull-up do breakout.
    const gpio_config_t saida = {
        .pin_bit_mask = 1ULL << pino,
        .mode = GPIO_MODE_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_set_level(pino, 0));
    ESP_ERROR_CHECK(gpio_config(&saida));
}

static bool inicializar_sensor()
{
    for (gpio_num_t pino : CFG_OUTROS_XSHUT) {
        desligar_sensor(pino);
    }
    desligar_sensor(CFG_PINO_XSHUT);
    vTaskDelay(pdMS_TO_TICKS(CFG_ESPERA_XSHUT_MS));

    i2c_master_bus_config_t config = {};
    config.i2c_port = I2C_NUM_0;
    config.sda_io_num = CFG_PINO_SDA;
    config.scl_io_num = CFG_PINO_SCL;
    config.clk_source = I2C_CLK_SRC_DEFAULT;
    config.glitch_ignore_cnt = 7;
    // Pull-ups externos para 3,3 V são necessários em 400 kHz.
    config.flags.enable_internal_pullup = false;
    ESP_ERROR_CHECK(i2c_new_master_bus(&config, &barramento));
    ESP_ERROR_CHECK(gpio_set_level(CFG_PINO_XSHUT, 1));
    vTaskDelay(pdMS_TO_TICKS(CFG_ESPERA_XSHUT_MS));

    if (i2c_master_probe(barramento, CFG_ENDERECO_PADRAO, CFG_TIMEOUT_I2C_MS) != ESP_OK) {
        printf("Sensor não respondeu no endereço de fábrica 0x29.\n");
        return false;
    }
    if (sensor.setBus(barramento, CFG_FREQUENCIA_I2C_HZ, CFG_TIMEOUT_I2C_MS) != ESP_OK) {
        printf("Erro ao associar sensor ao I²C: %s\n", esp_err_to_name(sensor.last_status));
        return false;
    }
    sensor.setTimeout(CFG_TIMEOUT_SENSOR_MS);
    if (!sensor.init()) {
        printf("Falha ao inicializar/calibrar VL53L0X: %s\n", esp_err_to_name(sensor.last_status));
        return false;
    }
    sensor.setAddress(CFG_ENDERECO_I2C);
    if (sensor.last_status != ESP_OK ||
        i2c_master_probe(barramento, CFG_ENDERECO_I2C, CFG_TIMEOUT_I2C_MS) != ESP_OK) {
        printf("Erro ao confirmar novo endereço I²C.\n");
        return false;
    }
    if (!sensor.setMeasurementTimingBudget(CFG_ORCAMENTO_MEDICAO_US)) {
        printf("Falha ao configurar orçamento de medição: %s\n", esp_err_to_name(sensor.last_status));
        return false;
    }
    sensor.startContinuous();
    if (sensor.last_status != ESP_OK) {
        printf("Erro ao iniciar modo contínuo: %s\n", esp_err_to_name(sensor.last_status));
        return false;
    }
    printf("Sensor iniciado no endereço 0x%02X\n", CFG_ENDERECO_I2C);
    return true;
}

extern "C" void app_main(void)
{
    printf("Teste VL53L0X | UART0 via USB | 115200 baud, 8N1\n");
    const bool inicializado = inicializar_sensor();
    while (true) {
        if (!inicializado) {
            printf("Sensor indisponível. Confira ligações e reinicie a ESP32.\n");
            vTaskDelay(pdMS_TO_TICKS(CFG_INTERVALO_ERRO_MS));
            continue;
        }
        sensor.clearStatus();
        const uint16_t distancia = sensor.readRangeContinuousMillimeters();
        const bool timeout = sensor.timeoutOccurred();
        if (timeout || sensor.last_status != ESP_OK) {
            printf("Leitura indisponível: timeout ou erro I²C (%s).\n",
                   esp_err_to_name(sensor.last_status));
        } else {
            printf("Distância: %u mm (leitura bruta)\n", static_cast<unsigned>(distancia));
        }
        vTaskDelay(pdMS_TO_TICKS(CFG_INTERVALO_LEITURA_MS));
    }
}
