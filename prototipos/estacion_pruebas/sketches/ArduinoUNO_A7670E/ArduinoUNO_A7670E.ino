#include <SoftwareSerial.h>

#define A7670_RX_PIN 10
#define A7670_TX_PIN 11

SoftwareSerial modem(A7670_RX_PIN, A7670_TX_PIN);

void sendAT(const char* cmd, unsigned long waitMs) {
  Serial.print(">> ");
  Serial.println(cmd);

  modem.println(cmd);

  unsigned long start = millis();
  while (millis() - start < waitMs) {
    while (modem.available()) {
      Serial.write(modem.read());
    }
  }

  Serial.println();
}

void setup() {
  Serial.begin(9600);
  modem.begin(9600);

  Serial.println("Chequeo red A7670E");
  delay(3000);

  sendAT("AT", 1000);
  sendAT("ATE0", 1000);
  sendAT("AT+CPIN?", 1000);
  sendAT("AT+CSQ", 1000);
  sendAT("AT+CEREG?", 1000);
  sendAT("AT+CGATT?", 1000);
}

void loop() {
}