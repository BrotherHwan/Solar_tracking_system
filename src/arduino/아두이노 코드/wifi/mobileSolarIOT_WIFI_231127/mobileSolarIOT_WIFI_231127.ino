/*
  WiFiEsp test: ClientTest
  http://www.kccistc.net/
  SmartESS
  작성일 : 2023.11.17
  작성자 : KWSHHW

*/
#define DEBUG

#define IR_FUNC     1

#define SERVO_THRSH 30

//#define DEBUG_WIFI

#include <WiFiEsp.h>
#include <SoftwareSerial.h>
#include <MsTimer2.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>


#define AP_SSID "SEMICON_2.4G"
// #define AP_SSID "SEMICON_5G"
#define AP_PASS "a1234567890"
#define SERVER_NAME "10.10.52.214"
#define SERVER_PORT 5000
#define LOGID "ESS_WIFI"
#define PASSWD "PASSWD"


#define CDS0_PIN A0
#define CDS1_PIN A1
#define CDS2_PIN A2
#define CDS3_PIN A3

#define VIN_PIN A5

#define BUZZER_PIN 8

#define SERVO_TILT_PIN 3
#define SERVO_PAN_PIN 5

#define WIFIRX 6  //6:RX-->ESP8266 TX
#define WIFITX 7  //7:TX -->ESP8266 RX

#define IRSENSOR_PIN 12
#define LED_BUILTIN_PIN 13

#define BUZZ_PIN_BT 10
#define STOP_PIN_BT 11




#define CMD_SIZE 50
#define ARR_CNT 5

#define CDS0_OFFSET 0
#define CDS1_OFFSET 80
#define CDS2_OFFSET 5
#define CDS3_OFFSET 65


bool timerIsrFlag = false;
int buttonState;            // the current reading from the input pin
int lastButtonState = LOW;  // the previous reading from the input pin
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50; 
boolean ledOn = false;      // LED의 현재 상태 (on/off)
boolean cdsFlag = false;

//char sendId[10] = "KSH_ARD";
char sendBuf[CMD_SIZE];


int absolute(int num);

int cds_signal_pin0 = CDS0_PIN;
int cds_signal_pin1 = CDS1_PIN;
int cds_signal_pin2 = CDS2_PIN;
int cds_signal_pin3 = CDS3_PIN;
int solar_adc = VIN_PIN;

int change_val1 = 60;
int change_val2 = 90;

int val0;
int val1;
int val2;
int val3;
int val5;

int pan;
int tilt;

int distance = 22;
int irSensor = 0;

enum _servo{
  STOP_MODE = 0,
  AUTO_MODE,
  MANUAL_MODE
};

unsigned int servoMode = AUTO_MODE;
unsigned int panLevel;
unsigned int tiltLevel;

unsigned int secCount;
unsigned int myservoTime = 0;



char getSensorId[10];
int sensorTime;

bool updatTimeFlag = false;


SoftwareSerial wifiSerial(WIFIRX, WIFITX);
WiFiEspClient client;

Servo myservo1;
Servo myservo2;

void setup() {

  pinMode(CDS0_PIN, INPUT);    
  pinMode(CDS1_PIN, INPUT);    
  pinMode(CDS2_PIN, INPUT);    
  pinMode(CDS3_PIN, INPUT);

  pinMode(SERVO_PAN_PIN, OUTPUT);    // SERVI 
  pinMode(SERVO_TILT_PIN, OUTPUT);    // 

  pinMode(IRSENSOR_PIN, INPUT); //
  pinMode(BUZZER_PIN, OUTPUT); //
  pinMode(LED_BUILTIN_PIN, OUTPUT); //D13

  pinMode(STOP_PIN_BT, INPUT);
  pinMode(BUZZ_PIN_BT, INPUT);

#ifdef DEBUG
  Serial.begin(115200); //DEBUG
#endif
  wifi_Setup();

  MsTimer2::set(1000, timerIsr); // 1000ms period
  MsTimer2::start();

  // myservo1.attach(SERVO_PAN_PIN);
  // myservo2.attach(SERVO_TILT_PIN);

  myservo1.write(60);
  myservo2.write(90);

  myservoTime = secCount;
}

