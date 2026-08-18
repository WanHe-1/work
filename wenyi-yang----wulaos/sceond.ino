// =====================================================
// Arduino UNO + 5个薄膜压力传感器
// 传感器：A0 ~ A4
// 输出：5个重量值，单位 g
// =====================================================

const byte SENSOR_COUNT = 5;
const byte sensorPins[SENSOR_COUNT] = {A0, A1, A2, A3, A4};

// 上电后500ms内采集空载基准，所有通道交替采样
const unsigned long TARE_TIME_MS = 500;

// 按压时ADC增大设为true；按压时ADC减小设为false
const bool PRESSURE_ADDS_ADC = true;

// 每个ADC变化量对应的克数，需要按传感器实际标定
const float scaleFactor[SENSOR_COUNT] = {
  1.25, 1.25, 1.25, 1.25, 1.25
};

// 小于这个变化量认为是0g
const int ZERO_THRESHOLD = 8;

float baseValue[SENSOR_COUNT] = {0, 0, 0, 0, 0};
unsigned long lastPrintTime = 0;

void tareSensors() {
  long sum[SENSOR_COUNT] = {0, 0, 0, 0, 0};
  unsigned long sampleCount = 0;
  unsigned long startTime = millis();

  while (millis() - startTime < TARE_TIME_MS) {
    for (byte i = 0; i < SENSOR_COUNT; i++) {
      sum[i] += analogRead(sensorPins[i]);
    }
    sampleCount++;
    delay(5);
  }

  if (sampleCount == 0) {
    sampleCount = 1;
  }

  for (byte i = 0; i < SENSOR_COUNT; i++) {
    baseValue[i] = (float)sum[i] / sampleCount;
  }
}

int readAverage(byte pin) {
  long sum = 0;
  for (byte i = 0; i < 8; i++) {
    sum += analogRead(pin);
  }
  return (int)(sum / 8);
}

int getDelta(int raw, float base) {
  int delta = PRESSURE_ADDS_ADC
                ? raw - (int)base
                : (int)base - raw;

  if (delta < ZERO_THRESHOLD) {
    return 0;
  }

  return delta;
}

void setup() {
  Serial.begin(9600);
  delay(300);

  Serial.println("Remove all pressure. Calibrating 500 ms...");
  tareSensors();
  Serial.println("Calibration complete. All sensors are now 0 g.");

  // 输出基准值，便于检查A3/A4是否存在接线或漂移问题
  Serial.print("Base ADC: ");
  for (byte i = 0; i < SENSOR_COUNT; i++) {
    Serial.print(baseValue[i], 1);
    if (i < SENSOR_COUNT - 1) {
      Serial.print('\t');
    }
  }
  Serial.println();
}

void loop() {
  if (Serial.available() > 0) {
    char command = Serial.read();
    if (command == 't' || command == 'T') {
      Serial.println("Remove all pressure. Recalibrating 500 ms...");
      tareSensors();
      Serial.println("Recalibration complete.");
    }
  }

  if (millis() - lastPrintTime < 200) {
    return;
  }
  lastPrintTime = millis();

  for (byte sensor = 0; sensor < SENSOR_COUNT; sensor++) {
    int raw = readAverage(sensorPins[sensor]);
    int delta = getDelta(raw, baseValue[sensor]);
    float weight = delta * scaleFactor[sensor];

    Serial.print(weight, 1);
    Serial.print(" g");

    if (sensor < SENSOR_COUNT - 1) {
      Serial.print("    ");
    }
  }
  Serial.println();
}
