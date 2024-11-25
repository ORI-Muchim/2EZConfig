// 아두이노 코드
const int RELAY_PIN = 7;  // 릴레이가 연결된 핀 번호

void setup() {
  Serial.begin(9600);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);  // 릴레이 초기 상태: OFF
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
    
    // 나머지 데이터 비우기
    while(Serial.available() > 0) {
      Serial.read();
    }
  }
}