void loop() {
  
  if (client.available()) {
    socketEvent();
  }

  if (timerIsrFlag) //1초에 한번씩 실행
  {
    //===================IRSENSOR=======================
    #if IR_FUNC
    irSensor = digitalRead(IRSENSOR_PIN);
    if (irSensor == 1) digitalWrite(BUZZER_PIN, HIGH);
    else digitalWrite(BUZZER_PIN, LOW);
    #endif
    //==================================================

    //===================Maual BUZZ BT==================
    int buzzManBt = digitalRead(BUZZ_PIN_BT);
    if (buzzManBt == 1) digitalWrite(BUZZER_PIN, HIGH);

    //===================Maual STOP BT==================
    int emergencyStop = digitalRead(STOP_PIN_BT);
    //==================================================

    //===================Read ADC Value=================
    val0 = analogRead(cds_signal_pin0) - CDS0_OFFSET; // left top
    val1 = analogRead(cds_signal_pin1) - CDS1_OFFSET; // right top
    val2 = analogRead(cds_signal_pin2) - CDS2_OFFSET; // left bottom
    val3 = analogRead(cds_signal_pin3) - CDS3_OFFSET; // right bottom

    val5 = analogRead(solar_adc);
    val5 = map(val5, 0 , 1023 ,0, 500) + 500;

    int sumTop = val0 + val1;
    int sumBottom = val2 + val3 ;
    int sumLeft = val0 + val2;
    int sumRight = val1 + val3;

    int diffTopBottom = sumTop - sumBottom;
    int diffLeftRight = sumLeft - sumRight;

    if (emergencyStop==0){
      if ((-1 * SERVO_THRSH) > diffTopBottom || diffTopBottom > SERVO_THRSH ) 
      { 
        if (sumTop < sumBottom) {
          change_val1 = change_val1 - 2;
          if (change_val1 > 120) {
            change_val1 = 120;
          }
        }
        else if (sumTop > sumBottom) {
          change_val1 = change_val1 + 2;
          if (change_val1 < 0) {
            change_val1 = 0;
          }
        }
        if(servoMode == AUTO_MODE)
        {
          myservo1.attach(SERVO_TILT_PIN);
          delay(15);
          myservo1.write(change_val1);
          delay(15);
          myservo1.detach();
        }
      }
      if (-1 * SERVO_THRSH > diffLeftRight || diffLeftRight > SERVO_THRSH){ 
        if (sumLeft < sumRight) {
          change_val2 = change_val2 + 2;
          if (change_val2 > 180) {
            change_val2 = 180;
          }
        }
        else if (sumLeft > sumRight) {
          change_val2 = change_val2 - 2;
          if (change_val2 < 0) {
            change_val2 = 0;
          }
        }
        if(servoMode == AUTO_MODE){
          myservo2.attach(SERVO_PAN_PIN);
          delay(15);
          myservo2.write(change_val2);
          delay(15);
          myservo2.detach();
        }
      }
    }

      timerIsrFlag = false;
      if (!(secCount % 5)) //5초에 한번씩 실행
      {
        if (!client.connected()) {
          server_Connect();
        }
      }
      if (sensorTime != 0 && !(secCount % sensorTime ))
      {
        //sprintf(sendBuf, "[%s]SENSOR@%d@%d@%d@%d@%d@%d\r\n", getSensorId, Vout, cds0, cds1, cds2, cds3, distance);
        sprintf(sendBuf, "[%s]SENSOR@%d@%d@%d@%d@%d@%d@%d@%d\r\n", getSensorId, val5, val0, val1, val2, val3, change_val2,change_val1, irSensor);
        client.write(sendBuf, strlen(sendBuf));
        client.flush();
      }
      if (updatTimeFlag)
      {
        client.print("[GETTIME]\n");
        updatTimeFlag = false;
      }
  }
}
void socketEvent()
{
  int i = 0;
  char * pToken;
  char * pArray[ARR_CNT] = {0};
  char recvBuf[CMD_SIZE] = {0};
  int len;

  sendBuf[0] = '\0';
  len = client.readBytesUntil('\n', recvBuf, CMD_SIZE-1);
  client.flush();
#ifdef DEBUG
  Serial.print("recv : ");
  Serial.print(recvBuf);
#endif
  pToken = strtok(recvBuf, "[@]");
  while (pToken != NULL)
  {
    pArray[i] =  pToken;
    if (++i >= ARR_CNT)
      break;
    pToken = strtok(NULL, "[@]");
  }
  //[KSH_ARD]LED@ON : pArray[0] = "KSH_ARD", pArray[1] = "LED", pArray[2] = "ON"

  if (!strncmp(pArray[1], " New", 4)) // New Connected
  {
#ifdef DEBUG
    Serial.write('\n');
#endif
    updatTimeFlag = true;
    return ;
  }
  else if (!strncmp(pArray[1], " Alr", 4)) //Already logged
  {
#ifdef DEBUG
    Serial.write('\n');
#endif
    client.stop();
    server_Connect();
    return ;
  }

  //====================SERVO Control=============================
  else if (!strcmp(pArray[1], "PAN"))
  {
      myservo1.attach(SERVO_PAN_PIN);
      delay(15);
      panLevel = atoi(pArray[2]);
      myservo1.write(panLevel);
      delay(15);
      myservo1.detach();
      servoMode = MANUAL_MODE;
      sprintf(sendBuf, "[%s]%s@%s\n", pArray[0], pArray[1], pArray[2]);
  }
  else if (!strcmp(pArray[1], "TILT"))
  {
      myservo2.attach(SERVO_TILT_PIN);
      delay(15);
      tiltLevel = atoi(pArray[2]);
      myservo2.write(tiltLevel);
      delay(15);
      myservo2.detach();
      servoMode = MANUAL_MODE;
      sprintf(sendBuf, "[%s]%s@%s\n", pArray[0], pArray[1], pArray[2]);
  }
  else if (!strcmp(pArray[1], "AUTO"))
  {
      servoMode = AUTO_MODE;
      sprintf(sendBuf, "[%s]%s\n", pArray[0], pArray[1]);
  }

  //=============================================================


  //=================SENSOR DATA Tx==============================

  else if (!strncmp(pArray[1], "GETSENSOR", 9)) {
    if (pArray[2] != NULL) {
      sensorTime = atoi(pArray[2]);
      strcpy(getSensorId, pArray[0]);
      return;
    } else {
      sensorTime = 0;
      sprintf(sendBuf, "[%s]%s@%d@%d@%d@%d@%d@%d@%d@%d\r\n", pArray[0], pArray[1], val5, val0, val1, val2, val3, change_val2,change_val1,irSensor);
    }
  }

  //====================LED Control==============================

  else
    return;

  client.write(sendBuf, strlen(sendBuf));
  client.flush();


#ifdef DEBUG
  Serial.print(", send : ");
  Serial.print(sendBuf);
#endif
}
void timerIsr()
{
  timerIsrFlag = true;
  secCount++;

}

