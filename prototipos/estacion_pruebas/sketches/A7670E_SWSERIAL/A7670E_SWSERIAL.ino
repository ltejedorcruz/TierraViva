#include <SoftwareSerial.h>

SoftwareSerial modem(10, 11); // RX Arduino, TX Arduino

void sendCommand(const char *cmd, unsigned long waitMs) {
  Serial.print("Enviando: ");
  Serial.println(cmd);

  modem.println(cmd);

  unsigned long start = millis();
  while (millis() - start < waitMs) {
    while (modem.available()) {
      char c = modem.read();
      Serial.write(c);
    }
  }

  Serial.println();
}

void setup() {
  Serial.begin(9600);
  delay(3000);

  Serial.println("Inicio prueba A7670E");
  Serial.println("Fase 1: intentando hablar con el modulo a 115200 por SoftwareSerial");

  modem.begin(115200);
  delay(1000);

  for (int i = 0; i < 10; i++) {
    sendCommand("AT", 300);
  }

  Serial.println("Intentando cambiar velocidad temporal a 9600...");
  for (int i = 0; i < 10; i++) {
    sendCommand("AT+IPR=9600", 500);
  }

  delay(1000);

  Serial.println("Fase 2: cambiando SoftwareSerial a 9600");
  modem.end();
  delay(500);
  modem.begin(9600);
  delay(1000);

  for (int i = 0; i < 5; i++) {
    sendCommand("AT", 1000);
  }

  sendCommand("AT+IPR?", 1000);
  sendCommand("AT+CPIN?", 1000);
  sendCommand("AT+CSQ", 1000);

  Serial.println("Fin de prueba");
}

void loop() {
}