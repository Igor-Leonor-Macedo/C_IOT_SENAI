/*
 * ETAPA 1 - Ultrassônico + Ponte H (HW-095)
 *
 * Quando o HC-SR04 detectar um objeto, liga a esteira por 1 minuto e depois desliga.
 * Só rearma quando o objeto for retirado da frente do sensor.
 *
 * Mesma pinagem do programa principal:
 *   HC-SR04: TRIG = D9, ECHO = D10
 *   HW-095 : IN1 = D5 (PWM), IN2 = D6 (PWM)
 *
 * Monitor Serial: 9600 baud (mostra o que está acontecendo)
 */

// ---------- Pinos ----------
const uint8_t PIN_TRIG    = 9;
const uint8_t PIN_ECHO    = 10;
const uint8_t PIN_MOTOR_A = 5;   // IN1
const uint8_t PIN_MOTOR_B = 6;   // IN2

// ---------- Parâmetros ----------
const uint8_t       VELOCIDADE      = 150;       // 0-255
const uint16_t      DIST_DETECCAO   = 6;         // cm
const unsigned long TEMPO_LIGADA_MS = 60000UL;   // 1 minuto

// ---------- Estados ----------
enum Estado { AGUARDANDO, ESTEIRA_LIGADA, AGUARDA_REMOCAO };
Estado estado = AGUARDANDO;
unsigned long inicioMs = 0;

void esteiraLigar() {
  analogWrite(PIN_MOTOR_A, VELOCIDADE);
  analogWrite(PIN_MOTOR_B, 0);
}

void esteiraParar() {
  analogWrite(PIN_MOTOR_A, 0);
  analogWrite(PIN_MOTOR_B, 0);
}

long lerDistanciaCm() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  long duracao = pulseIn(PIN_ECHO, HIGH, 30000UL);
  if (duracao == 0) return 999;      // sem eco
  return duracao / 58;
}

// 3 leituras seguidas para evitar falso positivo
bool objetoDetectado() {
  for (uint8_t i = 0; i < 3; i++) {
    if (lerDistanciaCm() > DIST_DETECCAO) return false;
    delay(15);
  }
  return true;
}

void setup() {
  Serial.begin(9600);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_MOTOR_A, OUTPUT);
  pinMode(PIN_MOTOR_B, OUTPUT);
  esteiraParar();

  Serial.println(F("ETAPA 1: aguardando objeto..."));
}

void loop() {
  switch (estado) {

    case AGUARDANDO:
      if (objetoDetectado()) {
        Serial.println(F("Objeto detectado! Esteira LIGADA por 60 s"));
        esteiraLigar();
        inicioMs = millis();
        estado = ESTEIRA_LIGADA;
      }
      break;

    case ESTEIRA_LIGADA: {
      unsigned long decorrido = millis() - inicioMs;
      if (decorrido >= TEMPO_LIGADA_MS) {
        esteiraParar();
        Serial.println(F("Tempo concluido. Esteira DESLIGADA."));
        Serial.println(F("Retire o objeto da frente do sensor para rearmar."));
        estado = AGUARDA_REMOCAO;
      } else {
        // Mostra o tempo restante a cada 10 s
        static unsigned long ultimoAviso = 0;
        if (millis() - ultimoAviso >= 10000UL) {
          ultimoAviso = millis();
          Serial.print(F("Restam "));
          Serial.print((TEMPO_LIGADA_MS - decorrido) / 1000);
          Serial.println(F(" s"));
        }
      }
      break;
    }

    case AGUARDA_REMOCAO:
      if (lerDistanciaCm() > DIST_DETECCAO) {
        delay(500);
        Serial.println(F("Rearmado. Aguardando objeto..."));
        estado = AGUARDANDO;
      }
      break;
  }

  delay(30);
}
