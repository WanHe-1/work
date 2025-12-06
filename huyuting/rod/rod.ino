#include <Servo.h>

Servo myservo;

const int servoPin = 9;
const int sensorPin1 = A0;
const int sensorPin2 = A1;

// 舵机控制参数
const int centerPos = 90;         // 中心位置（设定点）
int currentPos = 90;              // 当前角度
const int maxRotation = 25;       // 最大旋转角度（±25度）
const int minPos = centerPos - maxRotation;  // 最小角度 (65度)
const int maxPos = centerPos + maxRotation;  // 最大角度 (115度)

// 压力检测参数
const int baseline1 = 893;          // 左侧传感器基准值（无压力时）
const int baseline2 = 673;          // 右侧传感器基准值（无压力时）
const int pressureThreshold = 0;   // 压力阈值，根据实际传感器调整（降低阈值提高灵敏度）
bool leftPressurePrev = false;      // 左侧传感器上一次状态
bool rightPressurePrev = false;     // 右侧传感器上一次状态

// 时间控制
unsigned long lastMoveTime = 0;
const unsigned long moveInterval = 50;  // 每 50ms 移动一次

void setup() {
  Serial.begin(9600);
  myservo.attach(servoPin);
  myservo.write(currentPos);
  delay(500);  // 等待舵机初始化
}

void loop() {
  // 读取压力传感器原始值
  int rawVal1 = analogRead(sensorPin1);  // 左侧传感器原始值
  int rawVal2 = analogRead(sensorPin2);  // 右侧传感器原始值

  // 反转并校准传感器值（压力越大，数值越大）
  int val1 = baseline1 - rawVal1;  // 左侧传感器校准值
  int val2 = baseline2 - rawVal2;  // 右侧传感器校准值
  
  // 确保值不为负
  if (val1 < 0) val1 = 0;
  if (val2 < 0) val2 = 0;

  // 检测当前压力状态
  bool leftPressure = (val1 > pressureThreshold);
  bool rightPressure = (val2 > pressureThreshold);

  // 左侧传感器检测到压力：逆时针旋转2度（角度增加）
  if (leftPressure && !leftPressurePrev) {
    int newPos = currentPos + 2;
    if (newPos <= maxPos) {
      currentPos = newPos;
      myservo.write(currentPos);
      Serial.println("Left pressure! Rotating 2° counter-clockwise.");
    } else {
      Serial.println("Left pressure detected but max limit reached.");
    }
  }

  // 右侧传感器检测到压力：顺时针旋转4度（角度减小）
  if (rightPressure && !rightPressurePrev) {
    int newPos = currentPos - 4;
    if (newPos >= minPos) {
      currentPos = newPos;
      myservo.write(currentPos);
      Serial.println("Right pressure! Rotating 4° clockwise.");
    } else {
      Serial.println("Right pressure detected but min limit reached.");
    }
  }

  // 更新上一次状态
  leftPressurePrev = leftPressure;
  rightPressurePrev = rightPressure;

  // 打印传感器值和当前位置
  Serial.print("Left: ");
  Serial.print(val1);
  Serial.print(" | Right: ");
  Serial.print(val2);
  Serial.print(" | Pos: ");
  Serial.print(currentPos);
  Serial.print(" (");
  Serial.print(currentPos - centerPos);
  Serial.println("°)");

  delay(100);  // 延时以避免过快读取
}