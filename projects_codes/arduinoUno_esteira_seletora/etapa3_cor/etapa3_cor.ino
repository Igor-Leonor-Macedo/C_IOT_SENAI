/*
 * ETAPA 3 - Ultrassônico + Indutivo + Sensor de Cor (TCS34725) + Ponte H
 *
 * 1. O HC-SR04 detecta um objeto -> esteira AVANÇA.
 * 2. Enquanto avança:
 *      - Indutivo detectou METAL -> para um instante e gira para TRÁS por 10 s
 *      - Sensor de cor reconheceu VERMELHO / VERDE / AZUL -> mostra no Monitor Serial
 *      - Nada de metal -> esteira avança por 1 minuto e desliga
 * 3. Ao terminar, aguarda a retirada da peça para rearmar.
 *
 * Pinagem (igual ao programa principal):
 *   HC-SR04  : TRIG = D9, ECHO = D10
 *   Indutivo : sinal = D2 (INPUT_PULLUP, NPN: LOW = metal)
 *   HW-095   : IN1 = D5 (PWM), IN2 = D6 (PWM)
 *   TCS34725 : SDA = A4, SCL = A5, 5V, GND
 *
 * Biblioteca: Adafruit TCS34725 (+ Adafruit BusIO)
 * Monitor Serial: 9600 baud
 */

#include <Wire.h>
#include <Adafruit_TCS34725.h>

// ---------- Pinos ----------
const uint8_t PIN_TRIG     = 9;
const uint8_t PIN_ECHO     = 10;
const uint8_t PIN_INDUTIVO = 2;
const uint8_t PIN_MOTOR_A  = 5;   // IN1
const uint8_t PIN_MOTOR_B  = 6;   // IN2

// ---------- Parâmetros da esteira ----------
const uint8_t       VELOCIDADE        = 150;       // 0-255
const uint16_t      DIST_DETECCAO     = 6;         // cm
const unsigned long TEMPO_AVANCO_MS   = 60000UL;   // 1 minuto
const unsigned long TEMPO_RE_MS       = 10000UL;   // 10 segundos para trás
const uint16_t      PAUSA_INVERSAO_MS = 300;

// ---------- Parâmetros do sensor de cor ----------
const bool  DEBUG_COR        = false;  // true = imprime R,G,B,Clear de cada leitura (para calibrar)
const float FATOR_PRESENCA   = 1.4;    // peça presente se Clear ficar 1,4x maior (ou menor) que o da esteira vazia
const float LIMIAR_CROMA     = 0.04;   // variação mínima da proporção de cor (R, G ou B) em relação à lona
const uint16_t INTERVALO_COR_MS = 80;  // intervalo entre leituras do sensor de cor

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_154MS, TCS34725_GAIN_16X);

enum Cor { SEM_PECA, VERMELHO, VERDE, AZUL, OUTRA };

// ---------- Estados ----------
enum Estado { AGUARDANDO, AVANCANDO, EM_RE, AGUARDA_REMOCAO };
Estado estado = AGUARDANDO;
unsigned long inicioMs = 0;

// ---------- Controle da leitura de cor ----------
uint16_t baselineClear = 0;      // Clear com a esteira vazia
float baseRn = 0, baseGn = 0, baseBn = 0;   // proporção R,G,B da lona (esteira vazia)
unsigned long ultimaLeituraCor = 0;
Cor corAnterior = SEM_PECA;
uint8_t confirmacoes = 0;
bool corJaInformada = false;     // evita imprimir várias vezes a mesma peça
uint8_t leiturasSemPeca = 0;

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

// ===================== ULTRASSÔNICO =====================
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

bool objetoDetectado() {
  for (uint8_t i = 0; i < 3; i++) {
    if (lerDistanciaCm() > DIST_DETECCAO) return false;
    delay(15);
  }
  return true;
}

