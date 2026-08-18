#include <EEPROM.h>

// Arduino UNO：5个压力传感器
// A0、A1、A2、A3、A4分别连接传感器1～5。
const byte SENSOR_COUNT = 5;
const byte sensorPins[SENSOR_COUNT] = {A0, A1, A2, A3, A4};

// 按压时ADC增大设为true，按压时ADC减小设为false。
const bool PRESSURE_ADDS_ADC = true;
const int DEAD_BAND = 4;
const byte AVERAGE_SAMPLES = 10;
const unsigned long TARE_DURATION_MS = 500;
const unsigned long PRINT_INTERVAL_MS = 100;

// 校准时使用的标准砝码重量，可按实际砝码修改。
const float CALIBRATION_WEIGHT_G = 100.0;
const unsigned int EEPROM_MAGIC = 0x5A37;

struct CalibrationData {
  unsigned int magic;
  float gramPerAdc[SENSOR_COUNT];
};

CalibrationData calibration;
float baseValue[SENSOR_COUNT] = {0, 0, 0, 0, 0};
unsigned long lastPrintTime = 0;

int readAverage(byte pin) {
  long sum = 0;
  for (byte i = 0; i < AVERAGE_SAMPLES; i++) {
    sum += analogRead(pin);
  }
  return (int)(sum / AVERAGE_SAMPLES);
}

int pressureDelta(int raw, float base) {
  int diff = PRESSURE_ADDS_ADC
               ? raw - (int)base
               : (int)base - raw;
  return diff >= DEAD_BAND ? diff : 0;
}

void tareSensors() {
  long sum[SENSOR_COUNT] = {0, 0, 0, 0, 0};
  unsigned long sampleCount = 0;
  unsigned long startTime = millis();

  while (millis() - startTime < TARE_DURATION_MS) {
    for (byte i = 0; i < SENSOR_COUNT; i++) {
      sum[i] += analogRead(sensorPins[i]);
    }
    sampleCount++;
    delay(5);
  }

  for (byte i = 0; i < SENSOR_COUNT; i++) {
    baseValue[i] = (float)sum[i] / sampleCount;
  }
}

void loadCalibration() {
  EEPROM.get(0, calibration);

  if (calibration.magic != EEPROM_MAGIC) {
    calibration.magic = EEPROM_MAGIC;
    for (byte i = 0; i < SENSOR_COUNT; i++) {
      calibration.gramPerAdc[i] = 0.0;
    }
  }

  for (byte i = 0; i < SENSOR_COUNT; i++) {
    if (calibration.gramPerAdc[i] < 0.000001 ||
        calibration.gramPerAdc[i] > 10000.0) {
      calibration.gramPerAdc[i] = 0.0;
    }
  }
}

void saveCalibration() {
  calibration.magic = EEPROM_MAGIC;
  EEPROM.put(0, calibration);
}

void calibrateSensor(byte index) {
  int raw = readAverage(sensorPins[index]);
  int delta = pressureDelta(raw, baseValue[index]);

  if (delta == 0) {
    Serial.print("Sensor ");
    Serial.print(index + 1);
    Serial.println(": calibration failed, no pressure detected.");
    return;
  }

  calibration.gramPerAdc[index] = CALIBRATION_WEIGHT_G / delta;
  saveCalibration();

  Serial.print("Sensor ");
  Serial.print(index + 1);
  Serial.print(" calibrated: ");
  Serial.print(CALIBRATION_WEIGHT_G, 1);
  Serial.println(" g");
}

void setup() {
  Serial.begin(115200);
  loadCalibration();

  // 上电后立即用500ms采集空载水平值，作为0g。
  tareSensors();

  Serial.println("Tare completed: 0 g");
  Serial.print("Calibration weight: ");
  Serial.print(CALIBRATION_WEIGHT_G, 1);
  Serial.println(" g");
  Serial.println("Put the calibration weight on one sensor, then send 1-5.");
  Serial.println("Send T to tare again, C to clear calibration.");
}

void loop() {
  if (Serial.available() > 0) {
    char command = Serial.read();

    if (command >= '1' && command <= '5') {
      calibrateSensor(command - '1');
    } else if (command == 't' || command == 'T') {
      Serial.println("Remove all loads. Taring for 500 ms...");
      tareSensors();
      Serial.println("Tare completed: 0 g");
    } else if (command == 'c' || command == 'C') {
      for (byte i = 0; i < SENSOR_COUNT; i++) {
        calibration.gramPerAdc[i] = 0.0;
      }
      saveCalibration();
      Serial.println("All calibration data cleared.");
    }
  }

  if (millis() - lastPrintTime < PRINT_INTERVAL_MS) {
    return;
  }
  lastPrintTime = millis();

  for (byte i = 0; i < SENSOR_COUNT; i++) {
    int raw = readAverage(sensorPins[i]);
    int delta = pressureDelta(raw, baseValue[i]);
    float weightG = delta * calibration.gramPerAdc[i];

    Serial.print("S");
    Serial.print(i + 1);
    Serial.print(":");
    Serial.print(weightG, 1);
    Serial.print(" g");

    if (calibration.gramPerAdc[i] == 0.0) {
      Serial.print("(not calibrated)");
    }

    if (i < SENSOR_COUNT - 1) {
      Serial.print("\t");
    }
  }
  Serial.println();
}
