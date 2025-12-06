#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"

MAX30105 particleSensor;

// 引脚
const int WATER_PUMP_PIN = 9;
const int SMOKE_SENSOR_PWR_PIN = 8; // 给烟雾传感器供电的控制引脚

// 烟雾传感器改为供电控制，无需阈值

// 心率滑动平均
const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute = 0;
int beatAvg = 0;

// 状态：是否已检测到心率并激活系统
bool systemActivated = false;

void setup() {
  Serial.begin(115200);

  pinMode(WATER_PUMP_PIN, OUTPUT);
  digitalWrite(WATER_PUMP_PIN, LOW);
  pinMode(SMOKE_SENSOR_PWR_PIN, OUTPUT);
  digitalWrite(SMOKE_SENSOR_PWR_PIN, LOW);

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    while (1);
  }
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);
}

void loop() {
  long irValue = particleSensor.getIR();

  if (checkForBeat(irValue)) {
    long delta = millis() - lastBeat;
    lastBeat = millis();
    float bpm = 60.0 / (delta / 1000.0);
    if (bpm > 20 && bpm < 220) {
      rates[rateSpot++] = (byte)bpm;
      rateSpot %= RATE_SIZE;
      int sum = 0;
      for (byte i = 0; i < RATE_SIZE; i++) sum += rates[i];
      beatAvg = sum / RATE_SIZE;
      beatsPerMinute = beatAvg;
    }
  }

  // 烟雾传感器改为仅供电控制，不再读取模拟量

  static unsigned long lastOut = 0;
  if (millis() - lastOut > 100) {
    lastOut = millis();
    if (irValue < 50000) {
      Serial.print(" 未识别?");
      Serial.println();
    } else {
      Serial.println((int)beatsPerMinute);
      if (!systemActivated) {
        systemActivated = true;
        digitalWrite(WATER_PUMP_PIN, HIGH);
        digitalWrite(SMOKE_SENSOR_PWR_PIN, HIGH);
        Serial.print(" 1?");
      }
    }
  }
}
