//저울 캘리브레이션 무선 제어 리모컨
//아두이노 쉴드, 232통신(저울), IR리모컨
#include <LiquidCrystal.h>
#include <SoftwareSerial.h>

SoftwareSerial rs232Serial(12, 13);  // RX, TX
LiquidCrystal lcd(8, 9, 4, 5, 6, 7); 

int mode = 0;                  // 0. 매뉴얼 1. 오토
int a_mode = 0;                // 오토 모드 상태 변수
int a_cal[] = {100, 200, 500, 1000, 2000, 3000, 5000}; // 캘리브레이션 설정값 배열
int index = 0;                 // 인덱스 초기값

int lastButtonState = -1;      // 마지막 버튼 상태 저장 변수

String receivedData = "";      // RS232 수신 데이터 변수
int extractedInt = 0;          // 추출된 데이터의 int로 변환 변수

int UPrelay = 2;      // 정방향 릴레이 핀
int DOWNrelay = 3;    // 역방향 릴레이 핀
int IRrelay = 11;     // ir리모컨 릴레이 핀

unsigned long lastDebounceTime = 0; // 디바운스 시간 추적 변수
const unsigned long debounceDelay = 200; // 디바운스 지연 시간 

void setup() 
{
  Serial.begin(9600);  // 아두이노-시리얼모니터
  rs232Serial.begin(9600); // 아두이노-저울

  pinMode(UPrelay, OUTPUT);
  pinMode(DOWNrelay, OUTPUT);
  pinMode(IRrelay, OUTPUT);

  digitalWrite(UPrelay, LOW);
  digitalWrite(DOWNrelay, LOW);
  digitalWrite(IRrelay, LOW);

  lcd.begin(16, 2);   //  //LCD 초기화
  lcd.setCursor(6, 0);
  lcd.print("TMT");
  lcd.setCursor(2, 1);
  lcd.print("ENGINEERING");
  delay(750);
  lcd.clear(); // 초기화 후 LCD 클리어
}

void loop() 
{
  int key = analogRead(0);                  //키패드 변수
  int buttonState = getButtonState(key);    //키패드 상태 읽기
  int IRrealyState = digitalRead(IRrelay);  //ir리모컨 릴레이 상태 읽기

  while (rs232Serial.available()) 
  {
    char receivedChar = rs232Serial.read();
    receivedData += receivedChar;

    // 데이터의 끝을 나타내는 줄바꿈 문자를 받았을 때
    if (receivedChar == '\n') 
    {
      // 숫자만 추출하기
      String filteredData = "";
      for (int i = 0; i < receivedData.length(); i++)
      {
        if (isDigit(receivedData[i])) filteredData += receivedData[i];
      }

      // 앞의 0 제거하고 첫 번째 0이 아닌 숫자부터 추출
      int firstNonZeroIndex = 0;
      while (firstNonZeroIndex < filteredData.length() && filteredData[firstNonZeroIndex] == '0') firstNonZeroIndex++;
      
      String result = filteredData.substring(firstNonZeroIndex);
      extractedInt = result.toInt(); // String을 int로 변환

      receivedData = "";        // 버퍼 초기화
    }
  }

  if ((millis() - lastDebounceTime) > debounceDelay) // 디바운스 처리
  {
    if (buttonState == 4 && lastButtonState != 4) //sel 버튼 누르면 모드 전환
    {
      mode++;
      if (mode > 1) mode = 0;
      lcd.clear();  // 모드 전환 시 LCD 초기화
      Serial.print("현재 mode: ");  // test
      Serial.println(mode);         // test
    }

    if (buttonState == 0 && lastButtonState != 0)  //right 버튼 누르면 오토모드 전환
    {
      a_mode++;  // a_mode 값을 0과 1로 토글
      if (a_mode > 7) a_mode = 0;
      lcd.clear();
      Serial.print("현재 a_mode: ");  // test
      Serial.println(a_mode);         // test
    }

    if ((a_mode != 2 && a_mode != 4 && a_mode != 6 && a_mode != 8) && mode != 0)     //릴레이 꺼졌다 켜졌다 반복 시 여기 확인
    {
      digitalWrite(UPrelay, LOW);
      digitalWrite(DOWNrelay, LOW);
    }
    
    if (mode == 0)    digitalWrite(IRrelay, LOW);

    // 모드에 따라 다른 기능 실행
    switch (mode) 
    {    
      case 0:  // MANUAL 모드
        MANUAL();
        break;
      case 1:  // AUTO 모드
        AUTO();        
        switch(a_mode)
        {
          case 0:
            AUTO();
            break;
          case 1:
            printLCDMessage("   Auto Mode  ", "     START    ");
            delay(1000);
            a_mode++;
            break;
          case 2:
            AUTO_RUN("Zero");            
            break;
          case 3:
            digitalWrite(IRrelay, LOW);
            printLCDMessage("    Zero Set  ", "      END     ");
            delay(1000);
            a_mode++;
            break;
          case 4: 
            AUTO_LOAD_RUN("Load");
            break;
          case 5:
            printLCDMessage("    Load Set  ", "      END     ");
            delay(1000);            
            a_mode++;
            break;
          case 6:
            digitalWrite(IRrelay, LOW);  
            AUTO_RUN("Check");
            break;
          case 7:
            digitalWrite(IRrelay, LOW);  
            printLCDMessage("      CAL     ", "      END     ");
            delay(1000);
            a_mode++;
            break;
        }
        break;
    }
    lastButtonState = buttonState;  // 버튼 상태 업데이트
    lastDebounceTime = millis();    // 디바운스 시간 초기화
  }
}

