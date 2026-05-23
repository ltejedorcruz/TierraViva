#include <SoftwareSerial.h>

SoftwareSerial modem(10, 11); // RX Arduino, TX Arduino

const char APN[] = "internet";
const char THINGSPEAK_API_KEY[] = "2E94ZA4IBJ2KRU7U";

void clearModemBuffer() {
  while (modem.available()) {
    modem.read();
  }
}

void readModem(unsigned long timeoutMs) {
  unsigned long start = millis();

  while (millis() - start < timeoutMs) {
    while (modem.available()) {
      char c = modem.read();
      Serial.write(c);
    }
  }
}

void sendCommand(const char *cmd, unsigned long waitMs) {
  Serial.print("\n>>> ");
  Serial.println(cmd);

  clearModemBuffer();
  modem.println(cmd);
  readModem(waitMs);
  Serial.println();
}

bool waitForText(const char *target, unsigned long timeoutMs) {
  unsigned long start = millis();
  int index = 0;
  int len = strlen(target);

  while (millis() - start < timeoutMs) {
    while (modem.available()) {
      char c = modem.read();
      Serial.write(c);

      if (c == target[index]) {
        index++;
        if (index == len) {
          return true;
        }
      } else {
        index = 0;
        if (c == target[0]) {
          index = 1;
        }
      }
    }
  }

  return false;
}

bool sendCommandWaitOK(const char *cmd, unsigned long timeoutMs) {
  Serial.print("\n>>> ");
  Serial.println(cmd);

  clearModemBuffer();
  modem.println(cmd);

  bool ok = waitForText("OK", timeoutMs);
  Serial.println();

  if (ok) {
    Serial.println("[OK recibido]");
  } else {
    Serial.println("[NO se recibio OK]");
  }

  return ok;
}

void force9600() {
  Serial.println("Forzando A7670E a 9600...");

  modem.begin(115200);
  delay(1000);

  for (int i = 0; i < 10; i++) {
    modem.println("AT");
    delay(200);
  }

  for (int i = 0; i < 10; i++) {
    modem.println("AT+IPR=9600");
    delay(300);
  }

  delay(1000);

  modem.end();
  delay(500);
  modem.begin(9600);
  delay(1000);

  Serial.println("Probando comunicacion a 9600...");
}

bool httpGetThingSpeak(int value) {
  char url[120];

  snprintf(
    url,
    sizeof(url),
    "http://api.thingspeak.com/update?api_key=%s&field1=%d",
    THINGSPEAK_API_KEY,
    value
  );

  Serial.println("\nPreparando HTTP GET a ThingSpeak:");
  Serial.println(url);

  sendCommandWaitOK("AT+HTTPTERM", 2000);
  delay(500);

  if (!sendCommandWaitOK("AT+HTTPINIT", 5000)) {
    return false;
  }

  if (!sendCommandWaitOK("AT+HTTPPARA=\"CID\",1", 3000)) {
    return false;
  }

  char cmdUrl[170];
  snprintf(cmdUrl, sizeof(cmdUrl), "AT+HTTPPARA=\"URL\",\"%s\"", url);

  if (!sendCommandWaitOK(cmdUrl, 5000)) {
    return false;
  }

  Serial.println("\n>>> AT+HTTPACTION=0");
  clearModemBuffer();
  modem.println("AT+HTTPACTION=0");

  bool gotAction = waitForText("+HTTPACTION:", 30000);
  Serial.println();

  if (!gotAction) {
    Serial.println("[NO se recibio +HTTPACTION]");
    return false;
  }

  Serial.println("[HTTPACTION recibido]");

  sendCommand("AT+HTTPREAD", 5000);
  sendCommandWaitOK("AT+HTTPTERM", 3000);

  return true;
}

void setup() {
  Serial.begin(9600);
  delay(3000);

  Serial.println("=== TierraViva A7670E ThingSpeak ===");

  force9600();

  sendCommandWaitOK("AT", 3000);
  sendCommand("AT+IPR?", 2000);
  sendCommand("AT+CPIN?", 3000);
  sendCommand("AT+CSQ", 3000);
  sendCommand("AT+CREG?", 3000);
  sendCommand("AT+CGREG?", 3000);
  sendCommand("AT+COPS?", 5000);

  Serial.println("\nConfigurando datos moviles...");

  char cmdApn[80];
  snprintf(cmdApn, sizeof(cmdApn), "AT+CGDCONT=1,\"IP\",\"%s\"", APN);

  sendCommandWaitOK(cmdApn, 5000);
  sendCommandWaitOK("AT+CGATT=1", 10000);
  sendCommandWaitOK("AT+CGACT=1,1", 15000);

  sendCommand("AT+CGPADDR=1", 5000);

  Serial.println("\nEnviando dato fijo a ThingSpeak...");
  bool sent = httpGetThingSpeak(123);

  if (sent) {
    Serial.println("\nRESULTADO: envio HTTP terminado. Revisa ThingSpeak.");
  } else {
    Serial.println("\nRESULTADO: fallo el envio HTTP.");
  }

  Serial.println("\nFin del sketch.");
}

void loop() {
}