void wifi_Setup() {
  wifiSerial.begin(19200);
  wifi_Init();
  server_Connect();
}
void wifi_Init()
{
  do {
    WiFi.init(&wifiSerial);
    if (WiFi.status() == WL_NO_SHIELD) {
#ifdef DEBUG_WIFI
      Serial.println("WiFi shield not present");
#endif
    }
    else
      break;
  } while (1);

#ifdef DEBUG_WIFI
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(AP_SSID);
#endif
  while (WiFi.begin(AP_SSID, AP_PASS) != WL_CONNECTED) {
#ifdef DEBUG_WIFI
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(AP_SSID);
#endif
  }

#ifdef DEBUG_WIFI
  Serial.println("You're connected to the network");
  printWifiStatus();
#endif
}
int server_Connect()
{
#ifdef DEBUG_WIFI
  Serial.println("Starting connection to server...");
#endif

  if (client.connect(SERVER_NAME, SERVER_PORT)) {
#ifdef DEBUG_WIFI
    Serial.println("Connect to server");
#endif
    client.print("["LOGID":"PASSWD"]");   
  }
  else
  {
#ifdef DEBUG_WIFI
    Serial.println("server connection failure");
#endif
  }
}
void printWifiStatus()
{
  // print the SSID of the network you're attached to

  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength
  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

int absolute(int num)
{
    if (num < 0) 
    {
        return -num;
    } 
    else 
    {
        return num;
    }
}