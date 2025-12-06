// 定义引脚
const int pressureSensorPin = A0;  // 压力传感器连接到A0
const int relayPin = 2;            // 继电器连接到D2

// 设置压力阈值（根据实际传感器校准）
const int pressureThreshold = 250; // 举例：当传感器读数大于600时启动气泵
int sensorValue = 0;
void setup() {
  // 初始化串口通信（用于调试）
  Serial.begin(9600);

  // 设置引脚模式
  pinMode(pressureSensorPin, INPUT);
  pinMode(relayPin, OUTPUT);

  // 初始状态关闭气泵
  digitalWrite(relayPin, LOW);
}
void loop() {
  // 读取压力传感器的模拟值（范围：0~1023）
  sensorValue = analogRead(pressureSensorPin);
  // 打印传感器数值到串口监视器
  Serial.print("Pressure Sensor Value: ");
  Serial.println(sensorValue);

  // 根据压力值控制气泵
  if (sensorValue < pressureThreshold) {
    // 如果压力低于阈值，开启气泵
    digitalWrite(relayPin, HIGH);
    Serial.println("气泵已启动");
  } else {
    // 如果压力足够，关闭气泵
    digitalWrite(relayPin, LOW);
    Serial.println("气泵已关闭");
  }
  // 延迟一段时间再进行下一次检测（防止过于频繁切换）
  delay(1000); // 每秒检测一次
}