// ===================== INDUTIVO =====================
bool metalBruto() {
  return digitalRead(PIN_INDUTIVO) == LOW;
}

bool metalDetectado() {
  for (uint8_t i = 0; i < 3; i++) {
    if (!metalBruto()) return false;
    delay(10);
  }
  return true;
}

// ===================== SENSOR DE COR =====================
// Mede a cor da lona vazia (referência). As peças são comparadas com ela,
// assim a cor da lona deixa de atrapalhar a leitura.
void medirBaseline() {
  uint32_t somaR = 0, somaG = 0, somaB = 0, somaC = 0;
  uint16_t r, g, b, c;
  delay(200);
  for (uint8_t i = 0; i < 5; i++) {
    tcs.getRawData(&r, &g, &b, &c);
    somaR += r; somaG += g; somaB += b; somaC += c;
    delay(170);                       // um ciclo de integração (154 ms)
  }
  baselineClear = somaC / 5;
  if (somaC > 0) {
    baseRn = (float)somaR / somaC;
    baseGn = (float)somaG / somaC;
    baseBn = (float)somaB / somaC;
  }
  Serial.print(F("Lona vazia: Clear="));
  Serial.print(baselineClear);
  Serial.print(F(" R="));  Serial.print(baseRn, 2);
  Serial.print(F(" G="));  Serial.print(baseGn, 2);
  Serial.print(F(" B="));  Serial.println(baseBn, 2);
}

const __FlashStringHelper* nomeCor(Cor c) {
  switch (c) {
    case VERMELHO: return F("VERMELHO");
    case VERDE:    return F("VERDE");
    case AZUL:     return F("AZUL");
    default:       return F("COR NAO RECONHECIDA");
  }
}

Cor classificarCor(uint16_t r, uint16_t g, uint16_t b, uint16_t c) {
  if (c == 0 || baselineClear == 0) return SEM_PECA;

  // Proporção de cada canal e diferença em relação à lona vazia
  float rn = (float)r / c;
  float gn = (float)g / c;
  float bn = (float)b / c;
  float dr = rn - baseRn;
  float dg = gn - baseGn;
  float db = bn - baseBn;
  float razao = (float)c / baselineClear;

  if (DEBUG_COR) {
    Serial.print(F("   dR:")); Serial.print(dr, 3);
    Serial.print(F(" dG:"));   Serial.print(dg, 3);
    Serial.print(F(" dB:"));   Serial.print(db, 3);
    Serial.print(F(" Clear/base:")); Serial.println(razao, 2);
  }

  // Há peça? Clear mudou bastante OU a proporção de cor mudou em relação à lona
  bool presente = (razao > FATOR_PRESENCA) || (razao < 1.0 / FATOR_PRESENCA) ||
                  (fabs(dr) > LIMIAR_CROMA) || (fabs(dg) > LIMIAR_CROMA) ||
                  (fabs(db) > LIMIAR_CROMA);
  if (!presente) return SEM_PECA;

  // A lona é verde: vermelho e azul aparecem como aumento de R ou B
  if (dr > LIMIAR_CROMA && dr > db) return VERMELHO;
  if (db > LIMIAR_CROMA && db >= dr) return AZUL;
  // Sem aumento de R ou B e o verde não caiu: peça verde (parecida com a lona)
  if (dg > -LIMIAR_CROMA) return VERDE;
  return OUTRA;
}

void resetarLeituraCor() {
  corAnterior = SEM_PECA;
  confirmacoes = 0;
  corJaInformada = false;
  leiturasSemPeca = 0;
}