void AUTO() 
{
  int key = analogRead(0);
  int buttonState = getButtonState(key);

  lcd.setCursor(0, 0);
  lcd.print("Auto: " + String(a_cal[index]) + "kg  ");
  lcd.setCursor(0, 1);
  lcd.print("Value: " + String(extractedInt)+ "kg  ");

  if (buttonState == 1 && lastButtonState != 1)
  {   
    index++;
    if (index > 6) index = 0;
  }
  else if (buttonState == 2 && lastButtonState != 2) 
  {
    index--;
    if (index < 0) index = 6;
  }
  lastButtonState = buttonState;
}

void AUTO_RUN(const char* label) 
{
  static unsigned long previousMillis = 0;  // 이전 시간을 저장할 변수
  static bool reverseRunning = false;       // 릴레이가 역방향으로 작동 중인지 추적
  static bool relayOff = false;             // 릴레이가 꺼졌는지 상태 추적

  // LCD 출력, 라벨을 매개변수로 사용
  lcd.setCursor(0, 0);
  lcd.print(String(label) + ": " + String(a_cal[index]) + "kg  ");
  lcd.setCursor(0, 1);
  lcd.print("Value: " + String(extractedInt) + "kg  ");

  // 정방향 동작
  if (!reverseRunning && a_cal[index] > extractedInt) 
  {
    digitalWrite(UPrelay, HIGH);
    digitalWrite(DOWNrelay, LOW);
    relayOff = false;  // 릴레이가 켜져 있음
  }

  // 역방향 준비
  if (!reverseRunning && a_cal[index] <= extractedInt)
  {
    if (!relayOff) // 릴레이가 아직 켜져 있는 경우
    {
      digitalWrite(UPrelay, LOW);
      digitalWrite(DOWNrelay, LOW);
      previousMillis = millis();  // 현재 시간을 기록
      relayOff = true;            // 릴레이가 꺼졌음을 표시
    }

    // 릴레이를 끄고 3초 후 역방향으로 전환
    if (relayOff && millis() - previousMillis >= 3000) 
    {
      digitalWrite(UPrelay, LOW);
      digitalWrite(DOWNrelay, HIGH);
      reverseRunning = true;  // 역방향 상태 시작
    }
  }

  // 역방향 지속 동작 (extractedInt가 0이 될 때까지)
  if (reverseRunning) 
  {
    if (extractedInt > 0) 
    {
      digitalWrite(UPrelay, LOW);
      digitalWrite(DOWNrelay, HIGH);
    } 
    else if (extractedInt == 0)
    {
      // extractedInt가 0이 되면 역방향 동작 종료
      digitalWrite(UPrelay, LOW);
      digitalWrite(DOWNrelay, LOW);
      reverseRunning = false;  // 역방향 상태 종료
      relayOff = true;         // 릴레이가 꺼진 상태로 표시

      delay(1000);
      digitalWrite(IRrelay, HIGH);
      a_mode++;
    }
  }
}

