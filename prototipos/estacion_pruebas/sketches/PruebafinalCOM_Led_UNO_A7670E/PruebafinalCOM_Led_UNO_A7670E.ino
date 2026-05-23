#define LED_STATUS 13

bool modemOK = false;

void blink(int times, int onMs, int offMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_STATUS, HIGH);
    delay(onMs);
    digitalWrite(LED_STATUS, LOW);
    delay(offMs);
  }
}

void clearSerial() {
  while (Serial.available()) {
    Serial.read();
  }
}

bool waitForText(const char* expected, unsigned long timeoutMs) {
  String response = "";
  unsigned long start = millis();

  while (millis() - start < timeoutMs) {
    while (Serial.available()) {
      char c = Serial.read();
      response += c;

      if (response.indexOf(expected) >= 0) {
        return true;
      }
    }
  }

  return false;
}

bool sendATWaitOK(const char* cmd, unsigned long timeoutMs) {
  clearSerial();
  Serial.println(cmd);
  return waitForText("OK", timeoutMs);
}

void setup() {
  pinMode(LED_STATUS, OUTPUT);

  blink(3, 150, 150);

  Serial.begin(9600);

  delay(10000);

  modemOK = sendATWaitOK("AT", 5000);

  if (modemOK) {
    digitalWrite(LED_STATUS, HIGH);
  } else {
    digitalWrite(LED_STATUS, LOW);
  }
}

void loop() {
  if (!modemOK) {
    blink(1, 250, 750);
  }
}