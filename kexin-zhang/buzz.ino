const int LED_PIN = 9;  // LED连接到D9引脚（支持PWM）

void setup() {
Serial.begin(9600);
pinMode(LED_PIN, OUTPUT);

analogWrite(LED_PIN, 0);

Serial.println("请输入0-255之间的亮度值：");
}

void loop() {
if (Serial.available() > 0) {
int brightness = Serial.parseInt();

```
// 检查输入范围
if (brightness >= 0 && brightness <= 255) {
  analogWrite(LED_PIN, brightness);

  Serial.print("当前亮度：");
  Serial.println(brightness);
} else {
  Serial.println("输入错误，请输入0-255！");
}

// 清除剩余串口字符
while (Serial.available() > 0) {
  Serial.read();
}
```

}
}
