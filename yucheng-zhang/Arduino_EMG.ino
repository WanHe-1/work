/**************************************************************************
 * 单传感器肌电采集程序
 * 传感器 SIG 接 Arduino A0
 ***************************************************************************/

const byte analogInput = A0;

void setup() {
  pinMode(analogInput, INPUT);
  Serial.begin(9600);
}

void loop() {
  Serial.println(analogRead(analogInput));
  delay(10);
}
