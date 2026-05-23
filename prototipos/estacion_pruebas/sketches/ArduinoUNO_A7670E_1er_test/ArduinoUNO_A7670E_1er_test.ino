#include <SoftwareSerial.h>

#define A7670_RX_PIN 10  // Arduino recibe desde TXD del A7670E
#define A7670_TX_PIN 11  // Arduino transmite hacia RXD del A7670E

SoftwareSerial modem(A7670_RX_PIN, A7670_TX_PIN);

void setup() {
  Serial.begin(9600);
  modem.begin(115200);

  Serial.println("Prueba A7670E desde Arduino");
  delay(3000);

  sendAT("AT", 1000);
  sendAT("ATE0", 1000);
  sendAT("AT+CPIN?", 1000);
  sendAT("AT+CSQ", 1000);
  sendAT("AT+CEREG?", 1000);
}

void loop() {
  if (modem.available()) {
    Serial.write(modem.read());
  }

  if (Serial.available()) {
    modem.write(Serial.read());
  }
}

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