// Chamada a cada ciclo do loop enquanto a esteira avança
void verificarCor() {
  if (millis() - ultimaLeituraCor < INTERVALO_COR_MS) return;
  ultimaLeituraCor = millis();

  uint16_t r, g, b, c;
  tcs.getRawData(&r, &g, &b, &c);

  if (DEBUG_COR) {
    Serial.print(F("R:")); Serial.print(r);
    Serial.print(F(" G:")); Serial.print(g);
    Serial.print(F(" B:")); Serial.print(b);
    Serial.print(F(" C:")); Serial.println(c);
  }

  Cor cor = classificarCor(r, g, b, c);

  if (cor == SEM_PECA) {
    // Peça saiu da frente do sensor: libera para detectar a próxima
    if (++leiturasSemPeca >= 3) {
      corAnterior = SEM_PECA;
      confirmacoes = 0;
      corJaInformada = false;
    }
    return;
  }

  leiturasSemPeca = 0;
  if (corJaInformada) return;

  // Confirma a cor com 2 leituras iguais seguidas
  if (cor == corAnterior) confirmacoes++;
  else { corAnterior = cor; confirmacoes = 1; }

  if (confirmacoes >= 2) {
    Serial.print(F(">>> Cor detectada: "));
    Serial.println(nomeCor(cor));
    corJaInformada = true;
  }
}

// ===================== SETUP =====================
void scannerI2C() {
  Serial.println(F("Procurando dispositivos I2C..."));
  uint8_t achados = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print(F("  Encontrado: 0x"));
      if (addr < 16) Serial.print('0');
      Serial.print(addr, HEX);
      if (addr == 0x29) Serial.print(F(" (TCS34725)"));
      if (addr == 0x3C || addr == 0x3D) Serial.print(F(" (OLED)"));
      Serial.println();
      achados++;
    }
  }
  if (achados == 0) Serial.println(F("  NENHUM dispositivo I2C encontrado!"));
}

void setup() {
  Serial.begin(9600);
  Serial.println(F("ETAPA 3: iniciando..."));   // primeira mensagem, antes de qualquer sensor

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_INDUTIVO, INPUT_PULLUP);
  pinMode(PIN_MOTOR_A, OUTPUT);
  pinMode(PIN_MOTOR_B, OUTPUT);
  esteiraParar();

  Wire.begin();
#ifdef WIRE_HAS_TIMEOUT
  Wire.setWireTimeout(3000, true);   // evita travar se o barramento I2C ficar preso
#endif
  scannerI2C();

  Serial.println(F("Iniciando TCS34725..."));
  if (!tcs.begin()) {
    while (true) {
      Serial.println(F("ERRO: TCS34725 nao encontrado! Confira SDA/SCL, 5V e GND."));
      delay(2000);
    }
  }

  Serial.println(F("TCS34725 OK. Medindo referencia da esteira vazia..."));
  medirBaseline();
  Serial.println(F("Aguardando objeto..."));
}

// ===================== LOOP =====================
void loop() {
  switch (estado) {

    case AGUARDANDO:
      if (objetoDetectado()) {
        Serial.println(F("Objeto detectado! Esteira AVANCANDO (max 60 s)"));
        resetarLeituraCor();
        esteiraAvancar();
        inicioMs = millis();
        estado = AVANCANDO;
      }
      break;

    case AVANCANDO: {
      // 1) Metal?
      if (metalDetectado()) {
        Serial.println(F("METAL detectado! Parando e invertendo por 10 s"));
        esteiraParar();
        delay(PAUSA_INVERSAO_MS);
        esteiraRe();
        inicioMs = millis();
        estado = EM_RE;
        break;
      }

      // 2) Cor?
      verificarCor();

      // 3) Tempo de avanço
      unsigned long decorrido = millis() - inicioMs;
      if (decorrido >= TEMPO_AVANCO_MS) {
        esteiraParar();
        Serial.println(F("Tempo concluido, esteira DESLIGADA."));
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
      if (lerDistanciaCm() > DIST_DETECCAO && !metalBruto()) {
        delay(500);
        medirBaseline();              // nova referência com a esteira vazia
        Serial.println(F("Rearmado. Aguardando objeto..."));
        estado = AGUARDANDO;
      }
      break;
  }

  delay(5);
}
