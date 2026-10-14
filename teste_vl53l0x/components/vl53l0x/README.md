# VL53L0X — transporte nativo ESP-IDF

Adaptado de [Pololu VL53L0X 1.3.1](https://github.com/pololu/vl53l0x-arduino/tree/1.3.1).
Licenças e atribuições Pololu/ST em `LICENSE.txt`.

A sequência de inicialização, calibração e cálculo do orçamento de medição é da
Pololu. O transporte Arduino/Wire foi substituído por `i2c_master_transmit` e
`i2c_master_transmit_receive` do ESP-IDF; o relógio usa `esp_timer_get_time`.
As esperas cedem a CPU ao FreeRTOS. Não requer Arduino nem download de componentes.

`setBus(bus, frequencia_hz, timeout_ms)` associa o sensor ao barramento criado pela
aplicação. `setAddress` atualiza também o handle do dispositivo. `last_status`
contém um `esp_err_t`: o primeiro erro fica retido até `clearStatus()`. Limpe-o
antes de cada tentativa de leitura e confira-o depois de operações de configuração.
Use sempre `setTimeout` com valor não zero para limitar as esperas pelo sensor.
A aplicação mantém o barramento e o dispositivo durante toda a execução.

Leituras são distâncias brutas: ausência de timeout/erro I²C não comprova validade
óptica ou precisão. Valide o sensor e a montagem em bancada.
