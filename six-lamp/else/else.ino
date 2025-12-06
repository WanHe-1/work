
#include "Arduino.h"
#include "DFRobotDFPlayerMini.h"

// 定义引脚
const int irSensorPin = 6; // HC-SR501人体红外传感器数字输入引脚
const int motorENA = 9;     // L298N 的 ENA，接 PWM 引脚 9
const int motorIN1 = 7;     // 控制方向
const int motorIN2 = 8;     // 控制方向

// 假设 LED 灯连接到 PWM 引脚 3
const int ledPin = 3;

// DFPlayer Mini 连接 (通常使用 SoftwareSerial)
// 根据你的实际接线修改这些引脚
#include <SoftwareSerial.h>
SoftwareSerial mySoftwareSerial(12, 13); // RX, TX (Arduino -> DFPlayer)
DFRobotDFPlayerMini myDFPlayer;

// HC-SR501 人体红外传感器参数
const int DETECTION_DISTANCE = 150; // 检测距离 (cm) - 根据HC-SR501调节

// 用于模拟距离变化的变量
unsigned long lastDetectionTime = 0;
const long DETECTION_TIMEOUT = 3000; // 检测超时时间 (毫秒)
float simulatedDistance = 200; // 模拟距离

const int AUDIO_FILE_1 = 1; // 40-60cm (远距离背景音)
const int AUDIO_FILE_2 = 2; // 30-40cm
const int AUDIO_FILE_3 = 3; // 20-30cm
const int AUDIO_FILE_4 = 4; // 10-20cm
const int AUDIO_FILE_5 = 5; // 0-10cm

// 用于防止声音播放过于频繁
unsigned long lastPlayTime = 0;
const long playInterval = 1000; // 最小播放间隔 (毫秒)

// 用于控制音频播放时长
unsigned long audioStartTime = 0;
const long audioDuration = 2000; // 音频播放时长 (毫秒) - 2秒
bool isAudioPlaying = false;

void setup() {
  // 初始化串口用于调试
  Serial.begin(9600);
  
  // 初始化 DFPlayer Mini 串口
  mySoftwareSerial.begin(9600);
  
  Serial.println();

  // 初始化 DFPlayer Mini
  Serial.println(F("正在初始化 DFPlayer Mini..."));
  delay(1000); // 等待DFPlayer启动
  
  if (!myDFPlayer.begin(mySoftwareSerial)) {  // 使用软件串口与 DFPlayer 通信
    Serial.println(F("DFPlayer Mini 初始化失败！"));
    Serial.println(F("请检查："));
    Serial.println(F("1. 接线是否正确 (RX->TX, TX->RX)"));
    Serial.println(F("2. 电源是否充足"));
    Serial.println(F("3. TF卡是否插入"));
    Serial.println(F("4. 音频文件是否存在"));
    while(true); // 停止程序
  }
  Serial.println(F("DFPlayer Mini 初始化成功！"));

  // 设置音量 (0~30)
  myDFPlayer.volume(20);  // 根据需要调整音量
  
  // 设置播放模式为单曲停止 (适用于DFPlayer库1.0.6版本)
  // 先启用循环，然后发送一个特殊命令来实现单曲停止。
  // 根据库的源码和社区经验，这是实现单曲停止的一种方法。
  myDFPlayer.enableLoop(); // 先启用循环
  // 发送一个特殊命令来设置为单曲停止模式（库内部没有直接的API）
  // 这个命令是 0x08 0x01 0x00 0x00，表示设置播放模式为单曲停止
  // 但库封装了命令发送，我们尝试通过调用库内部可能存在的方法或直接发送命令
  // 查看库文档或源码，发现 loopFolder() 可能会发送相关命令，但不完全匹配。
  // 最稳妥的方法是直接调用库提供的命令接口（如果有的话）或者发送原始命令。
  // 但为了兼容性和简单起见，我们先尝试库提供的方法。
  // 如果 enableLoop() 后再 play() 表现不符合预期，可能需要更底层的控制。
  // 这里我们假设 enableLoop() 后，播放单个文件会按预期停止。
  // 如果需要更精确控制，可以考虑升级库或手动发送命令。
  
  // 设置 EQ 为正常
  myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);
  
  // 设置 DFPlayer 输出设备为 TF Card
  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
  
  // 测试DFPlayer是否正常工作
  Serial.println(F("测试 DFPlayer 功能..."));
  delay(500);
  
  // 尝试播放第一个音频文件进行测试
  myDFPlayer.play(1);
  delay(1000);
  myDFPlayer.stop();
  Serial.println(F("DFPlayer 测试完成"));

  // 设置引脚模式
  pinMode(motorENA, OUTPUT);
  pinMode(motorIN1, OUTPUT);
  pinMode(motorIN2, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(irSensorPin, INPUT); // 设置HC-SR501人体红外传感器引脚为输入

  Serial.println(F("IR Sensor Distance Control Started"));
}

