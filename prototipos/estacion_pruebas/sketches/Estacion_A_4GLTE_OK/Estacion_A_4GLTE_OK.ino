#include <SoftwareSerial.h>
#include <Wire.h>
#include <BH1750.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <string.h>

// --------------------------------------------------
// Módem LTE
// --------------------------------------------------
SoftwareSerial modem(10, 11); // RX Arduino <- TX módem, TX Arduino -> RX módem

// --------------------------------------------------
// Sensores
// --------------------------------------------------
BH1750 lightMeter;
Adafruit_BME280 bme;

// --------------------------------------------------
// Configuración general
// --------------------------------------------------
const char APN[] = "internet";
const char THINGSPEAK_API_KEY[] = "REEMPLAZAR_POR_WRITE_API_KEY";

const int SOIL_PIN = A0;
const int RELAY_PIN = 7;

const int SOIL_DRY = 558;
const int SOIL_WET = 211;

const byte RELAY_ON = HIGH;
const byte RELAY_OFF = LOW;

const unsigned long SEND_INTERVAL_MS = 10000UL;
const unsigned long NETWORK_RETRY_MS = 5000UL;
const unsigned long LTE_WAIT_MAX_MS = 300000UL;

// --------------------------------------------------
// Estado
// --------------------------------------------------
bool mobileDataReady = false;
long modemBaudDetected = -1;

// --------------------------------------------------
// Utilidades serie del módem
// --------------------------------------------------
void clearModemBuffer() {
  while (modem.available()) {
    modem.read();
  }
}

bool readReply(char *buffer, size_t bufferSize, unsigned long timeoutMs) {
  unsigned long start = millis();
  size_t idx = 0;

  if (bufferSize == 0) return false;
  buffer[0] = '\0';

  while (millis() - start < timeoutMs) {
    while (modem.available()) {
      char c = modem.read();
      if (idx < bufferSize - 1) {
        buffer[idx++] = c;
        buffer[idx] = '\0';
      }
    }
  }

  return idx > 0;
}

bool sendCommandOK(const char *cmd, unsigned long timeoutMs) {
  char resp[160];
  clearModemBuffer();
  modem.println(cmd);
  readReply(resp, sizeof(resp), timeoutMs);
  return strstr(resp, "OK") != nullptr;
}

bool sendCommandAny(const char *cmd, unsigned long timeoutMs) {
  char resp[160];
  clearModemBuffer();
  modem.println(cmd);
  readReply(resp, sizeof(resp), timeoutMs);
  return (strstr(resp, "OK") != nullptr) || (strstr(resp, "ERROR") != nullptr);
}

// --------------------------------------------------
// Detección de baudios
// --------------------------------------------------
bool probeBaud(long baud) {
  char resp[80];

  modem.end();
  delay(250);
  modem.begin(baud);
  delay(800);

  clearModemBuffer();

  for (int i = 0; i < 3; i++) {
    modem.println("AT");
    delay(150);
  }

  readReply(resp, sizeof(resp), 2000);
  return strstr(resp, "OK") != nullptr;
}

long detectModemBaud() {
  const long bauds[] = {9600, 115200, 57600, 38400, 19200};

  for (unsigned int i = 0; i < sizeof(bauds) / sizeof(bauds[0]); i++) {
    Serial.print(F("Probando baudios: "));
    Serial.println(bauds[i]);

    if (probeBaud(bauds[i])) {
      Serial.print(F("RESPONDE EN "));
      Serial.println(bauds[i]);
      return bauds[i];
    }
  }

  return -1;
}

// --------------------------------------------------
// Fijar 9600 solo si hace falta
// --------------------------------------------------
bool setModem9600IfNeeded(long detectedBaud) {
  if (detectedBaud < 0) return false;

  modem.end();
  delay(250);
  modem.begin(detectedBaud);
  delay(900);

  if (detectedBaud == 9600) {
    Serial.println(F("Modem ya estaba en 9600."));
    return true;
  }

  Serial.println(F("Cambiando modem a 9600..."));

  if (!sendCommandOK("AT", 2500)) return false;
  if (!sendCommandOK("AT+IPR=9600", 2500)) return false;

  delay(1000);

  modem.end();
  delay(250);
  modem.begin(9600);
  delay(1200);

  if (!sendCommandOK("AT", 2500)) return false;

  Serial.println(F("Modem cambiado a 9600."));
  return true;
}

// --------------------------------------------------
// Red LTE
// --------------------------------------------------
bool isNetworkRegistered() {
  char resp[120];

  clearModemBuffer();
  modem.println("AT+CEREG?");
  readReply(resp, sizeof(resp), 3000);

  return (strstr(resp, "+CEREG: 0,1") != nullptr) ||
         (strstr(resp, "+CEREG: 0,5") != nullptr);
}

bool waitForNetworkReady(unsigned long maxWaitMs) {
  unsigned long start = millis();

  Serial.println(F("Esperando registro LTE..."));

  while (millis() - start < maxWaitMs) {
    if (isNetworkRegistered()) {
      Serial.println(F("LTE listo."));
      return true;
    }

    Serial.println(F("LTE aun no listo. Reintentando..."));
    delay(NETWORK_RETRY_MS);
  }

  Serial.println(F("Tiempo de espera LTE agotado."));
  return false;
}

bool configureMobileData() {
  char cmdApn[64];
  snprintf(cmdApn, sizeof(cmdApn), "AT+CGDCONT=1,\"IP\",\"%s\"", APN);

  if (!sendCommandOK(cmdApn, 4000)) return false;
  if (!sendCommandOK("AT+CGATT=1", 8000)) return false;
  if (!sendCommandOK("AT+CGACT=1,1", 12000)) return false;

  clearModemBuffer();
  modem.println("AT+CGPADDR=1");
  delay(800);

  mobileDataReady = true;
  return true;
}

