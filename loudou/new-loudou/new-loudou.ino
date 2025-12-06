#include <Servo.h>

Servo myServo1;
Servo myServo2;

int servoPin1 = 9;
int servoPin2 = 10;
int pressurePin = A0;
int pressureValue = 0;

// 状态标志
bool inRangeLastTime = false;     // 上一次是否在压力范围内
bool servo1Scanned = false;      // 舵机1是否已经扫描到180°
bool servo2Scanning = false;      // 是否正在执行舵机2的扫描（可选，这里用于防重入）

void setup() {
  myServo1.attach(servoPin1);
  myServo2.attach(servoPin2);
  Serial.begin(9600);

  // 初始化位置
  myServo1.write(0);
  myServo2.write(0);

  // 读取初始状态
  pressureValue = analogRead(pressurePin);
  inRangeLastTime = (pressureValue >= 200 && pressureValue <= 300);
}

void loop() {
  pressureValue = analogRead(pressurePin);
  Serial.println(pressureValue);

  bool inRange = (pressureValue >= 200 && pressureValue <= 300);
  bool inHighRange = (pressureValue >= 500 && pressureValue <= 700);

  if (inRange) {
    // --- 在压力范围内 (200-300) ---
    if (!inRangeLastTime) {
      Serial.println("Servo1 back to 0°");
      myServo1.write(0);
      servo1Scanned = false;  // 重置标志，允许下次扫描
    }

    // 舵机2执行扫描动作
    if (!inRangeLastTime) {
      Serial.println("Servo2 0->180->0");
      
      for (int angle = 0; angle <= 180; angle++) {
        myServo2.write(angle);
        delay(30);
      }
      delay(3000);
      for (int angle = 180; angle >= 0; angle--) {
        myServo2.write(angle);
        delay(30);
      }
      
      myServo2.write(0);
    }

    inRangeLastTime = true;
  }
  else if (inHighRange) {
    // --- 在高压范围内 (500-700) ---
    myServo2.write(0);  
    if (!servo1Scanned) {
      // 获取当前舵机1的角度
      int currentAngle = myServo1.read();
      
      // 如果还没到180°，每次增加30°
      if (currentAngle < 180) {
        int newAngle = currentAngle + 20;
        if (newAngle > 180) newAngle = 180; // 确保不超过180°
  
        myServo1.write(newAngle);
        delay(500);  // 增加延迟时间，让旋转速度更慢
        
        // 如果到达180°，设置标志
        if (newAngle >= 180) {
          servo1Scanned = true;
        }
      }
    }

    inRangeLastTime = false;
  }
  else {
    // --- 不在任何指定范围内 ---
    myServo2.write(0);  // 舵机2保持在0度
    inRangeLastTime = false;
  }

  delay(10);
}