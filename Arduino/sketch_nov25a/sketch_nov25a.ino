// 개선된 Arduino 코드 - 안정성과 통신 상태 모니터링 추가
const uint8_t RELAY_PIN = 7;
const uint8_t LED_PIN = 13;  // 상태 표시용 내장 LED

// 상태 관리
bool relayState = false;
unsigned long lastCommandTime = 0;
unsigned long lastHeartbeat = 0;
const unsigned long SAFETY_TIMEOUT = 10000;  // 10초 안전 타임아웃
const unsigned long HEARTBEAT_INTERVAL = 1000;  // 1초마다 하트비트

void setup() {
  Serial.begin(9600);
  
  // 시리얼 버퍼 크기 최적화
  Serial.setTimeout(100);  // 100ms 타임아웃
  
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  
  // 초기 상태
  setRelayState(false);
  
  // 시작 신호
  Serial.println("READY");
  
  lastCommandTime = millis();
  lastHeartbeat = millis();
}

void loop() {
  unsigned long currentTime = millis();
  
  // 시리얼 명령 처리
  if (Serial.available()) {
    char command = Serial.read();
    
    // 남은 버퍼 데이터 클리어 (오버플로우 방지)
    while (Serial.available()) {
      Serial.read();
    }
    
    processCommand(command);
    lastCommandTime = currentTime;
  }
  
  // 안전 타임아웃 체크 - 일정 시간 명령이 없으면 릴레이 OFF
  if (currentTime - lastCommandTime > SAFETY_TIMEOUT) {
    if (relayState) {
      setRelayState(false);
      Serial.println("TIMEOUT_OFF");
    }
  }
  
  // 하트비트 전송 (연결 상태 확인용)
  if (currentTime - lastHeartbeat > HEARTBEAT_INTERVAL) {
    Serial.println("HB");  // HeartBeat
    lastHeartbeat = currentTime;
  }
  
  // LED 상태 표시 (릴레이 상태와 동기화)
  digitalWrite(LED_PIN, relayState);
  
  // CPU 부하 감소
  delay(1);
}

void processCommand(char command) {
  switch(command) {
    case '0':
      setRelayState(false);
      Serial.println("OFF");
      break;
      
    case '1':
      setRelayState(true);
      Serial.println("ON");
      break;
      
    case 'P':  // Ping 명령
      Serial.println("PONG");
      break;
      
    case 'S':  // Status 요청
      Serial.print("STATUS:");
      Serial.println(relayState ? "ON" : "OFF");
      break;
      
    case 'R':  // Reset 명령
      setRelayState(false);
      Serial.println("RESET");
      break;
      
    default:
      // 알 수 없는 명령
      Serial.print("UNK:");
      Serial.println(command);
      break;
  }
}

void setRelayState(bool state) {
  relayState = state;
  digitalWrite(RELAY_PIN, state ? HIGH : LOW);
}

// 에러 처리 및 복구
void handleError() {
  // 시리얼 포트 리셋
  Serial.end();
  delay(100);
  Serial.begin(9600);
  
  // 안전을 위해 릴레이 OFF
  setRelayState(false);
  
  Serial.println("ERROR_RECOVERY");
}
