#include <SoftwareSerial.h>
#include <Wire.h>
#include <BH1750.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <string.h>

SoftwareSerial modem(10, 11); // RX Arduino <- TX módem, TX Arduino -> RX módem
BH1750 lightMeter;
Adafruit_BME280 bme;

const char APN[] = "internet";
const char THINGSPEAK_API_KEY[] = "****************";
const int SOIL_PIN = A0;
const int RELAY_PIN = 7;
const int SOIL_DRY = 558;
const int SOIL_WET = 211;
const byte RELAY_ON = LOW;
const byte RELAY_OFF = HIGH;
const unsigned long SEND_INTERVAL_MS = 10000UL;
const unsigned long NETWORK_RETRY_MS = 5000UL;
const unsigned long LTE_WAIT_MAX_MS = 30000UL;
const unsigned long IRRIGATION_SECONDS = 10UL;
// const unsigned long COOLDOWN_MS = 30UL * 60UL * 1000UL; // 30 min
const unsigned long COOLDOWN_MS = 60UL * 1000UL; // 60 segundos
const int NORMAL_SOIL_THRESHOLD = 30;
const int SAFE_SOIL_THRESHOLD = 15;
const byte SAFE_EXIT_VALID_CYCLES = 3;
const byte NORMAL_CONFIRM_DRY_CYCLES = 2;
const byte SAFE_CONFIRM_DRY_CYCLES = 1;

bool mobileDataReady = false;
long modemBaudDetected = -1;

bool bmeOk = false;
bool sensorOk = true;

bool safeMode = true;
byte validCycleCount = 0;
byte dryCycleCount = 0;

unsigned long lastIrrigationEndMs = 0;

int soilRawValue = 0;
int soilPercent = 0;
float temperatureC = 0.0;
float humidityPct = 0.0;
int luxValue = 0;
int pressureHpa = 0;

bool relayActive = false;

// Field 7: estado del sistema / evento
int systemEventCode = 100;   // 100=idle, 206=safe mode, 210=riego ON, 211=riego OFF, 400=error, 429=cooldown

// Field 8: debug / código de estado
int debugCode = 204;         // 200=OK, 204=no riego, 206=modo seguro, 400=error, 429=cooldown

// Buffers globales
char tsTemp[8];
char tsHum[8];
char tsUrl[180];
char httpResp[128];

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

bool sendATCommand(const char* cmd, char* response, size_t size, unsigned long timeoutMs) {
  clearModemBuffer();
  modem.println(cmd);
  return readReply(response, size, timeoutMs);
}

