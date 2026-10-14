#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"

MAX30105 particleSensor;

// MAX30102 
long lastBeat = 0;
float beatsPerMinute = 0;

// GSR 
const int LED = 13;
const int GSR = A0;
const int BUZZER = 8;

int threshold = 0;
int sensorValue = 0;

void setup()
{
  Serial.begin(115200);

  pinMode(LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  digitalWrite(LED, LOW);
  digitalWrite(BUZZER, LOW);

  // 初始化 MAX30102
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST))
  {
    while (1);
  }

  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);

  // GSR 校准：采集500次，计算初始基准值
  long sum = 0;

  for (int i = 0; i < 500; i++)
  {
    sensorValue = analogRead(GSR);
    sum += sensorValue;
    delay(10);
  }

  threshold = sum / 500;
}

void loop()
{
  //  IR 数值
  long irValue = particleSensor.getIR();

  // 心跳检测
  if (checkForBeat(irValue))
  {
    long delta = millis() - lastBeat;
    lastBeat = millis();

    if (delta > 0)
    {
      beatsPerMinute = 60000.0 / delta;

      if (beatsPerMinute < 255 && beatsPerMinute > 70)
      {
        digitalWrite(BUZZER, HIGH);
        delay(1000);
        digitalWrite(BUZZER, LOW);
      }
    }
  }

  // 读取 GSR 传感器 A0
  sensorValue = analogRead(GSR);

  // GSR 变化检测
  int temp = threshold - sensorValue;

  if (abs(temp) > 60)
  {
    digitalWrite(LED, HIGH);
  }
  else
  {
    digitalWrite(LED, LOW);
  }

  // 串口绘图仪：仅输出两列数值
  Serial.print("IR:");
  Serial.print(irValue);
  Serial.print("\tGSR:");
  Serial.println(sensorValue);

  delay(20);
}