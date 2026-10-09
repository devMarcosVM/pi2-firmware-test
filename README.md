# Testes de sensores — ESP32

Três sketches independentes para validar distância e detecção de obstáculos. Carregue **um teste por vez** na ESP32.

**Placa:** ESP32 Dev Module (`esp32:esp32:esp32`). **Serial:** USB/UART0, **115200 baud, 8N1**.

## Configurações atuais

| Sketch | Ligações ESP32 ↔ sensor | Alimentação | Leitura |
|---|---|---|---|
| [VL53L0X](/Users/yaba/Sandbox/pi2-grupo2-teste/teste_vl53l0x/teste_vl53l0x.ino) | **GPIO21** → SDA<br>**GPIO22** → SCL<br>**GPIO17** → XSHUT (U12) | 3,3 V | I²C 400 kHz; endereço `0x29` → `0x31`; modo contínuo, orçamento 33 ms; pausa entre leituras 50 ms |
| [HC-SR04](/Users/yaba/Sandbox/pi2-grupo2-teste/teste_hc_sr04/teste_hc_sr04.ino) | **GPIO25** → TRIG<br>**GPIO26** ← ECHO (via divisor) | 5 V | Disparo a cada 100 ms; pulso TRIG 10 µs; eco por interrupção; timeout 30 ms |
| [LM393](/Users/yaba/Sandbox/pi2-grupo2-teste/teste_lm393/teste_lm393.ino) | **GPIO36** ← OUT/DO<br>Frontal esquerdo | 3,3 V | **LOW = detectado**; leitura a cada 50 ms |

Todos os sensores devem compartilhar **GND com a ESP32**. Altere pinos e parâmetros somente no bloco `// ===== CONFIGURAÇÃO =====` do sketch.

### Cuidados de ligação

- **HC-SR04:** `ECHO → 1 kΩ → GPIO26 → 2 kΩ → GND`. Não ligue ECHO de 5 V diretamente à ESP32.
- **VL53L0X:** GPIO16 e GPIO18 mantêm os outros ToF desligados, se conectados. O teste depende do pull-up de XSHUT do breakout. Confira também os pull-ups de SDA/SCL; se ausentes, use 4,7 kΩ para 3,3 V. Timeouts: I²C 50 ms e sensor 100 ms.
- **LM393:** use a saída digital, com nível máximo de 3,3 V. GPIO36 não tem pull-up interno; confirme o pull-up do módulo/externo se a saída for coletor aberto. Ajuste a sensibilidade no potenciômetro.

### Trocar o canal testado

| Sensor | Alterações no bloco de configuração |
|---|---|
| VL53L0X U13 | `CFG_PINO_XSHUT = 18`, `CFG_OUTROS_XSHUT = {16, 17}`, `CFG_ENDERECO_I2C = 0x32` |
| LM393 | `CFG_PINO_SAIDA_DIGITAL`: 39 (frontal direito), 34 (esquerdo) ou 35 (direito) |

## Instalação

Na Arduino IDE, adicione esta URL em **Preferências → URLs adicionais do Gerenciador de Placas**:

```text
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

Instale **esp32 by Espressif Systems** no Gerenciador de Placas e selecione **ESP32 Dev Module**. No Gerenciador de Bibliotecas, instale **VL53L0X by Pololu**. HC-SR04 e LM393 não precisam de bibliotecas adicionais.

Preparação equivalente pela CLI:

```sh
arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli lib update-index
arduino-cli lib install "VL53L0X"
```

## Compilar e executar

**IDE:** abra o `.ino` desejado, selecione a placa e a porta, carregue e abra o Monitor Serial.

**CLI:** execute na pasta do projeto. Substitua `<porta>` pela porta identificada em `arduino-cli board list`.

```sh
cd /Users/yaba/Sandbox/pi2-grupo2-teste
arduino-cli board list

# VL53L0X
arduino-cli compile --fqbn esp32:esp32:esp32 teste_vl53l0x
arduino-cli upload -p <porta> --fqbn esp32:esp32:esp32 teste_vl53l0x

# HC-SR04
arduino-cli compile --fqbn esp32:esp32:esp32 teste_hc_sr04
arduino-cli upload -p <porta> --fqbn esp32:esp32:esp32 teste_hc_sr04

# LM393
arduino-cli compile --fqbn esp32:esp32:esp32 teste_lm393
arduino-cli upload -p <porta> --fqbn esp32:esp32:esp32 teste_lm393

# Monitor serial após o upload; feche-o antes de carregar outro teste.
arduino-cli monitor -p <porta> --config baudrate=115200
```

## O que aparece no monitor

| Sensor | Exemplo |
|---|---|
| VL53L0X | `Distância: 123 mm (leitura bruta)` |
| HC-SR04 | `Distância: 123.4 mm` |
| LM393 | `Obstáculo detectado: SIM \| Saída digital: LOW` |

Os testes de distância informam erros/timeouts. No VL53L0X, uma leitura recebida sem erro de comunicação não garante qualidade óptica; confirme valores fora de alcance em bancada.

## Referências e validação

Pinagem: [PC1 — Mapa de pinos, Tabela 1](/Users/yaba/Sandbox/pi2-grupo-2/docs/PontoDeControle_01/Build-Software-Embarcado/mapa-de-pinos.md:19). Alimentação e divisor: [Esquemático, seções 2.1 e 2.3 / Figura 1](/Users/yaba/Sandbox/pi2-grupo-2/docs/PontoDeControle_01/Eletronica-Energia/esquematico.md:25). I²C, pull-ups, serial e temporização: [Barramentos, seções I²C, UART0 e interfaces ponto a ponto](/Users/yaba/Sandbox/pi2-grupo-2/docs/PontoDeControle_01/Eletronica-Energia/barramentos-comunicacao.md:62). Modelo IR: [Lista final de componentes](/Users/yaba/Sandbox/pi2-grupo-2/docs/Geral/analise-financeira.md:90).