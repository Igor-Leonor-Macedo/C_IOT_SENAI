/*
 * ETAPA 6 - Etapa 5 + 2 servos motores
 *
 * Servos (movimento de 90° apenas):
 *      - Cor VERMELHA reconhecida -> Servo 1 (D7) aciona (gira -90°)
 *      - Cor AZUL reconhecida     -> Servo 2 (D8) aciona (gira +90°)
 *      - Depois de um tempo o servo volta para a posição de repouso (0°)
 *
 * (Funções das etapas anteriores:)
 * 0. Botão de retenção (liga/desliga) e LEDs indicam o estado do sistema:
 *      - OFF: esteira parada, sensores ignorados, OLED mostra "OFF", LED vermelho aceso
 *      - ON : OLED mostra "ON", LED verde aceso e o ciclo abaixo funciona
 * 1. O HC-SR04 detecta um objeto -> esteira AVANÇA.
 * 2. Enquanto avança:
 *      - Indutivo detectou METAL -> mostra "METAL" no OLED e gira para TRÁS por 10 s
 *      - Sensor de cor reconheceu VERMELHO / VERDE / AZUL -> mostra o nome da cor no OLED
 *      - Nada de metal -> esteira avança por 1 minuto e desliga
 * 3. Ao terminar, aguarda a retirada da peça para rearmar.
 *
 * Pinagem (igual ao programa principal):
 *   HC-SR04  : TRIG = D9, ECHO = D10
 *   Indutivo : sinal = D2 (INPUT_PULLUP, NPN: LOW = metal)
 *   HW-095   : IN1 = D5 (PWM), IN2 = D6 (PWM)
 *   TCS34725 : SDA = A4, SCL = A5, 5V, GND
 *   OLED SH1106 : SDA = A4, SCL = A5, 5V, GND (mesmo barramento I2C do TCS34725)
 *   Servo 1 (vermelho): D7  | Servo 2 (azul): D8  (alimentar com 5 V externo, GND comum)
 *   Botão ON/OFF: D3 e GND (INPUT_PULLUP: fechado = ON)
 *   LED ON (verde)    : D4  -> resistor 220 ohm -> LED -> GND
 *   LED OFF (vermelho): D12 -> resistor 220 ohm -> LED -> GND
 *
 * Bibliotecas: Adafruit TCS34725 (+ Adafruit BusIO) e U8g2
 * Monitor Serial: 9600 baud
 */

#include <Wire.h>
#include <Adafruit_TCS34725.h>
#include <U8g2lib.h>
#include <Servo.h>

// ---------- Pinos ----------
const uint8_t PIN_TRIG     = 9;
const uint8_t PIN_ECHO     = 10;
const uint8_t PIN_INDUTIVO = 2;
const uint8_t PIN_MOTOR_A  = 5;   // IN1
const uint8_t PIN_MOTOR_B  = 6;   // IN2
const uint8_t PIN_BOTAO    = 3;   // botão de retenção (outro lado em GND)
const uint8_t PIN_LED_ON   = 4;   // LED verde
const uint8_t PIN_LED_OFF  = 12;  // LED vermelho
const uint8_t PIN_SERVO1   = 7;   // aciona com VERMELHO
const uint8_t PIN_SERVO2   = 8;   // aciona com AZUL

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

// Modo "page buffer" (_1_): usa pouca RAM
// Se a tela não acender, troque por U8G2_SSD1306_128X64_NONAME_1_HW_I2C
U8G2_SH1106_128X64_NONAME_1_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);

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

// ===================== SERVOS =====================
// Ângulos de cada servo: [0] = Servo 1 (D7), [1] = Servo 2 (D8)
// Servo 1: repouso em 90° e vai a 0°  -> gira -90°
// Servo 2: repouso em 0°  e vai a 90° -> gira +90°
const uint8_t       ANGULO_REPOUSO[2]      = {90, 0};
const uint8_t       ANGULO_ACIONADO[2]     = {0, 90};
const unsigned long ATRASO_SERVO_MS        = 0;     // tempo entre reconhecer a cor e acionar (distância sensor -> servo)
const unsigned long TEMPO_SERVO_ACIONADO_MS = 800;  // tempo que o servo fica em 90° antes de voltar

