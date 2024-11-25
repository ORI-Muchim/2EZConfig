const int RELAY_PIN = 7;  // Relay pin number

void setup() {
  Serial.begin(9600);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);  // Initialize relay to off
}

void loop() {
  if (Serial.available() > 0) {
    char command = Serial.read();
    if (command == '0') {
      digitalWrite(RELAY_PIN, LOW);
    }
    else if (command == '1') {
      digitalWrite(RELAY_PIN, HIGH);
    }
    
    // Clear the serial buffer
    while(Serial.available() > 0) {
      Serial.read();
    }
  }
}