// --------------------------------------------------
// Lectura de sensores
// --------------------------------------------------
int readSoilRaw() {
  long sum = 0;

  for (int i = 0; i < 10; i++) {
    sum += analogRead(SOIL_PIN);
    delay(10);
  }

  return sum / 10;
}

int soilRawToPercent(int raw) {
  int percent = map(raw, SOIL_DRY, SOIL_WET, 0, 100);
  if (percent < 0) percent = 0;
  if (percent > 100) percent = 100;
  return percent;
}

void relayPulse2s() {
  digitalWrite(RELAY_PIN, RELAY_ON);
  delay(2000);
  digitalWrite(RELAY_PIN, RELAY_OFF);
}

// --------------------------------------------------
// Envío a ThingSpeak
// field1 = suelo
// field2 = temperatura
// field3 = humedad
// field4 = lux
// field5 = presión
// --------------------------------------------------
bool sendThingSpeak(int soilPercent, float tempC, float humPct, int lux, int pressureHpa) {
  char tempStr[10];
  char humStr[10];
  char url[200];
  char cmdUrl[240];
  char resp[160];

  dtostrf(tempC, 0, 1, tempStr);
  dtostrf(humPct, 0, 1, humStr);

  snprintf(
    url,
    sizeof(url),
    "http://api.thingspeak.com/update?api_key=%s&field1=%d&field2=%s&field3=%s&field4=%d&field5=%d",
    THINGSPEAK_API_KEY,
    soilPercent,
    tempStr,
    humStr,
    lux,
    pressureHpa
  );

  clearModemBuffer();
  modem.println("AT+HTTPTERM");
  delay(150);

  if (!sendCommandOK("AT+HTTPINIT", 2500)) {
    return false;
  }

  snprintf(cmdUrl, sizeof(cmdUrl), "AT+HTTPPARA=\"URL\",\"%s\"", url);

  if (!sendCommandOK(cmdUrl, 4000)) {
    sendCommandAny("AT+HTTPTERM", 1500);
    return false;
  }

  clearModemBuffer();
  modem.println("AT+HTTPACTION=0");
  readReply(resp, sizeof(resp), 5000);

  bool httpOk = (strstr(resp, "+HTTPACTION: 0,200") != nullptr);

  sendCommandAny("AT+HTTPTERM", 1500);
  return httpOk;
}

// --------------------------------------------------
// Setup
// --------------------------------------------------
void setup() {
  Serial.begin(9600);
  delay(2500);

  Serial.println(F("=== TierraViva RAM ahorro - version final ==="));

  pinMode(SOIL_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF);

  Wire.begin();

  bool bmeOk = bme.begin(0x76);
  if (!bmeOk) bmeOk = bme.begin(0x77);

  lightMeter.begin();

  Serial.println(F("Sensores I2C listos."));
  Serial.println(bmeOk ? F("BME280 OK") : F("BME280 ERROR"));
  Serial.println(F("BH1750 OK"));

  Serial.println(F("Detectando baudios del modem..."));
  modemBaudDetected = detectModemBaud();

  if (modemBaudDetected < 0) {
    Serial.println(F("No se detecta el modem."));
    return;
  }

  if (!setModem9600IfNeeded(modemBaudDetected)) {
    Serial.println(F("No se pudo preparar el modem a 9600."));
    return;
  }

  if (sendCommandOK("AT", 2500)) {
    Serial.println(F("Modem OK"));
  } else {
    Serial.println(F("Modem ERROR"));
  }

  sendCommandAny("AT+CFUN=1", 4000);

  if (waitForNetworkReady(LTE_WAIT_MAX_MS)) {
    if (configureMobileData()) {
      Serial.println(F("Datos moviles OK"));
    } else {
      Serial.println(F("Datos moviles ERROR"));
    }
  }

  Serial.println(F("Sistema listo"));
}

// --------------------------------------------------
// Loop principal
// --------------------------------------------------
void loop() {
  if (!mobileDataReady) {
    Serial.println(F("Red no preparada. Reintentando..."));

    if (waitForNetworkReady(LTE_WAIT_MAX_MS)) {
      if (configureMobileData()) {
        Serial.println(F("Datos moviles OK"));
      }
    }

    delay(NETWORK_RETRY_MS);
    return;
  }

  int soilRaw = readSoilRaw();
  int soilPercent = soilRawToPercent(soilRaw);

  float tempC = bme.readTemperature();
  float humPct = bme.readHumidity();
  int lux = (int)(lightMeter.readLightLevel() + 0.5);
  int pressureHpa = (int)(bme.readPressure() / 100.0F + 0.5);

  Serial.println(F("\n--- Lecturas ---"));
  Serial.print(F("Suelo bruto: "));
  Serial.println(soilRaw);
  Serial.print(F("Suelo %: "));
  Serial.println(soilPercent);
  Serial.print(F("Temp C: "));
  Serial.println(tempC, 1);
  Serial.print(F("Humedad %: "));
  Serial.println(humPct, 1);
  Serial.print(F("Lux: "));
  Serial.println(lux);
  Serial.print(F("Presion hPa: "));
  Serial.println(pressureHpa);

  if (sendThingSpeak(soilPercent, tempC, humPct, lux, pressureHpa)) {
    Serial.println(F("ThingSpeak OK"));
    relayPulse2s();
  } else {
    Serial.println(F("ThingSpeak ERROR"));
    mobileDataReady = false;
  }

  Serial.println(F("Esperando 30 segundos..."));
  delay(SEND_INTERVAL_MS);
}