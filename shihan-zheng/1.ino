#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <MAX30105.h>
#include "heartRate.h"

// 硬件连接及可调参数
// MAX30102/MAX30105 使用 I2C：Arduino UNO/Nano 的 SDA=A4、SCL=A5。
// DS18B20 使用单总线，数据线可以接 Arduino 的 A0（A0 也可作为数字引脚使用）。
const byte TEMP_SENSOR_PIN = A0;
const byte VIBRATION_PIN_1 = 5;
const byte VIBRATION_PIN_2 = 6;
const byte RELAY_PIN = 7;
const bool VIBRATION_ACTIVE_LOW = true;

const long MIN_IR_VALUE = 50000;       // 低于此值时认为没有稳定接触
const long IR_LEVEL_25S = 80000;
const long IR_LEVEL_45S = 100000;
const long IR_LEVEL_60S = 120000;
const unsigned long HEAT_TIME_25S = 25000;
const unsigned long HEAT_TIME_45S = 45000;
const unsigned long HEAT_TIME_60S = 60000;
const float TARGET_TEMPERATURE_C = 38.0;
const float FULL_POWER_BELOW_C = 37.5;
const float SAFETY_TEMPERATURE_C = 39.0;


const unsigned long REPORT_INTERVAL_MS = 1000;
const unsigned long HEART_RATE_TIMEOUT_MS = 5000;
const byte RATE_SIZE = 4;

OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature temperatureSensors(&oneWire);
MAX30105 heartSensor;

float temperatureC = DEVICE_DISCONNECTED_C;
float instantBpm = 0.0;
int heartRateBpm = 0;
int heatingPowerPercent = 0;
bool vibrationDetected = false;
bool relayOn = false;
byte rateValues[RATE_SIZE] = {0};
byte rateIndex = 0;
unsigned long lastBeatMs = 0;
unsigned long temperatureRequestMs = 0;
unsigned long heatingStartMs = 0;
unsigned long heatingDurationMs = 0;
unsigned long lastReportMs = 0;

void setRelay(bool on) {
  // 按当前要求：D7 为 HIGH 时继电器启动，LOW 时关闭。
  digitalWrite(RELAY_PIN, on ? HIGH : LOW);
}

void updateHeartRate() {
  const unsigned long now = millis();
  const long infraredValue = heartSensor.getIR();

  // 参考 max5060.ino：红外值过低时表示没有稳定放置手指。
  if (infraredValue < 50000) {
    instantBpm = 0.0;
    heartRateBpm = 0;
    rateIndex = 0;
    for (byte index = 0; index < RATE_SIZE; index++) {
      rateValues[index] = 0;
    }
    lastBeatMs = 0;
    return;
  }

  if (checkForBeat(infraredValue)) {
    if (lastBeatMs != 0) {
      const unsigned long beatInterval = now - lastBeatMs;
      instantBpm = 60000.0 / beatInterval;

      if (instantBpm < 255.0 && instantBpm > 20.0) {
        rateValues[rateIndex++] = (byte)instantBpm;
        rateIndex %= RATE_SIZE;

        unsigned int rateTotal = 0;
        for (byte index = 0; index < RATE_SIZE; index++) {
          rateTotal += rateValues[index];
        }
        heartRateBpm = rateTotal / RATE_SIZE;
      }
    }
    lastBeatMs = now;
  }

  if (lastBeatMs != 0 && now - lastBeatMs > HEART_RATE_TIMEOUT_MS) {
    instantBpm = 0.0;
    heartRateBpm = 0;
    lastBeatMs = 0;
  }
}

void updateTemperature() {
  const unsigned long now = millis();

  if (now - temperatureRequestMs < 750) {
    return;
  }

  temperatureSensors.requestTemperatures();
  const float reading = temperatureSensors.getTempCByIndex(0);
  if (reading != DEVICE_DISCONNECTED_C && reading >= -55.0 && reading <= 125.0) {
    temperatureC = reading;
  } else {
    temperatureC = DEVICE_DISCONNECTED_C;
  }
  temperatureRequestMs = now;
}

void updateVibration() {
  const int vibrationLevel1 = digitalRead(VIBRATION_PIN_1);
  const int vibrationLevel2 = digitalRead(VIBRATION_PIN_2);
  const int activeLevel = VIBRATION_ACTIVE_LOW ? LOW : HIGH;

  // 两个震动传感器任意一个检测到震动，就认为发生了震动。
  vibrationDetected = vibrationLevel1 == activeLevel || vibrationLevel2 == activeLevel;
}

unsigned long getHeatingDuration(long infraredValue) {
  if (infraredValue >= IR_LEVEL_60S) {
    return HEAT_TIME_60S;
  }
  if (infraredValue >= IR_LEVEL_45S) {
    return HEAT_TIME_45S;
  }
  if (infraredValue >= IR_LEVEL_25S) {
    return HEAT_TIME_25S;
  }
  return 0;
}

void updateRelay() {
  const unsigned long now = millis();
  const long infraredValue = heartSensor.getIR();
  const bool temperatureValid = temperatureC != DEVICE_DISCONNECTED_C;
  const bool temperatureSafe = temperatureValid &&
      temperatureC < TARGET_TEMPERATURE_C &&
      temperatureC < SAFETY_TEMPERATURE_C;

  if (!relayOn && infraredValue >= MIN_IR_VALUE && temperatureSafe) {
    const unsigned long requestedDuration = getHeatingDuration(infraredValue);
    if (requestedDuration > 0) {
      heatingDurationMs = requestedDuration;
      heatingStartMs = now;
      relayOn = true;
    }
  }

  if (relayOn && (!temperatureSafe || now - heatingStartMs >= heatingDurationMs ||
                   infraredValue < MIN_IR_VALUE)) {
    relayOn = false;
    heatingDurationMs = 0;
  }

  heatingPowerPercent = relayOn ? 100 : 0;
  setRelay(relayOn);
}

void reportStatus() {
  const unsigned long now = millis();
  if (now - lastReportMs < REPORT_INTERVAL_MS) {
    return;
  }
  lastReportMs = now;

  // 串口绘图仪要求每行使用“名称:数值”格式，并且只输出数值数据。
  Serial.print("IR:");
  Serial.print(heartSensor.getIR());
  Serial.print("\tBPM:");
  Serial.print(heartRateBpm);
  Serial.print("\tTemperature:");
  if (temperatureC == DEVICE_DISCONNECTED_C) {
    Serial.print(0.0, 2);
  } else {
    Serial.print(temperatureC, 2);
  }
  Serial.print("\tHeating:");
  Serial.print(heatingPowerPercent);
  Serial.print("\tVibration:");
  Serial.println(vibrationDetected ? 1 : 0);
}

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  setRelay(false);
  relayOn = false;
  pinMode(VIBRATION_PIN_1, INPUT);
  pinMode(VIBRATION_PIN_2, INPUT);

  temperatureSensors.begin();
  temperatureSensors.setResolution(12);
  temperatureSensors.setWaitForConversion(true);

  Wire.begin();
  if (!heartSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 not found. Heating disabled.");
    while (true) {
      setRelay(false);
      delay(1000);
    }
  }

  heartSensor.setup();
  heartSensor.setPulseAmplitudeRed(0x0A);
  heartSensor.setPulseAmplitudeGreen(0);
}

void loop() {
  updateHeartRate();
  updateTemperature();
  updateVibration();
  updateRelay();
  reportStatus();
}
