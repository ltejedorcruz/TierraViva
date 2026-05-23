#include <SoftwareSerial.h>

#define A7670_RX_PIN 10
#define A7670_TX_PIN 11

SoftwareSerial modem(A7670_RX_PIN, A7670_TX_PIN);

void setup() {
  Serial.begin(9600);
  modem.begin(115200);

  Serial.println("Test A7670E UART a 115200");
  delay(5000);

  Serial.println(">> AT");
  modem.println("AT");
}

void loop() {
  while (modem.available()) {
    Serial.write(modem.read());
  }

  while (Serial.available()) {
    modem.write(Serial.read());
  }
}