void AUTO_LOAD_RUN(const char* label)       // 로드 함수 따로 만듦
{
  static unsigned long previousMillis = 0;  // 이전 시간을 저장할 변수
  static unsigned long delayStartMillis = 0; // 1초 지연 시작 시간 저장 변수
  static bool reverseRunning = false;       // 릴레이가 역방향으로 작동 중인지 추적
  static bool relayOff = false;             // 릴레이가 꺼졌는지 상태 추적
  static bool delayStarted = false;         // 1초 지연 시작 여부

  // LCD 출력, 라벨을 매개변수로 사용
  lcd.setCursor(0, 0);
  lcd.print(String(label) + ": " + String(a_cal[index]) + "kg  ");
  lcd.setCursor(0, 1);
  lcd.print("Value: " + String(extractedInt) + "kg  ");

  if (!reverseRunning && a_cal[index] > extractedInt)     // 정방향 동작
  {
    digitalWrite(UPrelay, HIGH);
    digitalWrite(DOWNrelay, LOW);
    relayOff = false;  // 릴레이가 켜져 있음
  }

  if (!reverseRunning && a_cal[index] <= extractedInt)    // 역방향 동작 준비
  {
    if (!relayOff) // 릴레이 정지
    {
      digitalWrite(UPrelay, LOW);
      digitalWrite(DOWNrelay, LOW);
      previousMillis = millis();  // 현재 시간을 기록
      relayOff = true;            // 릴레이가 꺼졌음을 표시
      delayStarted = false;       // 1초 지연을 아직 시작하지 않음
    }
    if (relayOff && millis() - previousMillis >= 3000 && !delayStarted)     // 3초간 정지 후 1초 대기 시작
    {
      delayStartMillis = millis();  // 1초 지연 시작 시간 기록
      digitalWrite(IRrelay, HIGH);  // 1초 지연 동안 IRrelay 켜기
      delayStarted = true;          // 1초 지연 시작 표시
    }
    if (delayStarted && millis() - delayStartMillis >= 1000)  // 1초 대기 후 역방향 동작
    {
      digitalWrite(IRrelay, LOW);   // 1초 후 IRrelay 끄기
      digitalWrite(UPrelay, LOW);
      digitalWrite(DOWNrelay, HIGH);
      reverseRunning = true;  // 역방향 상태 시작
    }
  }

  // 역방향 지속 동작 (extractedInt가 0이 될 때까지)
  if (reverseRunning) 
  {
    if (extractedInt > 0) 
    {
      digitalWrite(UPrelay, LOW);
      digitalWrite(DOWNrelay, HIGH);
    } 
    else if (extractedInt == 0)
    {
      // extractedInt가 0이 되면 역방향 동작 종료
      digitalWrite(UPrelay, LOW);
      digitalWrite(DOWNrelay, LOW);
      reverseRunning = false;  // 역방향 상태 종료
      relayOff = true;         // 릴레이가 꺼진 상태로 표시
      delay(1000);
      digitalWrite(IRrelay, HIGH);
      a_mode++;
    }
  }
}


void MANUAL()
{
  int key = analogRead(0); 
  int buttonState = getButtonState(key); 

  lcd.setCursor(0, 1);
  lcd.print("Value: " + String(extractedInt) + "kg  ");

  if (buttonState == 1) 
  {      
    printLCDMessage("Manual: UP  ", "");

    digitalWrite(UPrelay, HIGH);  
    digitalWrite(DOWNrelay, LOW);   
  }
  else if (buttonState == 2) 
  { 
    printLCDMessage("Manual: DOWN  ", "");
    
    digitalWrite(UPrelay, LOW);   
    digitalWrite(DOWNrelay, HIGH);  
  }
  else 
  {
    printLCDMessage("Manual: STOP  ", "");

    digitalWrite(UPrelay, LOW);   
    digitalWrite(DOWNrelay, LOW);   
  }
  
  if (a_mode >= 1) // 매뉴얼에서 right버튼 눌러도 a_mode 안넘어감 (매뉴얼<->오토런 버그 방지)
  {
    a_mode = 0;
  }
}

void printLCDMessage(const char* line1, const char* line2)  // lcd.print 간략화 함수 
{
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

int getButtonState(int key) // 키패드 정의
{
  if (key < 50)
    return 0; // right
  else if (key < 195)
    return 1; // up
  else if (key < 380)
    return 2; // down
  else if (key < 555)
    return 3; // left
  else if (key < 790)
    return 4; // select
  else
    return -1; // none
}
