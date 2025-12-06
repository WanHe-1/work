// 定义引脚
const int pressureSensorPin = A0;  
const int relayPin = 2;       

// 设置压力阈值（根据实际传感器校准）
const int pressureThreshold = 250; // 
int sensorValue = 0;
int pressurePercent = 0;

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

  sensorValue = analogRead(pressureSensorPin);

  // 将传感器值从 0~1023 映射为 0~100，并限制在合理范围内
  pressurePercent = map(sensorValue, 0, 1023, 0, 100);      // 线性映射
  pressurePercent = constrain(pressurePercent, 0, 100);     // 防止越界

  // 打印原始和映射后的数值到串口监视器

  Serial.print(" Humidity : ");
  Serial.print(pressurePercent);
  Serial.println("%");
  //
  if (sensorValue < pressureThreshold) {
    digitalWrite(relayPin, HIGH);
    Serial.println("气泵已启动");
  } else {
    digitalWrite(relayPin, LOW);
    Serial.println("气泵已关闭");
  }

  // 延迟一段时间再进行下一次检测
  delay(1000); // 每秒检测一次
}