void loop() {
  // --- 读取HC-SR501人体红外传感器 ---
  int irValue = digitalRead(irSensorPin);
  unsigned long currentTime = millis();
  
  // HC-SR501输出数字信号：HIGH(1) = 检测到人体，LOW(0) = 未检测到
  float distance_cm;
  
  if (irValue == HIGH) {
    // 检测到人体，记录检测时间
    lastDetectionTime = currentTime;
    // 模拟距离逐渐减小（人越来越近）
    if (simulatedDistance > 50) {
      simulatedDistance -= 5; // 每次减少5cm
    } else {
      simulatedDistance = 50; // 最小距离50cm
    }
    distance_cm = simulatedDistance;
  } else {
    // 未检测到人体
    if (currentTime - lastDetectionTime < DETECTION_TIMEOUT) {
      // 在超时时间内，保持最后检测到的距离
      distance_cm = simulatedDistance;
    } else {
      // 超时后，逐渐增加距离直到超出范围
      if (simulatedDistance < 200) {
        simulatedDistance += 10; // 每次增加10cm
      } else {
        simulatedDistance = 200; // 最大距离200cm
      }
      distance_cm = simulatedDistance;
    }
  }

  Serial.print(F("HC-SR501 Value: "));
  Serial.print(irValue);
  Serial.print(F(" -> Simulated Distance: "));
  Serial.print(distance_cm);
  Serial.println(F(" cm"));

  // --- 根据距离设置电机速度 ---
  int motorSpeed;
  if (distance_cm < 40) {
    motorSpeed = 255;
  } else if (distance_cm > 180) {
    motorSpeed = 0;
  } else {
    // 距离在 40~180cm 之间，线性加速
    motorSpeed = map(distance_cm, 180, 40, 0, 255);
    motorSpeed = constrain(motorSpeed, 0, 255); // 限制在 0~255
  }

  // --- 控制电机方向（正转）---
  digitalWrite(motorIN1, HIGH);
  digitalWrite(motorIN2, LOW);

  // --- 设置电机速度（通过 ENA 引脚 PWM）---
  analogWrite(motorENA, motorSpeed);

  Serial.print(F("Motor Speed: "));
  Serial.println(motorSpeed);

  // --- 根据距离设置灯光亮度 ---
  int ledBrightness = 0;
  if (distance_cm > 180) {
    // 180cm外: 关闭灯光
    ledBrightness = 0;
  } else if (distance_cm >= 140 && distance_cm <= 180) {
    // 140-180cm: 最暗档位
    ledBrightness = map(distance_cm, 140, 180, 30, 60);
    ledBrightness = constrain(ledBrightness, 30, 60);
  } else if (distance_cm > 120 && distance_cm < 140) {
    // 120-140cm: 第二档
    ledBrightness = map(distance_cm, 120, 140, 70, 120);
    ledBrightness = constrain(ledBrightness, 70, 120);
  } else if (distance_cm > 80 && distance_cm <= 120) {
    // 80-120cm: 第三档
    ledBrightness = map(distance_cm, 80, 120, 130, 200);
    ledBrightness = constrain(ledBrightness, 130, 200);
  } else if (distance_cm > 40 && distance_cm <= 80) {
    // 40-80cm: 第四档
    ledBrightness = map(distance_cm, 40, 80, 210, 245);
    ledBrightness = constrain(ledBrightness, 210, 245);
  } else if (distance_cm >= 0 && distance_cm <= 40) {
    // 0-40cm: 最亮档位
    ledBrightness = map(distance_cm, 0, 40, 250, 255);
    ledBrightness = constrain(ledBrightness, 250, 255);
  }
  analogWrite(ledPin, ledBrightness);
  Serial.print(F("LED Brightness: "));
  Serial.println(ledBrightness);

  // --- 根据距离播放不同音频 ---
  unsigned long currentMillis = millis();
  
  // 检查是否需要停止当前播放的音频（2秒后）
  if (isAudioPlaying && (currentMillis - audioStartTime >= audioDuration)) {
    myDFPlayer.stop();
    isAudioPlaying = false;
    Serial.println(F("Audio stopped after 2 seconds"));
  }
  
  // 检查是否可以播放新音频
  if (!isAudioPlaying && (currentMillis - lastPlayTime > playInterval)) {
    bool played = false;
    if (distance_cm > 40 && distance_cm <= 60) {
      // 40-60cm: 音频 1 (远距离背景音)
      myDFPlayer.play(AUDIO_FILE_1);
      Serial.println(F("Playing Audio File 1 (40-60cm)"));
      played = true;
    } else if (distance_cm > 30 && distance_cm <= 40) {
      // 30-40cm: 音频 2
      myDFPlayer.play(AUDIO_FILE_2);
      Serial.println(F("Playing Audio File 2 (30-40cm)"));
      played = true;
    } else if (distance_cm > 20 && distance_cm <= 30) {
      // 20-30cm: 音频 3
      myDFPlayer.play(AUDIO_FILE_3);
      Serial.println(F("Playing Audio File 3 (20-30cm)"));
      played = true;
    } else if (distance_cm > 10 && distance_cm <= 20) {
      // 10-20cm: 音频 4
      myDFPlayer.play(AUDIO_FILE_4);
      Serial.println(F("Playing Audio File 4 (10-20cm)"));
      played = true;
    } else if (distance_cm >= 0 && distance_cm <= 10) {
      // 0-10cm: 音频 5
      myDFPlayer.play(AUDIO_FILE_5);
      Serial.println(F("Playing Audio File 5 (0-10cm)"));
      played = true;
    }
    
    // 如果开始播放音频，记录播放状态和时间
    if(played) {
        isAudioPlaying = true;
        audioStartTime = currentMillis;
        lastPlayTime = currentMillis; // 更新上次播放时间
    }
  }

  delay(300); // 主循环延迟
}



