/*
 * ETAPA 2 - Ultrassônico + Indutivo + Ponte H (HW-095)
 *
 * 1. O HC-SR04 detecta um objeto -> esteira AVANÇA.
 * 2. Enquanto avança, o sensor indutivo vigia a passagem da peça:
 *      - Detectou METAL  -> para um instante e gira para TRÁS por 10 s
 *      - Não detectou    -> esteira continua avançando por 1 minuto e desliga
 * 3. Ao terminar, aguarda a retirada da peça para rearmar.
 *
 * Mesma pinagem do programa principal:
 *   HC-SR04  : TRIG = D9, ECHO = D10
 *   Indutivo : sinal = D2 (INPUT_PULLUP, NPN: LOW = metal detectado)
 *   HW-095   : IN1 = D5 (PWM), IN2 = D6 (PWM)
 *
 * Indutivo NPN: alimente com 12 V, una o GND com o do Arduino e
 * ligue o fio de sinal (preto) em D2. NUNCA ligue o sinal a 12 V.
 *
 * Monitor Serial: 9600 baud
 */

// ---------- Pinos ----------
const uint8_t PIN_TRIG     = 9;
const uint8_t PIN_ECHO     = 10;
const uint8_t PIN_INDUTIVO = 2;
const uint8_t PIN_MOTOR_A  = 5;   // IN1
const uint8_t PIN_MOTOR_B  = 6;   // IN2

// ---------- Parâmetros ----------
const uint8_t       VELOCIDADE        = 150;       // 0-255
const uint16_t      DIST_DETECCAO     = 6;         // cm
const unsigned long TEMPO_AVANCO_MS   = 60000UL;   // 1 minuto
const unsigned long TEMPO_RE_MS       = 10000UL;   // 10 segundos para trás
const uint16_t      PAUSA_INVERSAO_MS = 300;       // pausa antes de inverter o sentido

// ---------- Estados ----------
enum Estado { AGUARDANDO, AVANCANDO, EM_RE, AGUARDA_REMOCAO };
Estado estado = AGUARDANDO;
unsigned long inicioMs = 0;

// ===================== ESTEIRA =====================
void esteiraAvancar() {
  analogWrite(PIN_MOTOR_A, VELOCIDADE);
  analogWrite(PIN_MOTOR_B, 0);
}

void esteiraRe() {
  analogWrite(PIN_MOTOR_A, 0);
  analogWrite(PIN_MOTOR_B, VELOCIDADE);
}

void esteiraParar() {
  analogWrite(PIN_MOTOR_A, 0);
  analogWrite(PIN_MOTOR_B, 0);
}

// ===================== SENSORES =====================
long lerDistanciaCm() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  long duracao = pulseIn(PIN_ECHO, HIGH, 30000UL);
  if (duracao == 0) return 999;
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

// Indutivo NPN normalmente aberto: LOW quando detecta metal
bool metalBruto() {
  return digitalRead(PIN_INDUTIVO) == LOW;
}

// Confirma o metal com 3 leituras em 20 ms (filtra ruído sem perder a peça passando)
bool metalDetectado() {
  for (uint8_t i = 0; i < 3; i++) {
    if (!metalBruto()) return false;
    delay(10);
  }
  return true;
}

// ===================== SETUP =====================
void setup() {
  Serial.begin(9600);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_INDUTIVO, INPUT_PULLUP);
  pinMode(PIN_MOTOR_A, OUTPUT);
  pinMode(PIN_MOTOR_B, OUTPUT);
  esteiraParar();

  Serial.println(F("ETAPA 2: aguardando objeto..."));
}

// ===================== LOOP =====================
void loop() {
  switch (estado) {

    case AGUARDANDO:
      if (objetoDetectado()) {
        Serial.println(F("Objeto detectado! Esteira AVANCANDO (max 60 s)"));
        esteiraAvancar();
        inicioMs = millis();
        estado = AVANCANDO;
      }
      break;

    case AVANCANDO: {
      // Peça de metal passou pelo indutivo?
      if (metalDetectado()) {
        Serial.println(F("METAL detectado! Parando e invertendo por 10 s"));
        esteiraParar();
        delay(PAUSA_INVERSAO_MS);
        esteiraRe();
        inicioMs = millis();
        estado = EM_RE;
        break;
      }

      unsigned long decorrido = millis() - inicioMs;
      if (decorrido >= TEMPO_AVANCO_MS) {
        esteiraParar();
        Serial.println(F("Sem metal. Tempo concluido, esteira DESLIGADA."));
        Serial.println(F("Retire a peca para rearmar."));
        estado = AGUARDA_REMOCAO;
      } else {
        static unsigned long ultimoAviso = 0;
        if (millis() - ultimoAviso >= 10000UL) {
          ultimoAviso = millis();
          Serial.print(F("Avancando... restam "));
          Serial.print((TEMPO_AVANCO_MS - decorrido) / 1000);
          Serial.println(F(" s"));
        }
      }
      break;
    }

    case EM_RE:
      if (millis() - inicioMs >= TEMPO_RE_MS) {
        esteiraParar();
        Serial.println(F("Re concluida, esteira DESLIGADA."));
        Serial.println(F("Retire a peca de metal para rearmar."));
        estado = AGUARDA_REMOCAO;
      }
      break;

    case AGUARDA_REMOCAO:
      // Rearma somente quando nada estiver na frente do ultrassônico nem do indutivo
      if (lerDistanciaCm() > DIST_DETECCAO && !metalBruto()) {
        delay(500);
        Serial.println(F("Rearmado. Aguardando objeto..."));
        estado = AGUARDANDO;
      }
      break;
  }

  delay(5);
}
