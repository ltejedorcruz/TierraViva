#include <SoftwareSerial.h>

#define A7670_RX_PIN 10
#define A7670_TX_PIN 11

SoftwareSerial modem(A7670_RX_PIN, A7670_TX_PIN);

void setup() {
  Serial.begin(9600);
  modem.begin(9600);

  Serial.println("Test basico A7670E 9600");
  Serial.println("Esperando 15 segundos a que arranque el modulo...");
  delay(15000);
}

void loop() {
  Serial.println("Enviando AT...");
  modem.println("AT");

  unsigned long start = millis();
  bool received = false;

  while (millis() - start < 3000) {
    while (modem.available()) {
      char c = modem.read();
      Serial.write(c);
      received = true;
    }
  }

  if (!received) {
    Serial.println("SIN RESPUESTA");
  }

  Serial.println();
  delay(3000);
}