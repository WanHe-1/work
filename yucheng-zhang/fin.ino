/**************************************************************************
 * 单开发板双传感器采集程序
 *
 * 肌肉传感器 SIG  -> A0
 * AD8232 OUTPUT   -> A1
 * AD8232 LO+      -> D10
 * AD8232 LO-      -> D11
 *
 * 串口输出格式（115200 baud）：
 * 肌肉传感器值,AD8232值,电极脱落状态
 * 例如：512,498,0
 * 电极脱落状态为 0 表示正常，1 表示检测到电极脱落。
 ***************************************************************************/

const byte muscleSensorPin = A0;
const byte ad8232SensorPin = A1;
const byte leadOffPositivePin = 10;
const byte leadOffNegativePin = 11;

void setup() {
  Serial.begin(115200);

  pinMode(muscleSensorPin, INPUT);
  pinMode(ad8232SensorPin, INPUT);
  pinMode(leadOffPositivePin, INPUT);
  pinMode(leadOffNegativePin, INPUT);
}

void loop() {
  const int muscleValue = analogRead(muscleSensorPin);
  const int ad8232Value = analogRead(ad8232SensorPin);
  const bool leadOff = digitalRead(leadOffPositivePin) == HIGH ||
                       digitalRead(leadOffNegativePin) == HIGH;

  Serial.print("Muscle:");
  Serial.print(muscleValue);
  Serial.print('\t');
  Serial.print("AD8232:");
  Serial.println(ad8232Value);

  delay(2);
}
