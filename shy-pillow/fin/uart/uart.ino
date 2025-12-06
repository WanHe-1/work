#include "Arduino.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

// 引脚定义
#define DFPLAYER_RX 12  // DFPlayer TX -> Arduino RX
#define DFPLAYER_TX 13  // DFPlayer RX -> Arduino TX
#define L298N_ENA 7     // 电机使能A
#define L298N_ENB 2     // 电机使能B
#define L298N_IN1 5     // 电机控制1
#define L298N_IN2 6     // 电机控制2
#define L298N_IN3 4     // 电机控制3
#define L298N_IN4 3     // 电机控制4
#define LED_PIN 11      // LED控制引脚

// 创建DFPlayer Mini对象
SoftwareSerial mySoftwareSerial(DFPLAYER_RX, DFPLAYER_TX);
DFRobotDFPlayerMini myDFPlayer;

// 全局变量
long lastDistance = 0;
long lastConfidence = 0;
unsigned long lastAudioTime = 0;
const unsigned long AUDIO_DELAY = 1000; // 音频播放间隔1秒

// 音频文件映射
const int AUDIO_FILE_1 = 1; // 40-60cm
const int AUDIO_FILE_2 = 2; // 30-40cm
const int AUDIO_FILE_3 = 3; // 20-30cm
const int AUDIO_FILE_4 = 4; // 10-20cm
const int AUDIO_FILE_5 = 5; // 0-10cm

