#include <Wire.h>

// Arduino Uno 引脚配置
const int LED = 13;
const int PRESSURE_PIN = A0; // 压力传感器 AO -> A0
const int EMG_PIN = A1;      // 肌电传感器 AO -> A1

const int PRESSURE_CHANGE_THRESHOLD = 30;
const int EMG_CHANGE_THRESHOLD = 60;
const unsigned long TENS_RUN_TIME_MS = 5000;
const unsigned long CHANGE_WINDOW_MS = 300;

int pressureBaseline = 0;
int emgBaseline = 0;

unsigned long pressureChangeTime = 0;
unsigned long emgChangeTime = 0;
bool pressureChanged = false;
bool emgChanged = false;
bool tensBusy = false;

unsigned char tx1_data[15] = {0xFA, 0x01, 0x61, 0x94, 0x00, 0x14, 0x61, 0x94, 0x00, 0x14, 0x01, 0x01, 0x1E, 0x1E, 0xAF};
unsigned char Val1_1 = 20;
unsigned char Val1_2 = 40;

void sendTensCommand(unsigned char ch1, unsigned char ch2) {
  tx1_data[12] = Val1_1;
  tx1_data[13] = Val1_2;
  tx1_data[10] = ch1;
  tx1_data[11] = ch2;
  Wire.beginTransmission(0x01);
  Wire.write(tx1_data, 15);
  Wire.endTransmission();
}

void tensOff() {
  sendTensCommand(0, 0);
  Serial.println("TENS_OFF");
}

void runTensFor5Seconds() {
  tensBusy = true;
  Serial.println("TENS_ON");
  sendTensCommand(1, 1);
  delay(TENS_RUN_TIME_MS);
  tensOff();
  tensBusy = false;
}

void setup() {
  Serial.begin(9600);
  Wire.begin();
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  delay(1000);

  long pressureSum = 0;
  long emgSum = 0;
  for (int i = 0; i < 200; i++) {
    pressureSum += analogRead(PRESSURE_PIN);
    emgSum += analogRead(EMG_PIN);
    delay(5);
  }
  pressureBaseline = pressureSum / 200;
  emgBaseline = emgSum / 200;

  Serial.print("Pressure baseline = ");
  Serial.println(pressureBaseline);
  Serial.print("EMG baseline = ");
  Serial.println(emgBaseline);

  tensOff();
  Serial.println("System initialized. Waiting for simultaneous pressure + EMG change...");
}

void loop() {
  int pressureValue = analogRead(PRESSURE_PIN);
  int emgValue = analogRead(EMG_PIN);

  int pressureDelta = abs(pressureValue - pressureBaseline);
  int emgDelta = abs(emgValue - emgBaseline);

  if (pressureDelta > PRESSURE_CHANGE_THRESHOLD) {
    pressureChanged = true;
    pressureChangeTime = millis();
  }

  if (emgDelta > EMG_CHANGE_THRESHOLD) {
    emgChanged = true;
    emgChangeTime = millis();
  }

  if (pressureChanged && (millis() - pressureChangeTime > CHANGE_WINDOW_MS)) {
    pressureChanged = false;
  }
  if (emgChanged && (millis() - emgChangeTime > CHANGE_WINDOW_MS)) {
    emgChanged = false;
  }

  Serial.print("Pressure=");
  Serial.print(pressureValue);
  Serial.print("\tPressureDelta=");
  Serial.print(pressureDelta);
  Serial.print("\tEMG=");
  Serial.print(emgValue);
  Serial.print("\tEMGDelta=");
  Serial.println(emgDelta);

  if (!tensBusy && pressureChanged && emgChanged) {
    Serial.println("Pressure + EMG changed together, starting TENS for 5 seconds");
    pressureChanged = false;
    emgChanged = false;
    runTensFor5Seconds();
  }

  delay(50);
}