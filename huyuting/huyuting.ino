// 定义超声波传感器引脚
const int trigPin = 10;
const int echoPin = 11;

// 定义变量
long duration;
int distance;

void setup() {
  // 初始化串口通信
  Serial.begin(9600);
  // 定义引脚为输出和输入
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
}

void loop() {
  // 清除trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);


  // 读取echoPin，返回超声波传播时间（微秒）
  duration = pulseIn(echoPin, HIGH);
  // 计算距离（厘米）
  distance = duration * 0.034 / 2;
  Serial.println(distance);

  // 如果距离小于30cm，发送触发信号
  if (distance < 20) {
    Serial.println("T"); // 发送字符 'T' 作为触发信号
    delay(500); // 简单的去抖动/冷却时间，避免连续触发MS
  }

  // 短暂延迟
  delay(100);
}