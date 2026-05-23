void setup() {
  Serial.begin(115200);
  delay(3000);

  Serial.println("AT");
  delay(1000);

  Serial.println("AT+CPIN?");
  delay(1000);

  Serial.println("AT+CSQ");
  delay(1000);

  Serial.println("AT+CREG?");
  delay(1000);

  Serial.println("AT+COPS?");
}

void loop() {
}