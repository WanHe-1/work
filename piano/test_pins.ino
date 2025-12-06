// 测试代码：检测模拟引脚数值
// 用于调试水果钢琴的触摸检测和调整阈值

const int fruitPins[] = {A0, A1, A2, A3, A4, A5, A6};
const int pinCount = 7;

void setup() {
  Serial.begin(9600);
  
  Serial.println("=== 水果钢琴模拟引脚测试程序 ===");
  Serial.println("触摸苹果时，数值会发生变化");
  Serial.println("根据数值调整 piano.ino 中的 threshold");
  Serial.println("引脚: A0   A1   A2   A3   A4   A5   A6");
  Serial.println("----------------------------------------------");
}

void loop() {
  // 读取所有模拟引脚的值
  Serial.print("值: ");
  for (int i = 0; i < pinCount; i++) {
    int value = analogRead(fruitPins[i]);
    Serial.print(value);
    Serial.print("\t");
  }
  Serial.println();
  
  delay(200); // 每0.2秒更新一次
}
