#include <SoftwareSerial.h>

#define A7670_RX_PIN 10
#define A7670_TX_PIN 11

SoftwareSerial modem(A7670_RX_PIN, A7670_TX_PIN);

void readModem(unsigned long durationMs) {
  unsigned long start = millis();
  while (millis() - start < durationMs) {
    while (modem.available()) {
      Serial.write(modem.read());
    }
  }
}

void sendAT(const char* cmd, unsigned long waitMs) {
  Serial.print(">> ");
  Serial.println(cmd);
  modem.println(cmd);
  readModem(waitMs);
  Serial.println();
}

void setup() {
  Serial.begin(9600);
  delay(2000);

  Serial.println("Cambio baudrate A7670E a 9600");
  Serial.println("Probando primero a 115200...");

  modem.begin(115200);
  delay(1000);

  sendAT("AT", 1000);
  sendAT("ATE0", 1000);

  Serial.println("Enviando AT+IPR=9600...");
  sendAT("AT+IPR=9600", 1000);

  Serial.println("Cambiando SoftwareSerial a 9600...");
  modem.end();
  delay(500);
  modem.begin(9600);
  delay(1000);

  sendAT("AT", 1000);
  sendAT("ATE0", 1000);

  Serial.println("Si ves OK despues de este AT, ya esta a 9600.");
}

void loop() {
  if (modem.available()) {
    Serial.write(modem.read());
  }

  if (Serial.available()) {
    modem.write(Serial.read());
  }
}