
// 定义模拟输入引脚
const int pressureSensorPin = A0;

void setup() {
  // 初始化串行通信
  Serial.begin(9600);
}

void loop() {

  int sensorValue = analogRead(pressureSensorPin);
  

  Serial.println(sensorValue);
  
  // 等待一段时间再进行下一次读取
  delay(100);
}