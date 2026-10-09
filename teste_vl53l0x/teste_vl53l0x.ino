// ===== CONFIGURAÇÃO =====
constexpr unsigned long CFG_BAUD_RATE = 115200; // UART0 via USB, 8N1 (PC1).
constexpr int CFG_PINO_SDA = 21; // SDA compartilhado dos ToF (mapa de pinos).
constexpr int CFG_PINO_SCL = 22; // SCL compartilhado dos ToF (mapa de pinos).
constexpr int CFG_PINO_XSHUT = 17; // U12 / 2XSHUT; para U13, usar 18.
constexpr int CFG_OUTROS_XSHUT[] = {16, 18}; // Mantidos desligados; para U13, usar {16, 17}.
constexpr unsigned char CFG_ENDERECO_PADRAO = 0x29; // Endereço após reset.
constexpr unsigned char CFG_ENDERECO_I2C = 0x31; // Endereço de U12; para U13, usar 0x32.
constexpr unsigned long CFG_FREQUENCIA_I2C_HZ = 400000; // Fast Mode (PC1).
constexpr unsigned long CFG_ORCAMENTO_MEDICAO_US = 33000; // Contínuo com 33 ms (PC1).
constexpr unsigned int CFG_TIMEOUT_I2C_MS = 50; // Limite de uma transação; escolha de bancada.
constexpr unsigned int CFG_TIMEOUT_SENSOR_MS = 100; // Limite de espera por amostra; bancada.
constexpr unsigned long CFG_INTERVALO_LEITURA_MS = 50; // Pausa entre leituras; escolha de bancada de 50 ms.
constexpr unsigned long CFG_ESPERA_XSHUT_MS = 10; // Estabilização após reset; bancada.
constexpr unsigned long CFG_INTERVALO_ERRO_MS = 1000; // Evita inundar o monitor serial.

#include <Arduino.h>
#include <Wire.h>
#include <VL53L0X.h>

// ===== OBJETOS GLOBAIS =====
VL53L0X sensorDistancia;
bool sensorInicializado = false;

// ===== SETUP =====
void setup() {
  Serial.begin(CFG_BAUD_RATE);
  Serial.print("Teste VL53L0X | UART0 via USB | ");
  Serial.print(CFG_BAUD_RATE);
  Serial.println(" baud, 8N1");
  sensorInicializado = inicializarSensor();
}

// ===== LOOP =====
/** Obtém e imprime uma leitura, ou informa falha de inicialização. */
void loop() {
  if (!sensorInicializado) {
    Serial.println("Sensor indisponível. Confira ligações e reinicie a ESP32.");
    delay(CFG_INTERVALO_ERRO_MS);
    return;
  }
  unsigned int distanciaMm = 0;
  bool leituraValida = lerDistancia(distanciaMm);
  imprimirLeitura(distanciaMm, leituraValida);
  delay(CFG_INTERVALO_LEITURA_MS);
}

// ===== FUNÇÕES AUXILIARES =====
/** Configura I2C, isola os outros ToF e inicia U12. Retorna sucesso. */
bool inicializarSensor() {
  for (int pino : CFG_OUTROS_XSHUT) {
    digitalWrite(pino, LOW);
    pinMode(pino, OUTPUT);
  }
  digitalWrite(CFG_PINO_XSHUT, LOW);
  pinMode(CFG_PINO_XSHUT, OUTPUT);
  delay(CFG_ESPERA_XSHUT_MS);
  if (!Wire.begin(CFG_PINO_SDA, CFG_PINO_SCL, CFG_FREQUENCIA_I2C_HZ)) {
    Serial.println("Erro ao iniciar I2C.");
    return false;
  }
  Wire.setTimeOut(CFG_TIMEOUT_I2C_MS);
  // Libera XSHUT sem forçar nível alto: utiliza o pull-up do breakout.
  pinMode(CFG_PINO_XSHUT, INPUT);
  delay(CFG_ESPERA_XSHUT_MS);
  Wire.beginTransmission(CFG_ENDERECO_PADRAO);
  if (Wire.endTransmission() != 0) {
    Serial.println("Sensor não respondeu no endereço de fábrica.");
    return false;
  }
  sensorDistancia.setTimeout(CFG_TIMEOUT_SENSOR_MS);
  if (!sensorDistancia.init()) {
    Serial.println("VL53L0X não respondeu em 0x29.");
    return false;
  }
  sensorDistancia.setAddress(CFG_ENDERECO_I2C);
  Wire.beginTransmission(CFG_ENDERECO_I2C);
  if (Wire.endTransmission() != 0) {
    Serial.println("Erro ao confirmar novo endereço I2C.");
    return false;
  }
  if (!sensorDistancia.setMeasurementTimingBudget(CFG_ORCAMENTO_MEDICAO_US)) {
    Serial.println("Orçamento de medição inválido.");
    return false;
  }
  sensorDistancia.startContinuous();
  Serial.print("Sensor iniciado no endereço 0x");
  Serial.println(CFG_ENDERECO_I2C, HEX);
  return true;
}

/** Escreve distância em mm na referência; retorna ausência de timeout/erro I2C. */
bool lerDistancia(unsigned int &distanciaMm) {
  distanciaMm = sensorDistancia.readRangeContinuousMillimeters();
  return !sensorDistancia.timeoutOccurred() && sensorDistancia.last_status == 0;
}

/** Imprime a distância bruta ou erro; recebe mm e validade da comunicação. */
void imprimirLeitura(unsigned int distanciaMm, bool leituraValida) {
  if (!leituraValida) {
    Serial.println("Leitura indisponível: timeout ou erro I2C.");
    return;
  }
  Serial.print("Distância: ");
  Serial.print(distanciaMm);
  Serial.println(" mm (leitura bruta)");
}
