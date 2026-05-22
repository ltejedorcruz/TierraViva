#include <Wire.h>
#include <BH1750.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define BME_ADDR 0x76
const int SOIL_PIN = A0;

BH1750 lightMeter(0x23);
Adafruit_BME280 bme;

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }

  Serial.println("TierraViva - Sensor Test (BH1750 + BME280 + Soil A0)");

  Wire.begin();

  // BH1750
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("BH1750 OK");
  } else {
    Serial.println("BH1750 FAIL (check wiring/address)");
  }

  // BME280
  bool bme_ok = bme.begin(BME_ADDR);
  if (bme_ok) {
    Serial.println("BME280 OK");
  } else {
    Serial.println("BME280 FAIL (maybe BMP280 or wrong addr/wiring)");
  }

  Serial.println("----");
}

void loop() {
  // Soil
  int soil_raw = analogRead(SOIL_PIN);

  // Light
  float lux = lightMeter.readLightLevel();

  // BME values (only valid if sensor is BME and begin() worked)
  float temp = bme.readTemperature();        // °C
  float hum  = bme.readHumidity();           // %
  float pres = bme.readPressure() / 100.0F;  // hPa

  Serial.print("soil_raw=");
  Serial.print(soil_raw);

  Serial.print(" lux=");
  Serial.print(lux, 1);

  Serial.print(" temp_c=");
  Serial.print(temp, 2);

  Serial.print(" hum_pct=");
  Serial.print(hum, 1);

  Serial.print(" pres_hpa=");
  Serial.print(pres, 1);

  Serial.println();

  delay(1000);
}