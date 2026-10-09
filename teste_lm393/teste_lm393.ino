// ===== CONFIGURAÇÃO =====
constexpr unsigned long CFG_BAUD_RATE = 115200; // UART0 via USB, 8N1 (PC1).
constexpr int CFG_PINO_SAIDA_DIGITAL = 36; // IR_esquerdo_Frontal (PC1).
constexpr int CFG_NIVEL_DETECCAO = 0; // LOW: polaridade confirmada pelo usuário.
constexpr unsigned long CFG_INTERVALO_LEITURA_MS = 50; // Leitura a 20 Hz (PC1).

#include <Arduino.h>

// ===== SETUP =====
void setup() {
  Serial.begin(CFG_BAUD_RATE);
  Serial.print("Teste LM393 | UART0 via USB | ");
  Serial.print(CFG_BAUD_RATE);
  Serial.println(" baud, 8N1");
  inicializarSensor();
}

// ===== LOOP =====
/** Lê a saída, imprime detecção e aguarda o próximo ciclo. */
void loop() {
  int nivelDigital = lerSensor();
  imprimirLeitura(nivelDigital);
  delay(CFG_INTERVALO_LEITURA_MS);
}

// ===== FUNÇÕES AUXILIARES =====
/** Configura a entrada. GPIO36 não possui resistores internos. Sem retorno. */
void inicializarSensor() {
  pinMode(CFG_PINO_SAIDA_DIGITAL, INPUT);
}

/** Sem parâmetros; retorna HIGH ou LOW observado na saída do módulo. */
int lerSensor() {
  return digitalRead(CFG_PINO_SAIDA_DIGITAL);
}

/** Recebe o nível digital e imprime presença; sem retorno. */
void imprimirLeitura(int nivelDigital) {
  Serial.print("Obstáculo detectado: ");
  Serial.print(nivelDigital == CFG_NIVEL_DETECCAO ? "SIM" : "NÃO");
  Serial.print(" | Saída digital: ");
  Serial.println(nivelDigital == LOW ? "LOW" : "HIGH");
}
