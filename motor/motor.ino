// 定义引脚
const int trigPin = 10;     // 超声波 Trig
const int echoPin = 11;     // 超声波 Echo（不能和 ENA 共用）
const int motorENA = 9;   // L298N 的 ENA，接 PWM 引脚 10 ✅
const int motorIN1 = 5;    // 控制方向
const int motorIN2 = 6;    // 控制方向

void setup() {
  // 设置引脚模式
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(motorENA, OUTPUT);
  pinMode(motorIN1, OUTPUT);
  pinMode(motorIN2, OUTPUT);

  Serial.begin(9600);
}

void loop() {
  // --- 超声波测距 ---
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);           // 读取高电平持续时间
  float distance = duration * 0.034 / 2;           

  Serial.print(distance);
  Serial.println(" cm");

  // --- 根据距离设置电机速度 ---
  int motorSpeed;
  if (distance < 30) {
    motorSpeed = 255;      
  } else if (distance > 100) {
    motorSpeed = 0;      
  } else {
    // 距离在 5~50cm 之间，线性加速
    motorSpeed = map(distance, 100, 30, 0, 255);
    motorSpeed = constrain(motorSpeed, 0, 255);  // 限制在 0~255
  }

  digitalWrite(motorIN1, HIGH);
  digitalWrite(motorIN2, LOW);

  analogWrite(motorENA, motorSpeed); 
  // 打印电机速度
  Serial.println(motorSpeed);
  delay(50);  // 稍微延迟，避免串口输出太快
}