#include <Wire.h>

// Arduino Uno 引脚配置
const int LED = 13;           // 板载 LED
const int PRESSURE_PIN = A0;  // 压力传感器 AO -> A0
const int EMG_PIN = A1;       // 肌电传感器 AO -> A1

const int PRESSURE_THRESHOLD = 1000;
const int EMG_CHANGE_THRESHOLD = 60;
const unsigned long TENS_RUN_TIME_MS = 5000;

bool pressureArmed = true;
int gsrThreshold = 0;

// TENS I2C 数据
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
  Serial.println("TENS_ON");
  sendTensCommand(1, 1);
  delay(TENS_RUN_TIME_MS);
  tensOff();
}

void setup() {
  Serial.begin(9600);
  Wire.begin();
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  delay(1000);

  // 校准肌电基线（前 2.5 秒平均值）
  long sum = 0;
  for (int i = 0; i < 500; i++) {
    sum += analogRead(EMG_PIN);
    delay(5);
  }
  gsrThreshold = sum / 500;
  Serial.print("EMG threshold = ");
  Serial.println(gsrThreshold);

  tensOff();
  Serial.println("System initialized. Waiting for pressure trigger...");
}

void loop() {
  int pressureValue = analogRead(PRESSURE_PIN);
  int emgValue = analogRead(EMG_PIN);

  Serial.print("Pressure=");
  Serial.print(pressureValue);
  Serial.print("\tEMG=");
  Serial.println(emgValue);

  // 肌电变化检测
  int temp = gsrThreshold - emgValue;
  if (abs(temp) > EMG_CHANGE_THRESHOLD) {
    int confirmValue = analogRead(EMG_PIN);
    temp = gsrThreshold - confirmValue;
    if (abs(temp) > EMG_CHANGE_THRESHOLD) {
      digitalWrite(LED, HIGH);
      Serial.println("EMG change detected!");
      delay(3000);
      digitalWrite(LED, LOW);
      delay(1000);
    }
  }

  // 压力触发 TENS
  if (pressureValue >= PRESSURE_THRESHOLD && pressureArmed) {
    pressureArmed = false;
    Serial.println("Pressure threshold reached, starting TENS for 5 seconds");
    runTensFor5Seconds();
  }

  if (pressureValue < PRESSURE_THRESHOLD - 20) {
    pressureArmed = true;
  }

  delay(20);
}