// 最简单的方案，删除所有头文件包含
const int smokePin = A0;    // MQ-2烟雾传感器连接到模拟引脚A0
const int sampleDelay = 500; // 增加采样间隔到500毫秒

void setup() {
  Serial.begin(9600);  // 初始化串口通信
  Serial.println("烟雾传感器初始化完成...");  // 添加调试信息
}

void loop() {
  // 读取烟雾传感器的值
  int smokeValue = analogRead(smokePin);
  
  // 将模拟值(0-1023)映射到亮度值(0-100)
  int brightness = map(smokeValue, 0, 1023, 0, 100);
  
  // 发送格式化数据
  Serial.print("亮度值:");
  Serial.println(brightness);
  
  delay(sampleDelay);  // 延长采样间隔，减少数据量
} 