#include <SoftwareSerial.h>

SoftwareSerial modem(10, 11); // RX Arduino, TX Arduino

long baudRates[] = {
  9600,
  115200,
  57600,
  38400,
  19200
};

void clearModemBuffer() {
  while (modem.available()) {
    modem.read();
  }
}

bool waitForOK(unsigned long timeoutMs) {
  unsigned long start = millis();
  String resp = "";

  while (millis() - start < timeoutMs) {
    while (modem.available()) {
      char c = modem.read();
      resp += c;
    }
  }

  if (resp.length() > 0) {
    Serial.println(resp);
  }

  return resp.indexOf("OK") >= 0;
}

bool testBaud(long baud) {
  Serial.println();
  Serial.print("Probando baudios: ");
  Serial.println(baud);

  modem.end();
  delay(500);
  modem.begin(baud);
  delay(1000);

  clearModemBuffer();

  for (int i = 0; i < 5; i++) {
    modem.println("AT");
    delay(500);
  }

  bool ok = waitForOK(3000);

  if (ok) {
    Serial.print("RESPONDE EN ");
    Serial.println(baud);
  } else {
    Serial.print("No responde en ");
    Serial.println(baud);
  }

  return ok;
}

void sendAT(const char *cmd, unsigned long waitMs) {
  Serial.println();
  Serial.print(">>> ");
  Serial.println(cmd);

  clearModemBuffer();
  modem.println(cmd);

  unsigned long start = millis();
  bool gotData = false;

  while (millis() - start < waitMs) {
    while (modem.available()) {
      char c = modem.read();
      Serial.write(c);
      gotData = true;
    }
  }

  if (!gotData) {
    Serial.println("[sin respuesta]");
  }

  Serial.println();
}

void setup() {
  Serial.begin(9600);
  delay(3000);

  Serial.println("=== DIAGNOSTICO AUTO-BAUD A7670E ===");
  Serial.println("Probando velocidades del modem...");

  bool found = false;
  long goodBaud = 0;

  for (int i = 0; i < 5; i++) {
    if (testBaud(baudRates[i])) {
      found = true;
      goodBaud = baudRates[i];
      break;
    }
  }

  if (!found) {
    Serial.println();
    Serial.println("NO HAY RESPUESTA EN NINGUNA VELOCIDAD PROBADA.");
    Serial.println("Revisar TX/RX/GND/alimentacion.");
    return;
  }

  Serial.println();
  Serial.print("Usando baudios detectados: ");
  Serial.println(goodBaud);

  modem.end();
  delay(500);
  modem.begin(goodBaud);
  delay(1000);

  sendAT("AT", 3000);
  sendAT("ATI", 4000);
  sendAT("AT+IPR?", 4000);
  sendAT("AT+CPIN?", 4000);
  sendAT("AT+CSQ", 4000);
  sendAT("AT+COPS?", 5000);
  sendAT("AT+CREG?", 4000);
  sendAT("AT+CGREG?", 4000);
  sendAT("AT+CEREG?", 4000);
  sendAT("AT+CGATT?", 4000);
  sendAT("AT+CFUN?", 4000);

  Serial.println();
  Serial.println("=== FIN DIAGNOSTICO ===");
}

void loop() {
}