# Testes de sensores — ESP32 com ESP-IDF

Três projetos **independentes** para ESP32 clássico (ESP32 Dev Module), com
console UART0 a **115200 baud, 8N1**. Grave um teste por vez: o próximo firmware
substitui o anterior na placa.

Os projetos usam APIs nativas do ESP-IDF, sem Arduino. Os `.ino` originais foram
preservados como referência e não entram na compilação. Use ESP-IDF **5.3 ou
superior** (drivers separados de GPIO/I²C); a versão validada está na seção final.
A pinagem abaixo é para ESP32 clássico: ESP32-C3/S3 exigem revisão dos pinos.

| Projeto | Arquivo principal | Ligações ESP32 ↔ sensor | Alimentação | Leitura |
|---|---|---|---|---|
| VL53L0X | [main.cpp](teste_vl53l0x/main/main.cpp) | GPIO21 → SDA; GPIO22 → SCL; GPIO17 → XSHUT (U12) | 3,3 V | I²C 400 kHz; endereço `0x29` → `0x31`; contínuo, orçamento 33 ms; pausa 50 ms |
| HC-SR04 | [main.c](teste_hc_sr04/main/main.c) | GPIO25 → TRIG; GPIO26 ← ECHO via divisor | 5 V | Disparo a cada 100 ms; TRIG 10 µs; captura por interrupção; timeout 30 ms |
| LM393 | [main.c](teste_lm393/main/main.c) | GPIO36 ← OUT/DO (frontal esquerdo) | 3,3 V | LOW = detectado; leitura a cada 50 ms |

Todos os sensores devem compartilhar **GND com a ESP32**. Pinos e parâmetros
ficam no bloco `CONFIGURAÇÃO` de cada arquivo principal.

## Ligações

- **HC-SR04:** `ECHO → 1 kΩ → GPIO26 → 2 kΩ → GND`. Não ligue ECHO de 5 V diretamente à ESP32.
- **VL53L0X:** GPIO16 e GPIO18 mantêm os outros ToF desligados. XSHUT usa saída de dreno aberto: LOW desliga e HIGH libera a linha para o pull-up do breakout, sem aplicar 3,3 V ativamente em XSHUT. Confira esse pull-up. SDA/SCL precisam de pull-ups externos para 3,3 V (por exemplo, 4,7 kΩ, conforme a montagem). Timeouts: transação I²C 50 ms, espera pelo sensor 100 ms.
- **LM393:** use OUT/DO digital com nível máximo de 3,3 V. GPIO36 não possui pull-up interno; confirme o pull-up do módulo/externo se a saída for coletor aberto. Ajuste a sensibilidade no potenciômetro.

Para testar VL53L0X U13, configure `CFG_PINO_XSHUT = GPIO_NUM_18`,
`CFG_OUTROS_XSHUT = {GPIO_NUM_16, GPIO_NUM_17}` e `CFG_ENDERECO_I2C = 0x32`.
Para outros LM393, altere `CFG_PINO_SAIDA_DIGITAL`: GPIO39 (frontal direito),
GPIO34 (esquerdo) ou GPIO35 (direito).

## Ativar o ambiente ESP-IDF

No computador atual foi encontrada uma instalação ESP-IDF 6.0.1. Em **cada novo
terminal Bash**, ative-a com:

```bash
source "$HOME/.espressif/tools/activate_idf_v6.0.1.sh"
idf.py --version
```

Em outra instalação, use o script dela, por exemplo:

```bash
source "$HOME/esp/esp-idf/export.sh"
```

Se o ESP-IDF ainda não estiver instalado, siga a
[instalação oficial para Linux/macOS](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html).
Não misture o ambiente de uma versão com o diretório de outra.

## Compilar, gravar e acompanhar cada teste

Execute a partir da raiz deste repositório. Primeiro identifique a porta USB:

```bash
cd /home/marcos/Documentos/2026.1/pi2-firmware-test
ls /dev/serial/by-id/
# Alternativa:
ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null
```

