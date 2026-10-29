#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// ---------- Pinos ----------
#define PIN_GAS      A0
#define PIN_CHAMA    2
#define PIN_DHT      3
#define PIN_RELE     7
#define PIN_BUZZER   8
#define PIN_LED_VERDE 9
#define PIN_LED_VERM  10

// ---------- Tipo do DHT ----------
#define TIPO_DHT DHT22     // troque para DHT11 se for o seu caso

// ---------- Limites (ajuste após calibrar) ----------
#define LIMITE_GAS        400    // leitura analógica 0-1023
#define LIMITE_TEMP       45.0   // °C
#define LIMITE_UMIDADE    30.0   // % (umidade baixa + calor = risco)

// Lógica do sensor de chama: a maioria dos módulos dá LOW quando detecta
#define CHAMA_DETECTADA   HIGH

// Lógica do relé: a maioria dos módulos é ativa em LOW
#define RELE_ON   LOW
#define RELE_OFF  HIGH

// Buzzer ativo em LOW (HIGH = silêncio)
#define BUZZER_ON   LOW
#define BUZZER_OFF  HIGH

// Tempo de aquecimento do MQ-2 (ms)
#define TEMPO_AQUECIMENTO 20000UL

// Biblioteca Marco Schwartz (pinagem padrão da maioria dos módulos I2C PCF8574)
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

DHT dht(PIN_DHT, TIPO_DHT);

float temperatura = 0;
float umidade = 0;
int   nivelGas = 0;
bool  chama = false;

unsigned long ultimaLeituraDHT = 0;
unsigned long ultimoPiscar = 0;
bool estadoPiscar = false;

void setup() {
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, BUZZER_OFF);   // silêncio já na partida
  
  Serial.begin(9600);
  pinMode(PIN_CHAMA, INPUT);
  pinMode(PIN_RELE, OUTPUT);
  pinMode(PIN_LED_VERDE, OUTPUT);
  pinMode(PIN_LED_VERM, OUTPUT);

  digitalWrite(PIN_RELE, RELE_OFF);

  lcd.begin(16, 2);
  lcd.setBacklight(HIGH);
  dht.begin();

  // Aquecimento do sensor de gás
  lcd.setCursor(0, 0);
  lcd.print("Aquecendo MQ-2..");
  unsigned long inicio = millis();
  while (millis() - inicio < TEMPO_AQUECIMENTO) {
    lcd.setCursor(0, 1);
    lcd.print("Aguarde ");
    lcd.print((TEMPO_AQUECIMENTO - (millis() - inicio)) / 1000);
    lcd.print("s  ");
    delay(200);
  }
  lcd.clear();
}

void loop() {
  lerSensores();

  // ---------- Avaliação de risco ----------
  bool alertaGas   = (nivelGas >= LIMITE_GAS);
  bool alertaChama = chama;
  bool alertaTemp  = (temperatura >= LIMITE_TEMP) ||
                     (temperatura >= (LIMITE_TEMP - 10) && umidade <= LIMITE_UMIDADE);

  // Nível de severidade = quantos sensores dispararam
  int severidade = alertaGas + alertaChama + alertaTemp;

  // Chama sozinha já é emergência; caso contrário, 2+ sensores
  bool emergencia = alertaChama || (severidade >= 2);
  bool atencao    = (severidade == 1) && !emergencia;

  if (emergencia) {
    acionarAlarme();
  } else if (atencao) {
    acionarAtencao();
  } else {
    estadoSeguro();
  }

  atualizarDisplay(emergencia, atencao);
  imprimirSerial(alertaGas, alertaChama, alertaTemp);

  delay(100);
}

// ======================= FUNÇÕES =======================

void lerSensores() {
  nivelGas = analogRead(PIN_GAS);
  chama = (digitalRead(PIN_CHAMA) == CHAMA_DETECTADA);

  // DHT só pode ser lido a cada ~2 s
  if (millis() - ultimaLeituraDHT >= 2000) {
    ultimaLeituraDHT = millis();
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t)) temperatura = t;
    if (!isnan(h)) umidade = h;
  }
}

void estadoSeguro() {
  digitalWrite(PIN_LED_VERDE, HIGH);
  digitalWrite(PIN_LED_VERM, LOW);
  digitalWrite(PIN_BUZZER, BUZZER_OFF);
  digitalWrite(PIN_RELE, RELE_OFF);
}

void acionarAtencao() {
  // LED vermelho e buzzer pulsando devagar, relé desligado
  digitalWrite(PIN_LED_VERDE, LOW);

  if (millis() - ultimoPiscar >= 500) {
    ultimoPiscar = millis();
    estadoPiscar = !estadoPiscar;
    digitalWrite(PIN_LED_VERM, estadoPiscar);
    digitalWrite(PIN_BUZZER, estadoPiscar ? BUZZER_ON : BUZZER_OFF);
  }
  digitalWrite(PIN_RELE, RELE_OFF);
}

void acionarAlarme() {
  // LED vermelho e buzzer pulsando rápido, relé ligado
  digitalWrite(PIN_LED_VERDE, LOW);

  if (millis() - ultimoPiscar >= 150) {
    ultimoPiscar = millis();
    estadoPiscar = !estadoPiscar;
    digitalWrite(PIN_LED_VERM, estadoPiscar);
    digitalWrite(PIN_BUZZER, estadoPiscar ? BUZZER_ON : BUZZER_OFF);
  }
  digitalWrite(PIN_RELE, RELE_ON);
}

void atualizarDisplay(bool emergencia, bool atencao) {
  lcd.setCursor(0, 0);
  lcd.print("G:");
  lcd.print(nivelGas);
  lcd.print("   ");
  lcd.setCursor(7, 0);
  lcd.print("T:");
  lcd.print(temperatura, 0);
  lcd.print((char)223);
  lcd.print("C ");
  lcd.print((int)umidade);
  lcd.print("% ");

  lcd.setCursor(0, 1);
  if (emergencia) {
    lcd.print(chama ? "!! CHAMA / FOGO !!" : "!! PERIGO !!      ");
  } else if (atencao) {
    lcd.print("ATENCAO          ");
  } else {
    lcd.print("Ambiente seguro  ");
  }
}

void imprimirSerial(bool g, bool c, bool t) {
  static unsigned long ultimoSerial = 0;
  if (millis() - ultimoSerial < 1000) return;
  ultimoSerial = millis();

  Serial.print("Gas: ");   Serial.print(nivelGas);
  Serial.print(" | Chama: "); Serial.print(c ? "SIM" : "NAO");
  Serial.print(" | Temp: "); Serial.print(temperatura);
  Serial.print(" | Umid: "); Serial.println(umidade);
}
