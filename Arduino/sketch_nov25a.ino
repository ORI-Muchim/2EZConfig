const uint8_t RELAY_PIN = 7;  // Using uint8_t is more memory efficient for pin numbers

void setup() {
  Serial.begin(600);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
}

void loop() {
  if (Serial.available()) {  // No need to compare with > 0
    switch(Serial.read()) {  // Switch is more efficient than if-else
      case '0':
        digitalWrite(RELAY_PIN, LOW);
        break;
      case '1':
        digitalWrite(RELAY_PIN, HIGH);
        break;
    }
    Serial.flush();  // More efficient than manual buffer clearing
  }
}