Servo servo1, servo2;
uint8_t faseServo[2] = {0, 0};            // 0 = repouso, 1 = aguardando atraso, 2 = acionado
unsigned long tServo[2] = {0, 0};

// i = 0 -> servo 1 | i = 1 -> servo 2
void agendarServo(uint8_t i) {
  if (faseServo[i] != 0) return;          // já em ação
  faseServo[i] = 1;
  tServo[i] = millis() + ATRASO_SERVO_MS;
}

// Chamada a cada ciclo do loop (não bloqueia)
void atualizarServos() {
  for (uint8_t i = 0; i < 2; i++) {
    Servo &sv = (i == 0) ? servo1 : servo2;
    if (faseServo[i] == 1 && (long)(millis() - tServo[i]) >= 0) {
      sv.write(ANGULO_ACIONADO[i]);
      faseServo[i] = 2;
      tServo[i] = millis() + TEMPO_SERVO_ACIONADO_MS;
      Serial.print(F("Servo "));
      Serial.print(i + 1);
      Serial.println(F(" ACIONADO"));
    } else if (faseServo[i] == 2 && (long)(millis() - tServo[i]) >= 0) {
      sv.write(ANGULO_REPOUSO[i]);
      faseServo[i] = 0;
      Serial.print(F("Servo "));
      Serial.print(i + 1);
      Serial.println(F(" voltou ao repouso"));
    }
  }
}

void pararServos() {
  servo1.write(ANGULO_REPOUSO[0]);
  servo2.write(ANGULO_REPOUSO[1]);
  faseServo[0] = faseServo[1] = 0;
}

// ===================== OLED =====================
// topo = linha pequena no alto | grande = mensagem principal | base = linha pequena embaixo
void mostrar(const char* topo, const char* grande, const char* base) {
  display.firstPage();
  do {
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(0, 9, topo);
    display.drawHLine(0, 12, 128);

    display.setFont(u8g2_font_helvB18_tr);
    uint8_t x = (128 - display.getStrWidth(grande)) / 2;
    display.drawStr(x, 44, grande);

    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(0, 62, base);
  } while (display.nextPage());
}

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

