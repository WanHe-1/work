// 灯光设置五个当为分别为在140cm-180cm 120cm-140cm 80cm-120cm 40cm-80cm 0-40cm，灯光亮度是随着距离的变短亮度逐渐变强直至255。声音设置4个音频，30-40cm 20-30cm 10-20cm 0-10cm分别对应着1 2 3 4个音频,使用的是dfplayermini外加喇叭的配置
// 距离从超声波传感器改用为红外传感器模块


#include "Arduino.h"

// 引脚定义
const int pirPin = 6;        // HC-SR501 接在 A6
const int motorENA = 9;       // 电机使能引脚（PWM）
const int motorIN1 = 7;
const int motorIN2 = 8;
const int ledPin = 3;         // LED 引脚（PWM）

// 状态变量
bool lastPirState = LOW;

// LED 渐变亮度变量
int targetBrightness = 0;
int currentBrightness = 0;

void setup() {
  Serial.begin(9600);

  pinMode(pirPin, INPUT);
  pinMode(motorENA, OUTPUT);
  pinMode(motorIN1, OUTPUT);
  pinMode(motorIN2, OUTPUT);
  pinMode(ledPin, OUTPUT);

  Serial.println(F("PIR Sensor Control Started (A6 pin)"));
}

void loop() {
  int pirState = digitalRead(pirPin);

  // 如果状态改变（有人/无人）
  if (pirState != lastPirState) {
    if (pirState == HIGH) {
      Serial.println(F("👤 Motion Detected!"));
      targetBrightness = 255;

      // 电机启动
      digitalWrite(motorIN1, HIGH);
      digitalWrite(motorIN2, LOW);
      analogWrite(motorENA, 255);

    } else {
      Serial.println(F("🛑 No motion"));
      targetBrightness = 0;

      // 电机停止
      analogWrite(motorENA, 0);
    }

    lastPirState = pirState;
  }

  // 平滑过渡 LED 亮度
  if (currentBrightness < targetBrightness) {
    currentBrightness++;
  } else if (currentBrightness > targetBrightness) {
    currentBrightness--;
  }
  analogWrite(ledPin, currentBrightness);

  delay(10); // 小延迟，实现渐变效果
}



