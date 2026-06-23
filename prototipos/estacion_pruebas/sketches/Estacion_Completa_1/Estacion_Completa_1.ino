#include <SoftwareSerial.h>
#include <Wire.h>
#include <BH1750.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

SoftwareSerial modem(10, 11); // RX Arduino, TX Arduino

BH1750 lightMeter;
Adafruit_BME280 bme;

const char APN[] = "internet";
const char THINGSPEAK_API_KEY[] = "REEMPLAZAR_POR_WRITE_API_KEY";

const int SOIL_PIN = A0;
const int RELAY_PIN = 7;

// Calibración inicial del sensor
const int SOIL_DRY = 558;  // seco / aire
const int SOIL_WET = 211;  // húmedo

// Relé activo en LOW
const byte RELAY_ON = LOW;
const byte RELAY_OFF = HIGH;

const unsigned long SEND_INTERVAL_MS = 30000;

void clearModemBuffer() {
  while (modem.available()) {
    modem.read();
  }
}

bool waitForText(const char *target, unsigned long timeoutMs) {
  unsigned long start = millis();
  int index = 0;
  int len = strlen(target);

  while (millis() - start < timeoutMs) {
    while (modem.available()) {
      char c = modem.read();

      if (c == target[index]) {
        index++;
        if (index == len) return true;
      } else {
        index = 0;
        if (c == target[0]) index = 1;
      }
    }
  }
  return false;
}

bool sendCommandOK(const char *cmd, unsigned long timeoutMs) {
  clearModemBuffer();
  modem.println(cmd);
  return waitForText("OK", timeoutMs);
}

bool sendCommandAny(const char *cmd, unsigned long timeoutMs) {
  clearModemBuffer();
  modem.println(cmd);
  return waitForText("OK", timeoutMs) || waitForText("ERROR", timeoutMs);
}

void force9600() {
  modem.begin(115200);
  delay(1000);

  for (int i = 0; i < 8; i++) {
    modem.println("AT");
    delay(150);
  }

  for (int i = 0; i < 8; i++) {
    modem.println("AT+IPR=9600");
    delay(250);
  }

  delay(1000);
  modem.end();
  delay(500);
  modem.begin(9600);
  delay(1000);
}

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

bool configureMobileData() {
  char cmdApn[64];
  snprintf(cmdApn, sizeof(cmdApn), "AT+CGDCONT=1,\"IP\",\"%s\"", APN);

  if (!sendCommandOK(cmdApn, 5000)) return false;
  if (!sendCommandOK("AT+CGATT=1", 10000)) return false;
  if (!sendCommandOK("AT+CGACT=1,1", 15000)) return false;

  clearModemBuffer();
  modem.println("AT+CGPADDR=1");
  waitForText("OK", 5000);

  return true;
}

bool sendThingSpeak(int soilPercent, float tempC, float humPct, float lux) {
  char tempStr[10];
  char humStr[10];
  char luxStr[12];
  char url[190];
  char cmdUrl[230];

  dtostrf(tempC, 0, 1, tempStr);
  dtostrf(humPct, 0, 1, humStr);
  dtostrf(lux, 0, 0, luxStr);

  snprintf(
    url,
    sizeof(url),
    "http://api.thingspeak.com/update?api_key=%s&field1=%d&field2=%s&field3=%s&field4=%s",
    THINGSPEAK_API_KEY,
    soilPercent,
    tempStr,
    humStr,
    luxStr
  );

  clearModemBuffer();
  modem.println("AT+HTTPTERM");
  waitForText("OK", 2000);

  if (!sendCommandOK("AT+HTTPINIT", 5000)) return false;

  snprintf(cmdUrl, sizeof(cmdUrl), "AT+HTTPPARA=\"URL\",\"%s\"", url);
  if (!sendCommandOK(cmdUrl, 8000)) {
    sendCommandAny("AT+HTTPTERM", 3000);
    return false;
  }

  clearModemBuffer();
  modem.println("AT+HTTPACTION=0");

  bool httpOk = waitForText("+HTTPACTION: 0,200", 30000);

  sendCommandAny("AT+HTTPTERM", 3000);

  return httpOk;
}

void setup() {
  Serial.begin(9600);
  delay(3000);

  Serial.println(F("=== TierraViva integracion ligera ==="));

  pinMode(SOIL_PIN, INPUT);

  digitalWrite(RELAY_PIN, RELAY_OFF);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF);

  Wire.begin();

  bool bmeOk = bme.begin(0x76);
  if (!bmeOk) bmeOk = bme.begin(0x77);

  lightMeter.begin();

  Serial.println(F("Sensores I2C listos."));
  Serial.println(bmeOk ? F("BME280 OK") : F("BME280 ERROR"));

  force9600();

  if (sendCommandOK("AT", 3000)) {
    Serial.println(F("Modem OK"));
  } else {
    Serial.println(F("Modem ERROR"));
  }

  if (configureMobileData()) {
    Serial.println(F("Datos moviles OK"));
  } else {
    Serial.println(F("Datos moviles ERROR"));
  }

  Serial.println(F("Sistema listo"));
}

void loop() {
  int soilRaw = readSoilRaw();
  int soilPercent = soilRawToPercent(soilRaw);

  float tempC = bme.readTemperature();
  float humPct = bme.readHumidity();
  float lux = lightMeter.readLightLevel();

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
  Serial.println(lux, 0);

  if (sendThingSpeak(soilPercent, tempC, humPct, lux)) {
    Serial.println(F("ThingSpeak OK"));
    relayPulse2s();
  } else {
    Serial.println(F("ThingSpeak ERROR"));
  }

  delay(SEND_INTERVAL_MS);
}
