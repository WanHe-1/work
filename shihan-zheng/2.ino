#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"

#include <OneWire.h>
#include <DallasTemperature.h>

// ========================================
// MAX30102
// ========================================
MAX30105 particleSensor;

// ========================================
// DS18B20
// DATA 引脚 = D4
// ========================================
#define ONE_WIRE_BUS 4

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// ========================================
// 同步控制引脚
// D7 为 HIGH 时，D5 和 D6 同时为 HIGH
// ========================================
#define SYNC_PIN1 5
#define SYNC_PIN2 6

// ========================================
// 继电器
// IN 引脚 = D7
// ========================================
#define RELAY_PIN 7

// ========================================
// IR 阈值
// ========================================
#define IR_LEVEL_1 80000
#define IR_LEVEL_2 100000
#define IR_LEVEL_3 120000

// ========================================
// 输出时间
// ========================================
#define TIME_LEVEL_1 25000UL
#define TIME_LEVEL_2 45000UL
#define TIME_LEVEL_3 60000UL

// ========================================
// 输出控制变量
// ========================================
unsigned long outputStartTime = 0;
unsigned long outputDuration = 0;

bool outputActive = false;
int currentLevel = 0;

// ========================================
// 心率变量
// ========================================
long lastBeat = 0;
float beatsPerMinute = 0;
int beatAvg = 0;

// ========================================
// 温度变量
// ========================================
float temperature = 0;

// 温度读取定时
unsigned long lastTempRead = 0;
const unsigned long TEMP_INTERVAL = 1000;

// ========================================
// 开启输出
// ========================================
void startOutput(unsigned long duration, int level)
{
  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(SYNC_PIN1, HIGH);
  digitalWrite(SYNC_PIN2, HIGH);

  outputStartTime = millis();
  outputDuration = duration;

  outputActive = true;
  currentLevel = level;
}

// ========================================
// 关闭输出
// ========================================
void stopOutput()
{
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(SYNC_PIN1, LOW);
  digitalWrite(SYNC_PIN2, LOW);

  outputActive = false;
  currentLevel = 0;
}

// ========================================
// SETUP
// ========================================
void setup()
{
  Serial.begin(115200);

  delay(1000);

  // ----------------------------------------
  // 继电器和同步控制引脚
  // ----------------------------------------
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(SYNC_PIN1, OUTPUT);
  pinMode(SYNC_PIN2, OUTPUT);

  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(SYNC_PIN1, LOW);
  digitalWrite(SYNC_PIN2, LOW);

  // ----------------------------------------
  // I2C
  // ----------------------------------------
  Wire.begin();

  // ----------------------------------------
  // DS18B20
  // ----------------------------------------
  sensors.begin();

  // ----------------------------------------
  // MAX30102
  // ----------------------------------------
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST))
  {
    // 如果 MAX30102 初始化失败
    // 用 LED 状态表示错误
    while (1)
    {
      digitalWrite(RELAY_PIN, HIGH);
      delay(200);

      digitalWrite(RELAY_PIN, LOW);
      delay(200);
    }
  }

  // MAX30102 初始化
  particleSensor.setup();

  // 红灯亮度
  particleSensor.setPulseAmplitudeRed(0x0A);

  // IR LED 亮度
  particleSensor.setPulseAmplitudeIR(0x24);

  // 关闭绿色 LED
  particleSensor.setPulseAmplitudeGreen(0);

  delay(1000);
}

// ========================================
// LOOP
// ========================================
void loop()
{
  // ========================================
  // 读取 MAX30102 IR
  // ========================================
  long irValue = particleSensor.getIR();

  // ========================================
  // 心率检测
  // ========================================
  if (checkForBeat(irValue))
  {
    long delta = millis() - lastBeat;

    lastBeat = millis();

    beatsPerMinute = 60.0 / (delta / 1000.0);

    // 限制合理心率范围
    if (beatsPerMinute > 20 && beatsPerMinute < 255)
    {
      beatAvg = (beatAvg * 3 + (int)beatsPerMinute) / 4;
    }
  }

  // ========================================
  // 每 1 秒读取一次温度
  // ========================================
  if (millis() - lastTempRead >= TEMP_INTERVAL)
  {
    lastTempRead = millis();

    sensors.requestTemperatures();

    temperature = sensors.getTempCByIndex(0);
  }

  // ========================================
  // 判断输出是否结束
  // ========================================
  if (outputActive)
  {
    if (millis() - outputStartTime >= outputDuration)
    {
      stopOutput();
    }
  }

  // ========================================
  // IR 阈值判断
  // 只有当前没有输出时才能触发
  // ========================================
  if (!outputActive)
  {
    if (irValue >= IR_LEVEL_3)
    {
      // IR >= 120000
      // HIGH 60 秒
      startOutput(TIME_LEVEL_3, 3);
    }
    else if (irValue >= IR_LEVEL_2)
    {
      // IR >= 100000
      // HIGH 45 秒
      startOutput(TIME_LEVEL_2, 2);
    }
    else if (irValue >= IR_LEVEL_1)
    {
      // IR >= 80000
      // HIGH 25 秒
      startOutput(TIME_LEVEL_1, 1);
    }
  }

  // ========================================
  // 串口绘图仪输出
  // 固定输出 4 组数据
  // ========================================

  Serial.print(irValue);
  Serial.print("\t");

  Serial.print(beatAvg);
  Serial.print("\t");

  Serial.print(temperature);
  Serial.print("\t");

  // 输出状态
  if (outputActive)
  {
    Serial.println(1);
  }
  else
  {
    Serial.println(0);
  }

  // 绘图刷新速度
  delay(30);
}