Nos exemplos abaixo, substitua `/dev/ttyUSB0` pela porta da sua placa.
`set-target` é necessário na primeira configuração de cada projeto ou ao trocar
o chip; ele recria a configuração e o diretório de build. Para recompilar o mesmo
projeto depois, basta `build` ou `flash`.

```bash
# LM393
idf.py -C teste_lm393 set-target esp32
idf.py -C teste_lm393 build
idf.py -C teste_lm393 -p /dev/ttyUSB0 flash monitor

# Saia do monitor com Ctrl+] antes de gravar o próximo teste.

# HC-SR04
idf.py -C teste_hc_sr04 set-target esp32
idf.py -C teste_hc_sr04 build
idf.py -C teste_hc_sr04 -p /dev/ttyUSB0 flash monitor

# VL53L0X
idf.py -C teste_vl53l0x set-target esp32
idf.py -C teste_vl53l0x build
idf.py -C teste_vl53l0x -p /dev/ttyUSB0 flash monitor
```

`flash` já compila as alterações; `build` separado permite conferir a compilação
sem conectar a placa. Você também pode entrar na pasta do sensor e omitir `-C`:

```bash
cd teste_lm393
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
# Apenas abrir o monitor, sem gravar:
idf.py -p /dev/ttyUSB0 monitor
# Ajustar configurações do firmware:
idf.py menuconfig
# Limpar a compilação quando necessário:
idf.py fullclean
```

Se ocorrer erro de permissão na serial, verifique o grupo da porta com
`ls -l /dev/ttyUSB0`. Em distribuições que usam `dialout`, adicione seu usuário
com `sudo usermod -aG dialout "$USER"` e encerre/inicie sua sessão.
Se a placa não entrar automaticamente no modo de gravação, mantenha BOOT
pressionado durante a conexão do gravador e solte quando a gravação começar.

## Criar um novo projeto ESP-IDF do zero

Depois de ativar o ambiente, escolha uma pasta para seus projetos:

```bash
mkdir -p "$HOME/esp/projetos"
cd "$HOME/esp/projetos"
idf.py create-project meu_projeto
cd meu_projeto
idf.py set-target esp32
# Edite main/meu_projeto.c: a entrada do firmware é void app_main(void).
idf.py menuconfig
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

No ESP-IDF, `app_main()` inicia a aplicação. Use `vTaskDelay(pdMS_TO_TICKS(ms))`
para pausas que cedem a CPU. Em C++, declare `extern "C" void app_main(void)`.
Consulte os [comandos oficiais do idf.py](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/tools/idf-py.html)
e o [guia de compilação e gravação](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/start-project.html).

## Estrutura

Cada pasta possui `CMakeLists.txt`, `sdkconfig.defaults` e `main/` com seu
componente e arquivo principal. `sdkconfig.defaults` seleciona UART0 e tick
FreeRTOS de 1 ms; a UART padrão usa 115200 baud. `build/` e `sdkconfig` são
arquivos locais gerados e ignorados pelo Git.

O VL53L0X inclui o [driver adaptado da Pololu 1.3.1](teste_vl53l0x/components/vl53l0x/README.md)
com sua licença. Ele usa o [driver I²C master do ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/i2c.html),
sem dependências externas a baixar.

## Saída esperada e validação

| Sensor | Exemplo de mensagem |
|---|---|
| VL53L0X | `Distância: 123 mm (leitura bruta)` |
| HC-SR04 | `Distância: 123.4 mm` |
| LM393 | `Obstáculo detectado: SIM \| Saída digital: LOW` |

Os testes de distância informam erros/timeouts. No VL53L0X, ausência de erro I²C
não garante qualidade óptica; confirme alcance e precisão em bancada.

Validação de software: compilação dos três projetos para `esp32` com ESP-IDF
6.0.1. O teste físico dos sensores e a gravação precisam de uma placa conectada.
