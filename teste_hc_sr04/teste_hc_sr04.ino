// ===== CONFIGURAÇÃO =====
constexpr unsigned long CFG_BAUD_RATE = 115200; // UART0 via USB, 8N1 (PC1).
constexpr int CFG_PINO_TRIG = 25; // TRIG do HC-SR04 (PC1).
constexpr int CFG_PINO_ECHO = 26; // ECHO via divisor 1 kΩ / 2 kΩ (PC1).
constexpr unsigned long CFG_INTERVALO_DISPARO_MS = 100; // 10 Hz, conforme PC1.
constexpr unsigned long CFG_PULSO_TRIG_US = 10; // Pulso de disparo do HC-SR04.

// ===== OBJETOS GLOBAIS =====
portMUX_TYPE travaEco = portMUX_INITIALIZER_UNLOCKED;
volatile bool capturaAtiva = false;
volatile bool subidaRecebida = false;
volatile bool ecoDisponivel = false;constexpr unsigned long CFG_TIMEOUT_ECHO_US = 30000; // Limite total; cobre eco de 4 m.
constexpr float CFG_VELOCIDADE_SOM_MM_US = 0.343f; // 343 m/s, estimativa do PC1.
constexpr float CFG_FATOR_IDA_VOLTA = 2.0f; // Eco percorre ida e volta.
constexpr unsigned int CFG_CASAS_DECIMAIS = 1; // Precisão de exibição, não do sensor.
constexpr unsigned long CFG_PAUSA_LOOP_MS = 1; // Cede CPU enquanto aguarda interrupção.

#include <Arduino.h>

volatile unsigned long inicioEcoUs = 0;
volatile unsigned long duracaoEcoUs = 0;
bool aguardandoEco = false;
unsigned long inicioDisparoUs = 0;
unsigned long ultimoDisparoMs = 0;

// ===== SETUP =====
void setup() {
  Serial.begin(CFG_BAUD_RATE);
  Serial.print("Teste HC-SR04 | UART0 via USB | ");
  Serial.print(CFG_BAUD_RATE);
  Serial.println(" baud, 8N1");
  inicializarSensor();
  ultimoDisparoMs = millis() - CFG_INTERVALO_DISPARO_MS;
}

// ===== LOOP =====
/** Agenda disparos e trata resultado/timeout sem bloquear a espera pelo eco. */
void loop() {
  if (aguardandoEco) {
    unsigned long larguraUs = 0;
    bool leituraPronta = lerDistancia(larguraUs);
    if (leituraPronta) {
      aguardandoEco = false;
      imprimirLeitura(larguraUs);
    } else if (micros() - inicioDisparoUs >= CFG_TIMEOUT_ECHO_US) {
      portENTER_CRITICAL(&travaEco);
      capturaAtiva = false;
      portEXIT_CRITICAL(&travaEco);
      aguardandoEco = false;
      Serial.println("Distância indisponível: timeout de eco.");
    }
  }
  if (!aguardandoEco && millis() - ultimoDisparoMs >= CFG_INTERVALO_DISPARO_MS) {
    dispararMedicao();
  }
  delay(CFG_PAUSA_LOOP_MS);
}

// ===== FUNÇÕES AUXILIARES =====
/** Configura TRIG como saída e ECHO como entrada com interrupção. */
void inicializarSensor() {
  digitalWrite(CFG_PINO_TRIG, LOW);
  pinMode(CFG_PINO_TRIG, OUTPUT);
  pinMode(CFG_PINO_ECHO, INPUT);
  attachInterrupt(digitalPinToInterrupt(CFG_PINO_ECHO), capturarEco, CHANGE);
}

/** ISR: registra as bordas do eco; não imprime nem aguarda. Sem retorno. */
void ARDUINO_ISR_ATTR capturarEco() {
  const unsigned long agoraUs = micros();
  const int nivelEco = digitalRead(CFG_PINO_ECHO);
  portENTER_CRITICAL_ISR(&travaEco);
  if (capturaAtiva) {
    if (nivelEco == HIGH && !subidaRecebida) {
      inicioEcoUs = agoraUs;
      subidaRecebida = true;
    } else if (nivelEco == LOW && subidaRecebida) {
      duracaoEcoUs = agoraUs - inicioEcoUs;
      ecoDisponivel = true;
      capturaAtiva = false;
    }
  }
  portEXIT_CRITICAL_ISR(&travaEco);
}

/** Arma captura e gera pulso curto de TRIG. Sem parâmetros ou retorno. */
void dispararMedicao() {
  portENTER_CRITICAL(&travaEco);
  subidaRecebida = false;
  ecoDisponivel = false;
  capturaAtiva = true;
  portEXIT_CRITICAL(&travaEco);
  aguardandoEco = true;
  ultimoDisparoMs = millis();
  inicioDisparoUs = micros();
  digitalWrite(CFG_PINO_TRIG, HIGH);
  delayMicroseconds(CFG_PULSO_TRIG_US);
  digitalWrite(CFG_PINO_TRIG, LOW);
}

/** Copia largura em µs para a referência; retorna se existe novo eco. */
bool lerDistancia(unsigned long &larguraUs) {
  portENTER_CRITICAL(&travaEco);
  const bool leituraPronta = ecoDisponivel;
  if (leituraPronta) {
    larguraUs = duracaoEcoUs;
    ecoDisponivel = false;
  }
  portEXIT_CRITICAL(&travaEco);
  return leituraPronta;
}

/** Recebe largura do eco em µs e imprime distância estimada em mm. */
void imprimirLeitura(unsigned long larguraUs) {
  const float distanciaMm = larguraUs * CFG_VELOCIDADE_SOM_MM_US / CFG_FATOR_IDA_VOLTA;
  Serial.print("Distância: ");
  Serial.print(distanciaMm, CFG_CASAS_DECIMAIS);
  Serial.println(" mm");
}