const char* textoCor(Cor c) {
  switch (c) {
    case VERMELHO: return "RED";
    case VERDE:    return "GREEN";
    case AZUL:     return "BLUE";
    default:       return "UNKNOWN";
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
    if (cor == OUTRA) {
      mostrar("Cor nao identificada", textoCor(cor), "Esteira avancando");
    } else if (cor == VERMELHO) {
      mostrar("Cor detectada", textoCor(cor), "Servo 1 acionado");
      agendarServo(0);
    } else if (cor == AZUL) {
      mostrar("Cor detectada", textoCor(cor), "Servo 2 acionado");
      agendarServo(1);
    } else {
      mostrar("Cor detectada", textoCor(cor), "Esteira avancando");
    }
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

// ===================== BOTÃO ON/OFF =====================
int8_t sistemaLigado = -1;   // -1 = ainda não definido (força a atualização ao iniciar)

// Botão de retenção com filtro anti-ruído de 50 ms. Fechado (LOW) = ligado.
bool botaoLigado() {
  static bool estavel = false;
  static bool ultimo = false;
  static unsigned long tMudanca = 0;

  bool leitura = (digitalRead(PIN_BOTAO) == LOW);
  if (leitura != ultimo) {
    ultimo = leitura;
    tMudanca = millis();
  }
  if (millis() - tMudanca >= 50) estavel = leitura;
  return estavel;
}

void aplicarEstadoSistema(bool ligado) {
  esteiraParar();
  pararServos();
  estado = AGUARDANDO;
  resetarLeituraCor();

  if (ligado) {
    digitalWrite(PIN_LED_ON, HIGH);
    digitalWrite(PIN_LED_OFF, LOW);
    Serial.println(F("Sistema ON. Medindo referencia da esteira vazia..."));
    mostrar("Sistema", "ON", "Calibrando lona...");
    medirBaseline();
    mostrar("Sistema", "ON", "Coloque a peca");
    Serial.println(F("Aguardando objeto..."));
  } else {
    digitalWrite(PIN_LED_ON, LOW);
    digitalWrite(PIN_LED_OFF, HIGH);
    Serial.println(F("Sistema OFF. Esteira parada."));
    mostrar("Sistema", "OFF", "Desligado");
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println(F("ETAPA 6: iniciando..."));   // primeira mensagem, antes de qualquer sensor

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_INDUTIVO, INPUT_PULLUP);
  pinMode(PIN_MOTOR_A, OUTPUT);
  pinMode(PIN_MOTOR_B, OUTPUT);
  pinMode(PIN_BOTAO, INPUT_PULLUP);
  pinMode(PIN_LED_ON, OUTPUT);
  pinMode(PIN_LED_OFF, OUTPUT);
  digitalWrite(PIN_LED_ON, LOW);
  digitalWrite(PIN_LED_OFF, LOW);
  esteiraParar();

  servo1.attach(PIN_SERVO1);
  servo2.attach(PIN_SERVO2);
  servo1.write(ANGULO_REPOUSO[0]);
  servo2.write(ANGULO_REPOUSO[1]);

  Wire.begin();
#ifdef WIRE_HAS_TIMEOUT
  Wire.setWireTimeout(3000, true);   // evita travar se o barramento I2C ficar preso
#endif
  display.begin();
  mostrar("ETAPA 6", "INICIANDO", "Verificando I2C");
  scannerI2C();

  Serial.println(F("Iniciando TCS34725..."));
  if (!tcs.begin()) {
    while (true) {
      mostrar("ERRO", "SENSOR COR", "Confira SDA/SCL");
      Serial.println(F("ERRO: TCS34725 nao encontrado! Confira SDA/SCL, 5V e GND."));
      delay(2000);
    }
  }

  Serial.println(F("TCS34725 OK."));

  // Lê a posição do botão ao ligar (a calibração acontece ao entrar em ON)
  botaoLigado();
  delay(60);
  botaoLigado();
}

// ===================== LOOP =====================
void loop() {
  // ---- Botão ON/OFF ----
  bool ligado = botaoLigado();
  if ((int8_t)ligado != sistemaLigado) {
    sistemaLigado = (int8_t)ligado;
    aplicarEstadoSistema(ligado);
  }
  if (!ligado) {          // sistema desligado: não faz mais nada
    delay(50);
    return;
  }

  atualizarServos();      // movimenta os servos sem travar o restante do programa

  switch (estado) {

    case AGUARDANDO:
      if (objetoDetectado()) {
        Serial.println(F("Objeto detectado! Esteira AVANCANDO (max 60 s)"));
        mostrar("Objeto detectado", "AVANCANDO", "Esteira ligada");
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
        mostrar("Sensor indutivo", "METAL", "Esteira em re 10 s");
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
        mostrar("Tempo concluido", "PARADA", "Retire a peca");
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
        mostrar("Re concluida", "METAL", "Retire a peca");
        Serial.println(F("Retire a peca de metal para rearmar."));
        estado = AGUARDA_REMOCAO;
      }
      break;

    case AGUARDA_REMOCAO:
      if (lerDistanciaCm() > DIST_DETECCAO && !metalBruto()) {
        delay(500);
        medirBaseline();              // nova referência com a esteira vazia
        Serial.println(F("Rearmado. Aguardando objeto..."));
        mostrar("Pronto", "AGUARDANDO", "Coloque a peca");
        estado = AGUARDANDO;
      }
      break;
  }

  delay(5);
}
