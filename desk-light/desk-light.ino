#include <Servo.h>

// 引脚定义
const int trigPin = 12;    // 超声波传感器Trig引脚
const int echoPin = 13;    // 超声波传感器Echo引脚
const int ledPin = 9;      // LED连接到9号引脚

long duration;
int distance;

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ledPin, OUTPUT);

  Serial.begin(9600);
}

void loop() {
  // 发送超声波脉冲
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // 接收回波信号
  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;  // 计算距离

  // 输出距离，便于调试
  Serial.print("Distance: ");
  Serial.println(distance);

  // 当距离小于20cm时，开始执行呼吸灯效果
  if (distance < 20) {
    breathingLED();
  } else {
    // 距离大于20cm时，LED熄灭
    analogWrite(ledPin, 0);
  }
  delay(100);
}

// 呼吸灯效果
void breathingLED() {
  for (int i = 0; i <= 255; i++) {
    analogWrite(ledPin, i); // 从最暗到最亮
    delay(10);  // 延迟时间控制呼吸效果的速度
  }

  for (int i = 255; i >= 0; i--) {
    analogWrite(ledPin, i); // 从最亮到熄灭
    delay(10);  // 延迟时间控制呼吸效果的速度
  }
}
