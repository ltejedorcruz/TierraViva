#include <SoftwareSerial.h>

SoftwareSerial modem(10, 11); // RX Arduino, TX Arduino

const char APN[] = "internet";
const char THINGSPEAK_API_KEY[] = "REEMPLAZAR_POR_WRITE_API_KEY";

int valorPrueba = 100;

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

void readFor(unsigned long waitMs) {
  unsigned long start = millis();

  while (millis() - start < waitMs) {
    while (modem.available()) {
      Serial.write(modem.read());
    }
  }
}

void sendCommandRead(const char *cmd, unsigned long waitMs) {
  Serial.print("\n>>> ");
  Serial.println(cmd);

  clearModemBuffer();
  modem.println(cmd);

  readFor(waitMs);

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
  Serial.println("Forzando A7670E a 9600 para SoftwareSerial...");

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

  Serial.println("Comunicacion con modem ajustada a 9600.");
}

void modemBasicCheck() {
  Serial.println("\nComprobacion basica del modem...");

  sendCommandWaitOK("AT", 3000);
  sendCommandRead("AT+IPR?", 2000);
  sendCommandRead("AT+CPIN?", 3000);
  sendCommandRead("AT+CSQ", 3000);
  sendCommandRead("AT+CREG?", 3000);
  sendCommandRead("AT+CGREG?", 3000);
  sendCommandRead("AT+CEREG?", 3000);
  sendCommandRead("AT+COPS?", 5000);
}

bool configureMobileData() {
  Serial.println("\nConfigurando datos moviles...");

  char cmdApn[80];

  snprintf(
    cmdApn,
    sizeof(cmdApn),
    "AT+CGDCONT=1,\"IP\",\"%s\"",
    APN
  );

  if (!sendCommandWaitOK(cmdApn, 5000)) {
    Serial.println("Fallo configurando APN.");
    return false;
  }

  if (!sendCommandWaitOK("AT+CGATT=1", 10000)) {
    Serial.println("Fallo en CGATT.");
    return false;
  }

  if (!sendCommandWaitOK("AT+CGACT=1,1", 15000)) {
    Serial.println("Fallo activando contexto PDP.");
    return false;
  }

  sendCommandRead("AT+CGPADDR=1", 5000);

  return true;
}

void closeHttpSessionIfOpen() {
  Serial.println("\nCerrando posible sesion HTTP previa...");

  Serial.println(">>> AT+HTTPTERM");
  clearModemBuffer();
  modem.println("AT+HTTPTERM");

  readFor(2000);

  Serial.println();
  Serial.println("Nota: si aqui aparece ERROR, es normal si no habia sesion HTTP abierta.");
}

bool sendThingSpeakValue(int value) {
  char url[130];

  snprintf(
    url,
    sizeof(url),
    "http://api.thingspeak.com/update?api_key=%s&field1=%d",
    THINGSPEAK_API_KEY,
    value
  );

  Serial.println("\n==============================");
  Serial.print("Enviando a ThingSpeak field1=");
  Serial.println(value);

  Serial.println("URL:");
  Serial.println(url);

  closeHttpSessionIfOpen();
  delay(500);

  if (!sendCommandWaitOK("AT+HTTPINIT", 5000)) {
    Serial.println("No se pudo iniciar HTTP.");
    return false;
  }

  char cmdUrl[190];

  snprintf(
    cmdUrl,
    sizeof(cmdUrl),
    "AT+HTTPPARA=\"URL\",\"%s\"",
    url
  );

  if (!sendCommandWaitOK(cmdUrl, 8000)) {
    Serial.println("No se pudo configurar URL.");
    sendCommandWaitOK("AT+HTTPTERM", 3000);
    return false;
  }

  Serial.println("\n>>> AT+HTTPACTION=0");
  clearModemBuffer();
  modem.println("AT+HTTPACTION=0");

  bool http200 = waitForText("+HTTPACTION: 0,200", 30000);
  Serial.println();

  Serial.println("\nCerrando HTTP...");
  sendCommandWaitOK("AT+HTTPTERM", 5000);

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

  Serial.println("=== TierraViva envio periodico ThingSpeak ===");

  force9600();

  modemBasicCheck();

  bool dataOk = configureMobileData();

  if (dataOk) {
    Serial.println("\nDatos moviles configurados correctamente.");
  } else {
    Serial.println("\nERROR: no se pudieron configurar los datos moviles.");
  }

  Serial.println("\nSetup terminado. Comenzando envios periodicos...");
}

void loop() {
  valorPrueba++;

  bool ok = sendThingSpeakValue(valorPrueba);

  if (ok) {
    Serial.println("RESULTADO: dato enviado correctamente.");
  } else {
    Serial.println("RESULTADO: fallo el envio.");
  }

  Serial.println("Esperando 20 segundos antes del siguiente envio...");
  delay(20000);
}