const int vibrationPin = A0; // 震动传感器 AO
const int soundPin = A1;     // 声音传感器 AO

void setup() {
  Serial.begin(9600);
}

void loop() {
  int vibration = analogRead(vibrationPin); // 0~1023
  int sound = analogRead(soundPin);         // 0~1023

  int sum = vibration + sound;              // 相加，范围 0~2046

  Serial.print(vibration);
  Serial.print(",");
  Serial.print(sound);
  Serial.print(",");
  Serial.println(sum);                      // 最后一行必须 println

  delay(30); // 控制刷新速度，避免曲线太密
}