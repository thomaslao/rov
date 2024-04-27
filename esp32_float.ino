#include <BluetoothSerial.h>
#include <WiFi.h>
#include "time.h"
#include "Timer.h"
#include <ezButton.h>
#include <Wire.h>
#include "MS5837.h"

MS5837 sensor;
ezButton button(15);
ezButton button1(2);
//int pin = 13;
const char* ssid     = "Godfrey's iPhone"; // WiFi名稱
const char* password = "10031003";       // WiFi密碼
const int pin1 = 16; // 紅色板IN1 pin所接的ESP32 pin
const int pin2 = 17; // 紅色板IN2 pin所接的ESP32 pin
//const int pin3 = 15; // limit switch所接的ESP32 pin
//const int pin4 = 2;
float a;
float depth;
String d;
// 獲取時間設定(不用改)
const char* ntpServer = "pool.ntp.org";
const int  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;
BluetoothSerial BT;
struct tm timeinfoTmp; // 建立時間結構
Timer t;           // 建立計時物件
void time() {
  getTime();
}
void setup() {
  // 串口初始化(Debug用)
  Serial.begin(9600);
  Serial.println("Starting");
  Wire.begin();
  // 針腳初始化
  pinMode(pin1, OUTPUT);
  pinMode(pin2, OUTPUT);
  //limitswitch初始化
  button.setDebounceTime(50);
  button1.setDebounceTime(50);
  // 藍牙初始化
  BT.begin("PCMSROV.1");
  Serial.println("Setup complete.");
  // WiFi初始化
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  // 使計時器每秒執行一次getTime()
  t.every(1000, getTime);
  //timer.every(1000, getTime);
  while (!sensor.init()) {
    Serial.println("Init failed!");
    Serial.println("Are SDA/SCL connected correctly?");
    Serial.println("Blue Robotics Bar30: White=SDA, Green=SCL");
    Serial.println("\n\n\n");
    delay(5000);
  }
  sensor.setFluidDensity(997);
}
void getTime() {
  // 如果WiFi已連線則獲取時間
  int max = 0, min = 0;
  if (WiFi.status() == WL_CONNECTED) { 
    a = sensor.pressure() * 20.0;
    depth = sensor.depth()+10;
    // 獲取時間的設定(不用改)
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    struct tm timeinfo;
    getLocalTime(&timeinfo);
    // 將結果通過藍牙串口傳送到電腦(記得加返team number)
    BT.println(&timeinfo,"%H:%M:%S UTC R4");
    BT.print(a);
    BT.print("      ");
    BT.println(depth);
    if (depth > max) {
      max = depth;
    }
    if (depth < min) {
      min = depth;
    }
    BT.print("Max depth: ");
    BT.print(max);
    BT.print(" ");
    BT.print("Min depth: ");
    BT.print(min);
    
  }
  else { 
    // 否則傳送空字串表示無改變
    BT.println("");
    if (depth > max) {
      max = depth;
    }
    if (depth < min) {
      min = depth;
    }
  }
}
void loop() {
  t.update(); // 每次Loop更新計時器(會自動平衡刷新率，使getTime()保持在一秒一次)
  String inputFromPC; // 每次Loop重置接收指令用的空字串
  button.loop();
  button1.loop(); 
  int btnState = button.getState();
  int btnState1 = button1.getState();
  sensor.read();

  Serial.print("Pressure: ");
  Serial.print(sensor.pressure()*20);
  Serial.println(" mbar");

  Serial.print("Temperature: ");
  Serial.print(sensor.temperature());
  Serial.println(" deg C");

  Serial.print("Depth: ");
  Serial.print(sensor.depth()+10);
  Serial.println(" m");
  delay(1000);
  
  if(btnState==0){
    digitalWrite(pin1, LOW);    
  }if(btnState1==0){
    digitalWrite(pin2, LOW);
    delay(1000*15);
    digitalWrite(pin1, HIGH);
  }
  if (BT.available()) {
    // 當藍牙串口收到訊息時運行以下程式
    inputFromPC = BT.readString(); // 讀取文字訊息
    Serial.println(inputFromPC);   // 打印文字訊息(Debug用)
    // 以下指令對照電腦端，不作解釋
    if (inputFromPC == "push") {
      BT.println("Float is pushing!");
      digitalWrite(pin1, HIGH); // 將pin1改成高電位
      digitalWrite(pin2, LOW);  // 將pin2改成低電位
    } else if (inputFromPC == "pull") {
      BT.println("Float is pulling!");
      digitalWrite(pin1, LOW);
      digitalWrite(pin2, HIGH);
    } else if (inputFromPC == "dive") {
      BT.println("Float is diving!");
      digitalWrite(pin1, LOW);
      digitalWrite(pin2, HIGH);
    } else if (inputFromPC.indexOf("wifi") > -1) {
      // 以下為遠程控制連接WiFi的程式
      String tmp[3];
      // 將"wifi^HUAWEI P40 Pro^12345678以"^"分割成字串列表
      for (int i=0;i<3;i++) {
        int index = inputFromPC.lastIndexOf("^");
        int length = inputFromPC.length();
        tmp[i] = inputFromPC.substring(index+1, length);
        inputFromPC.remove(index, length);
      }
      BT.println("Float Connnecting to: " + tmp[1]);
      char new_ssid[20];     // 初始化長度為20字元的字符new_ssid
      char new_password[20]; // 初始化長度為20字元的字符new_password
      tmp[1].toCharArray(new_ssid, 20);     // 將字串轉換為字符
      tmp[0].toCharArray(new_password, 20); // 將字串轉換為字符
      Serial.println(new_ssid);     // Debug用，可Delete
      Serial.println(new_password); // Debug用，可Delete
      WiFi.disconnect();   // 重置WiFi連線
      WiFi.begin(new_ssid, new_password);  // 使用新SSID及密碼連接WiFi
      // 等待WiFi成功連接（20秒）
      for (int i=0;i<20;i++) {
        Serial.print(".");
        if (WiFi.status() == WL_CONNECTED) {
          BT.println("WiFi connected.");
          return;  // 成功時直接進入下一Loop
        }
        delay(1000);
      }
      // 成功時直接進入下一Loop，不會運行以下程式
      BT.println("Fail to connect WiFi.");
    } else if (inputFromPC != "") {
      BT.println("Invalid Action!");
    }
  }
}