#include <SoftwareSerial.h>
#include <Wire.h>
#include <DHT.h>
#include <TimerOne.h>
#include <LiquidCrystal_I2C.h>

#define VIN_PIN A0

#define BUZZER_PIN 8
#define STOP_PIN 13

#define DEBUG

#define CDS_PIN A0

#define ARR_CNT 5
#define CMD_SIZE 50
char lcdLine1[17] = "Mobile SMART ESS ";
char lcdLine2[17] = "";
char sendBuf[CMD_SIZE];
char recvId[10] = "ESS_LIN";


bool ledOn = false;         // LED의 현재 상태 (on/off)
bool timerIsrFlag = false;
unsigned int secCount;

int Vout = 0;

int stopFlag=0; // 0 normal mode
                // 1 emergency stop mode

int getSensorTime;

LiquidCrystal_I2C lcd(0x27, 16, 2);

SoftwareSerial BTSerial(10, 11);  // RX ==>BT:TXD, TX ==> BT:RX
//시리얼 모니터에서 9600bps, line ending 없음 설정 후 AT명령 --> OK 리턴
//AT+NAMEiotxx ==> OKname   : 이름 변경 iotxx
void setup() {
#ifdef DEBUG
  Serial.begin(9600);
  Serial.println("setup() start!");
#endif
  lcd.init();
  lcd.backlight();
  lcdDisplay(0, 0, lcdLine1);
  lcdDisplay(0, 1, lcdLine2);
  //  pinMode(BUTTON_PIN, INPUT_PULLUP);    //MCU내부 풀업 활성화

  pinMode(BUZZER_PIN, OUTPUT);

  pinMode(STOP_PIN, OUTPUT);
  BTSerial.begin(9600);  // set the data rate for the SoftwareSerial port
  Timer1.initialize(1000000);
  Timer1.attachInterrupt(timerIsr);  // timerIsr to run every 1 seconds

}

void loop() {
  if (BTSerial.available())
    bluetoothEvent();

  if (timerIsrFlag) {
    timerIsrFlag = false;
    if (!(secCount % 2)) {  //2초에 한번 실행

      Vout = map(analogRead(VIN_PIN), 0, 1023, 0, 100);

      if(stopFlag){
        sprintf(lcdLine2, "Vout:%d%% STOP", Vout);
      }
      else{
        sprintf(lcdLine2, "Vout:%d%% AUTO", Vout);
      }
      
      lcdDisplay(0, 1, lcdLine2);
#ifdef DEBUG
      Serial.println(lcdLine2);
#endif
    }
  }
  
#ifdef DEBUG
  if (Serial.available())
    BTSerial.write(Serial.read());
#endif
}
void bluetoothEvent() {
  int i = 0;
  char* pToken;
  char* pArray[ARR_CNT] = { 0 };
  char recvBuf[CMD_SIZE] = { 0 };
  int len = BTSerial.readBytesUntil('\n', recvBuf, sizeof(recvBuf) - 1);

#ifdef DEBUG
  Serial.print("Recv : ");
  Serial.println(recvBuf);
#endif

  pToken = strtok(recvBuf, "[@]");
  while (pToken != NULL) {
    pArray[i] = pToken;
    if (++i >= ARR_CNT)
      break;
    pToken = strtok(NULL, "[@]");
  }
  //recvBuf : [XXX_LIN]LED@ON
  //pArray[0] = "XXX_LIN"   : 송신자 ID
  //pArray[1] = "LED"
  //pArray[2] = "ON"
  //pArray[3] = 0x0

  if (!strcmp(pArray[1], "STOP")) {
    if (!strcmp(pArray[2], "ON")) {
      digitalWrite(STOP_PIN, HIGH);
      sprintf(sendBuf, "[%s]%s@%s\n", pArray[0], pArray[1], pArray[2]);
      stopFlag = 1;
    } else if (!strcmp(pArray[2], "OFF")) {
      digitalWrite(STOP_PIN, LOW);
      sprintf(sendBuf, "[%s]%s@%s\n", pArray[0], pArray[1], pArray[2]);
      stopFlag = 0;
    }
    // sprintf(sendBuf, "[%s]%s@%s\n", pArray[0], pArray[1], pArray[2]);
  } 
  else if (!strcmp(pArray[1], "BUZZ")) {
    if (!strcmp(pArray[2], "ON")) {
      digitalWrite(BUZZER_PIN, HIGH);
      sprintf(sendBuf, "[%s]%s@%s\n", pArray[0], pArray[1], pArray[2]);
      //stopFlag = 1;
    } else if (!strcmp(pArray[2], "OFF")) {
      digitalWrite(BUZZER_PIN, LOW);
      sprintf(sendBuf, "[%s]%s@%s\n", pArray[0], pArray[1], pArray[2]);
      //stopFlag = 0;
    }
  } 
  else if (!strncmp(pArray[1], " New", 4))  // New Connected
  {
    return;
  } else if (!strncmp(pArray[1], " Alr", 4))  //Already logged
  {
    return;
  }

#ifdef DEBUG
  Serial.print("Send : ");
  Serial.print(sendBuf);
#endif
  BTSerial.write(sendBuf);
}
void timerIsr() {
  timerIsrFlag = true;
  secCount++;
}
void lcdDisplay(int x, int y, char* str) {
  int len = 16 - strlen(str);
  lcd.setCursor(x, y);
  lcd.print(str);
  for (int i = len; i > 0; i--)
    lcd.write(' ');
}