void setup() {
  // 初始化串口
  Serial.begin(115200);
  while (!Serial) {
    ; // 等待串口连接
  }
  
  Serial.println("Starting setup...");
  
  // 初始化L298N控制引脚
  pinMode(L298N_ENA, OUTPUT);
  pinMode(L298N_ENB, OUTPUT);
  pinMode(L298N_IN1, OUTPUT);
  pinMode(L298N_IN2, OUTPUT);
  pinMode(L298N_IN3, OUTPUT);
  pinMode(L298N_IN4, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  
  // 设置L298N输入为低电平（灯默认关闭）
  digitalWrite(L298N_IN1, LOW);
  digitalWrite(L298N_IN2, LOW);
  digitalWrite(L298N_IN3, LOW);
  digitalWrite(L298N_IN4, LOW);
  analogWrite(LED_PIN, 0);
  
  Serial.println("L298N pins initialized.");
  
  // 初始化DFPlayer Mini
  Serial.println("Initializing DFPlayer...");
  mySoftwareSerial.begin(9600);
  
  if (!myDFPlayer.begin(mySoftwareSerial)) {
    Serial.println("Unable to begin DFPlayer Mini:");
    Serial.println("1. Please recheck the connection!");
    Serial.println("2. Please insert the SD card!");
    while(true);
  }
  
  Serial.println("DFPlayer Mini online.");
  myDFPlayer.volume(25); // 设置音量 (0~30)
  
  Serial.println("System initialized successfully!");
  Serial.println("=== 激光传感器距离读取程序 ===");
  Serial.println("等待距离数据...");
  Serial.println("数据格式应为: 距离,置信度 (例如: 1500,95)");
  Serial.println("----------------------------------------");
}

void loop() {
  if (Serial.available() > 0) {
    // 读取一行数据 (以换行符结束)
    String data = Serial.readStringUntil('\n');
    
    // 解析数据
    parseDistanceData(data);
    
    // 更新灯光和音频
    if(lastDistance != 0 && lastConfidence != 0) {
      Serial.print("Updating with distance: ");
      Serial.print(lastDistance);
      Serial.print(" mm, confidence: ");
      Serial.println(lastConfidence);
      
      updateLights(lastDistance);
      updateAudio(lastDistance);
    } else {
      Serial.println("No valid distance data to update with.");
    }
  } else {
    // 每隔2秒输出一次状态信息，帮助调试
    static unsigned long lastStatusTime = 0;
    if(millis() - lastStatusTime > 2000) {
      Serial.println("等待距离数据...");
      lastStatusTime = millis();
    }
  }
}

void parseDistanceData(String &data) {
  // 去除首尾空白字符
  data.trim();
  
  // 查找逗号位置
  int commaPos = data.indexOf(',');
  if (commaPos == -1) {
    Serial.print("数据格式错误 (缺少逗号): ");
    Serial.println(data);
    return;
  }
  
  // 提取距离
  String distanceStr = data.substring(0, commaPos);
  distanceStr.trim();
  
  // 提取置信度
  String confidenceStr = data.substring(commaPos + 1);
  confidenceStr.trim();
  
  // 转换为整数
  long distance = distanceStr.toInt();  
  long confidence = confidenceStr.toInt();
  
  if ((distance <=20 || distance>=4000) && distanceStr != "0") {
    Serial.print("距离超出有效范围: ");
    Serial.println(distance);
    return;
  }
  if (confidence == 0 && confidenceStr != "0") {
    Serial.print("置信度为0但数据非零: ");
    Serial.println(confidence);
    return;
  }
  
  // 保存解析结果到全局变量
  lastDistance = distance;
  lastConfidence = confidence;
  
  // 输出结果
  Serial.print("Distance:");
  Serial.print(distance);
  Serial.print(" mm, Confidence: ");
  Serial.println(confidence);
}

void updateLights(long distance) {
  // 将距离从毫米转换为厘米
  int distanceCm = distance / 10;
  int pwmValue = 0;
  int ledBrightness = 0;
  
  // L298N控制的灯光亮度
  if(distanceCm >= 140 && distanceCm <= 180) {
    // 挡位1: 140-180cm
    pwmValue = map(distanceCm, 140, 180, 255, 50);
  } else if(distanceCm >= 120 && distanceCm < 140) {
    // 挡位2: 120-140cm
    pwmValue = map(distanceCm, 120, 140, 255, 100);
  } else if(distanceCm >= 80 && distanceCm < 120) {
    // 挡位3: 80-120cm
    pwmValue = map(distanceCm, 80, 120, 255, 150);
  } else if(distanceCm >= 40 && distanceCm < 80) {
    // 挡位4: 40-80cm
    pwmValue = map(distanceCm, 40, 80, 255, 200);
  } else if(distanceCm >= 0 && distanceCm < 40) {
    // 挡位5: 0-40cm
    pwmValue = 255; // 最亮
  } else {
    // 距离超出范围，关闭灯光
    pwmValue = 0;
  }
  
  // 限制PWM值在0-255范围内
  if(pwmValue < 0) pwmValue = 0;
  if(pwmValue > 255) pwmValue = 255;
  
  // 设置L298N PWM值控制灯光亮度
  analogWrite(L298N_ENA, pwmValue);
  analogWrite(L298N_ENB, pwmValue);
  
  // 确保使能端为高电平以便PWM生效
  digitalWrite(L298N_IN1, HIGH);
  digitalWrite(L298N_IN2, HIGH);
  digitalWrite(L298N_IN3, HIGH);
  digitalWrite(L298N_IN4, HIGH);
  
  // 单独LED控制
  if(distanceCm > 180) {
    ledBrightness = 0;
  } else if(distanceCm >= 140) {
    ledBrightness = map(distanceCm, 140, 180, 40, 80);
    ledBrightness = constrain(ledBrightness, 40, 80);
  } else if(distanceCm > 120) {
    ledBrightness = map(distanceCm, 120, 140, 80, 120);
    ledBrightness = constrain(ledBrightness, 80, 120);
  } else if(distanceCm > 80) {
    ledBrightness = map(distanceCm, 80, 120, 120, 180);
    ledBrightness = constrain(ledBrightness, 120, 180);
  } else if(distanceCm > 40) {
    ledBrightness = map(distanceCm, 40, 80, 180, 220);
    ledBrightness = constrain(ledBrightness, 180, 220);
  } else {
    ledBrightness = map(distanceCm, 0, 40, 220, 255);
    ledBrightness = constrain(ledBrightness, 220, 255);
  }
  
  analogWrite(LED_PIN, ledBrightness);
  
  Serial.print("L298N PWM: ");
  Serial.print(pwmValue);
  Serial.print(", LED Brightness: ");
  Serial.println(ledBrightness);
}

void updateAudio(long distance) {
  int distanceCm = distance / 10;
  unsigned long currentTime = millis();
  
  // 检查是否已过音频播放延迟时间
  if(currentTime - lastAudioTime < AUDIO_DELAY) {
    return;
  }
  
  int audioTrack = 0;
  
  if(distanceCm > 40 && distanceCm <= 60) {
    // 40-60cm 对应音频1
    audioTrack = AUDIO_FILE_1;
  } else if(distanceCm > 30 && distanceCm <= 40) {
    // 30-40cm 对应音频2
    audioTrack = AUDIO_FILE_2;
  } else if(distanceCm > 20 && distanceCm <= 30) {
    // 20-30cm 对应音频3
    audioTrack = AUDIO_FILE_3;
  } else if(distanceCm > 10 && distanceCm <= 20) {
    // 10-20cm 对应音频4
    audioTrack = AUDIO_FILE_4;
  } else if(distanceCm <= 10) {
    // 0-10cm 对应音频5
    audioTrack = AUDIO_FILE_5;
  }
  
  if(audioTrack > 0) {
    Serial.print("Playing audio track: ");
    Serial.println(audioTrack);
    myDFPlayer.play(audioTrack);
    lastAudioTime = currentTime;
  }
}



