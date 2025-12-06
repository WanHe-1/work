const int soundPin = A0; // 声音传感器连接到A0引脚
const int ledPin = 9;    // LED连接到D9引脚 (支持PWM)

void setup() {
  pinMode(soundPin, INPUT);
  pinMode(ledPin, OUTPUT);
}

void loop() {
  int soundLevel = analogRead(soundPin); // 读取声音传感器的值
  int brightness = map(soundLevel, 0, 1023, 0, 255); // 将声音值映射到0-255的范围
  analogWrite(ledPin, brightness); // 根据声音强度设置LED亮度
}