bool probeBaud(long baud) {
  char resp[80];

  modem.end();
  delay(250);
  modem.begin(baud);
  delay(800);

  clearModemBuffer();

  for (int i = 0; i < 3; i++) {
    modem.println(F("AT"));
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

bool isNetworkRegistered() {
  char resp[120];

  clearModemBuffer();
  modem.println(F("AT+CEREG?"));
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
  modem.println(F("AT+CGPADDR=1"));
  delay(800);

  mobileDataReady = true;
  return true;
}

int csqToDbm(int csq) {
  if (csq == 99) return -999;   // desconocido
  if (csq < 0 || csq > 31) return -999;
  return -113 + (2 * csq);
}

void printSignalPretty() {
  char resp[160];
  int csq = -1;
  int rssiDbm = -999;

  Serial.println(F("Consultando calidad de señal..."));
  if (sendATCommand("AT+CSQ", resp, sizeof(resp), 3000)) {
    Serial.print(F("CSQ raw: "));
    Serial.println(resp);

    char *p = strstr(resp, "+CSQ:");
    if (p) {
      int rx = -1, ber = -1;
      if (sscanf(p, "+CSQ: %d,%d", &rx, &ber) == 2) {
        csq = rx;
        rssiDbm = csqToDbm(csq);

        Serial.print(F("CSQ: "));
        Serial.println(csq);

        if (rssiDbm != -999) {
          Serial.print(F("RSSI aprox: "));
          Serial.print(rssiDbm);
          Serial.println(F(" dBm"));
        } else {
          Serial.println(F("RSSI: desconocido"));
        }
      } else {
        Serial.println(F("No se pudo parsear CSQ"));
      }
    } else {
      Serial.println(F("No se encontro +CSQ"));
    }
  } else {
    Serial.println(F("No se pudo leer CSQ"));
  }
}

void printNetworkInfo() {
  char resp[160];

  Serial.println(F("Consultando operador LTE..."));
  if (sendATCommand("AT+COPS?", resp, sizeof(resp), 3000)) {
    Serial.print(F("COPS raw: "));
    Serial.println(resp);
  } else {
    Serial.println(F("No se pudo leer COPS"));
  }

  printSignalPretty();
}

int readSoilRaw() {
  long sum = 0;

  for (int i = 0; i < 10; i++) {
    sum += analogRead(SOIL_PIN);
    delay(10);
  }

  return (int)(sum / 10L);
}

int soilRawToPercent(int raw) {
  int percent = map(raw, SOIL_DRY, SOIL_WET, 0, 100);
  if (percent < 0) percent = 0;
  if (percent > 100) percent = 100;
  return percent;
}

void readSensors() {
  sensorOk = true;

  soilRawValue = readSoilRaw();
  soilPercent = soilRawToPercent(soilRawValue);

  float t = bme.readTemperature();
  float h = bme.readHumidity();
  float p = bme.readPressure();
  float l = lightMeter.readLightLevel();

  if (isnan(t) || isnan(h) || isnan(p) || l < 0) {
    sensorOk = false;
  } else {
    temperatureC = t;
    humidityPct = h;
    pressureHpa = (int)(p / 100.0F + 0.5);
    luxValue = (int)(l + 0.5);
  }

  if (!sensorOk) {
    temperatureC = 0.0;
    humidityPct = 0.0;
    pressureHpa = 0;
    luxValue = 0;
  }

  Serial.println(F("\n--- Lecturas ---"));
  Serial.print(F("Suelo bruto: "));
  Serial.println(soilRawValue);
  Serial.print(F("Suelo %: "));
  Serial.println(soilPercent);
  Serial.print(F("Temp C: "));
  Serial.println(temperatureC, 1);
  Serial.print(F("Humedad %: "));
  Serial.println(humidityPct, 1);
  Serial.print(F("Lux: "));
  Serial.println(luxValue);
  Serial.print(F("Presion hPa: "));
  Serial.println(pressureHpa);
}

bool cooldownActive() {
  if (lastIrrigationEndMs == 0) return false;
  return (millis() - lastIrrigationEndMs) < COOLDOWN_MS;
}

void updateSafetyState() {
  if (!sensorOk) {
    safeMode = true;
    validCycleCount = 0;
    dryCycleCount = 0;
    return;
  }

  validCycleCount++;

  if (safeMode && validCycleCount >= SAFE_EXIT_VALID_CYCLES) {
    safeMode = false;
    dryCycleCount = 0;
  }

  int threshold = safeMode ? SAFE_SOIL_THRESHOLD : NORMAL_SOIL_THRESHOLD;

  if (soilPercent <= threshold) {
    if (dryCycleCount < 255) dryCycleCount++;
  } else {
    dryCycleCount = 0;
  }
}

bool shouldIrrigate() {
  if (!sensorOk) {
    systemEventCode = 400;
    debugCode = 400;
    return false;
  }

  if (cooldownActive()) {
    systemEventCode = 429;
    debugCode = 429;
    return false;
  }

  if (safeMode) {
    return (soilPercent <= SAFE_SOIL_THRESHOLD) &&
           (dryCycleCount >= SAFE_CONFIRM_DRY_CYCLES);
  }

  return (soilPercent <= NORMAL_SOIL_THRESHOLD) &&
         (dryCycleCount >= NORMAL_CONFIRM_DRY_CYCLES);
}

void irrigateForSeconds(unsigned long seconds) {
  relayActive = true;
  digitalWrite(RELAY_PIN, RELAY_ON);

  systemEventCode = 210; // riego encendido
  debugCode = 200;

  delay(seconds * 1000UL);

  digitalWrite(RELAY_PIN, RELAY_OFF);
  relayActive = false;
  lastIrrigationEndMs = millis();

  systemEventCode = 211; // riego finalizado
  debugCode = 200;

  Serial.print(F("Riego completado. Segundos: "));
  Serial.println(seconds);
}

// ====
// Envío a ThingSpeak
// field1 = humedad suelo %
// field2 = temperatura
// field3 = humedad ambiente
// field4 = lux
// field5 = presión
// field6 = relay activo
// field7 = estado del sistema / evento
// field8 = debug / código de estado
// ====
bool sendThingSpeak() {
  dtostrf(temperatureC, 0, 1, tsTemp);
  dtostrf(humidityPct, 0, 1, tsHum);

  snprintf(
    tsUrl,
    sizeof(tsUrl),
    "http://api.thingspeak.com/update?api_key=%s&field1=%d&field2=%s&field3=%s&field4=%d&field5=%d&field6=%d&field7=%d&field8=%d",
    THINGSPEAK_API_KEY,
    soilPercent,
    tsTemp,
    tsHum,
    luxValue,
    pressureHpa,
    relayActive ? 1 : 0,
    systemEventCode,
    debugCode
  );

  clearModemBuffer();
  modem.println(F("AT+HTTPTERM"));
  delay(100);

  if (!sendCommandOK("AT+HTTPINIT", 2500)) {
    Serial.println(F("HTTPINIT fallo"));
    return false;
  }

  clearModemBuffer();
  modem.print(F("AT+HTTPPARA=\"URL\",\""));
  modem.print(tsUrl);
  modem.println(F("\""));

  readReply(httpResp, sizeof(httpResp), 4000);

  if (strstr(httpResp, "OK") == nullptr) {
    Serial.println(F("HTTPPARA fallo"));
    Serial.println(httpResp);
    sendCommandAny("AT+HTTPTERM", 1500);
    return false;
  }

  clearModemBuffer();
  modem.println(F("AT+HTTPACTION=0"));
  readReply(httpResp, sizeof(httpResp), 12000);

  Serial.print(F("HTTPACTION: "));
  Serial.println(httpResp);

  bool httpOk = (strstr(httpResp, "+HTTPACTION: 0,200") != nullptr);

  sendCommandAny("AT+HTTPTERM", 1500);
  return httpOk;
}

void setup() {
  Serial.begin(9600);
  delay(2500);

  Serial.println(F("=== TierraViva Estacion Remota A ==="));

  // Relé OFF desde el primer instante
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF);

  pinMode(SOIL_PIN, INPUT);

  Wire.begin();

  bmeOk = bme.begin(0x76);
  if (!bmeOk) bmeOk = bme.begin(0x77);

  lightMeter.begin();

  Serial.println(F("Sensores I2C listos."));
  Serial.println(bmeOk ? F("BME280 OK") : F("BME280 ERROR"));
  Serial.println(F("BH1750 OK"));

  if (!bmeOk) {
    safeMode = true;
  }

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

  /* Función para consultar la calidad de la señal LTE (relación señal-ruido) y el operador de comunicaciones
  printNetworkInfo();
  */
  sendCommandAny("AT+CFUN=1", 3000);

  if (waitForNetworkReady(LTE_WAIT_MAX_MS)) {
    if (configureMobileData()) {
      Serial.println(F("Datos moviles OK"));
    } else {
      Serial.println(F("Datos moviles ERROR"));
      mobileDataReady = false;
    }
  } else {
    mobileDataReady = false;
  }

  Serial.println(F("Sistema listo"));
}

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

  readSensors();
  updateSafetyState();

  Serial.print(F("Modo seguro: "));
  Serial.println(safeMode ? F("ACTIVO") : F("NORMAL"));

  Serial.print(F("Cooldown activo: "));
  Serial.println(cooldownActive() ? F("SI") : F("NO"));

  if (shouldIrrigate()) {
    Serial.println(F("Decision local: RIEGO"));
    irrigateForSeconds(IRRIGATION_SECONDS);
    debugCode = 200;
  } else {
    if (!sensorOk) {
      systemEventCode = 400;
      debugCode = 400;
      Serial.println(F("Decision local: ERROR DE SENSORES"));
    } else if (cooldownActive()) {
      systemEventCode = 429;
      debugCode = 429;
      Serial.println(F("Decision local: COOLDOWN"));
    } else if (safeMode) {
      systemEventCode = 206;
      debugCode = 206;
      Serial.println(F("Decision local: SAFE MODE"));
    } else {
      systemEventCode = 100;
      debugCode = 204;
      Serial.println(F("Decision local: NO RIEGO"));
    }
  }

  if (sendThingSpeak()) {
    Serial.println(F("ThingSpeak OK"));
  } else {
    Serial.println(F("ThingSpeak ERROR"));
  }

  Serial.println(F("Esperando 30 segundos..."));
  delay(SEND_INTERVAL_MS);
}
