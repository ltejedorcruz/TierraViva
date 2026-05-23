#include <SoftwareSerial.h>

SoftwareSerial modem(10, 11); // RX Arduino, TX Arduino

const char APN[] = "internet";
const char THINGSPEAK_API_KEY[] = "REEMPLAZAR_POR_WRITE_API_KEY";

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

void sendCommandRead(const char *cmd, unsigned long waitMs) {
  Serial.print("\n>>> ");
  Serial.println(cmd);

  clearModemBuffer();
  modem.println(cmd);

  unsigned long start = millis();
  while (millis() - start < waitMs) {
    while (modem.available()) {
      Serial.write(modem.read());
    }
  }

  Serial.println();
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

  Serial.println("Comunicacion del modem ajustada a 9600.");
}

bool sendThingSpeakFixedValue(int value) {
  char url[120];

  snprintf(
    url,
    sizeof(url),
    "http://api.thingspeak.com/update?api_key=%s&field1=%d",
    THINGSPEAK_API_KEY,
    value
  );

  Serial.println("\nURL:");
  Serial.println(url);

  sendCommandWaitOK("AT+HTTPTERM", 2000);
  delay(500);

  if (!sendCommandWaitOK("AT+HTTPINIT", 5000)) {
    return false;
  }

  char cmdUrl[170];
  snprintf(cmdUrl, sizeof(cmdUrl), "AT+HTTPPARA=\"URL\",\"%s\"", url);

  if (!sendCommandWaitOK(cmdUrl, 8000)) {
    return false;
  }

  Serial.println("\n>>> AT+HTTPACTION=0");
  clearModemBuffer();
  modem.println("AT+HTTPACTION=0");

  bool http200 = waitForText("+HTTPACTION: 0,200", 30000);
  Serial.println();

  sendCommandWaitOK("AT+HTTPTERM", 3000);

  if (http200) {
    Serial.println("[HTTP 200 recibido]");
    return true;
  } else {
    Serial.println("[NO se recibio HTTP 200]");
    return false;
  }
}

void setup() {
  Serial.begin(9600);
  delay(3000);

  Serial.println("=== TierraViva A7670E ThingSpeak OK ===");

  force9600();

  sendCommandWaitOK("AT", 3000);
  sendCommandRead("AT+IPR?", 2000);
  sendCommandRead("AT+CPIN?", 3000);
  sendCommandRead("AT+CSQ", 3000);
  sendCommandRead("AT+CREG?", 3000);
  sendCommandRead("AT+CGREG?", 3000);
  sendCommandRead("AT+COPS?", 5000);

  Serial.println("\nConfigurando datos moviles...");

  char cmdApn[80];
  snprintf(cmdApn, sizeof(cmdApn), "AT+CGDCONT=1,\"IP\",\"%s\"", APN);

  sendCommandWaitOK(cmdApn, 5000);
  sendCommandWaitOK("AT+CGATT=1", 10000);
  sendCommandWaitOK("AT+CGACT=1,1", 15000);
  sendCommandRead("AT+CGPADDR=1", 5000);

  Serial.println("\nEnviando field1=123 a ThingSpeak...");

  bool ok = sendThingSpeakFixedValue(123);

  if (ok) {
    Serial.println("\nRESULTADO: DATO ENVIADO CORRECTAMENTE A THINGSPEAK.");
  } else {
    Serial.println("\nRESULTADO: FALLO EL ENVIO A THINGSPEAK.");
  }

  Serial.println("\nFin.");
}

void loop() {
}