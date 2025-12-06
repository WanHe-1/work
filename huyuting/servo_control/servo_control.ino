#include <Servo.h>

Servo myservo;

const int servoPin = 9;
int currentPos = 90;  // 初始位置90度

void setup() {
  Serial.begin(9600);
  myservo.attach(servoPin);
  myservo.write(currentPos);
  
  Serial.println("=== 舵机控制系统 ===");
  Serial.println("当前位置: 90度");
  Serial.println("命令:");
  Serial.println("  输入角度 (0-180) - 移动到指定角度");
  Serial.println("  + - 增加1度");
  Serial.println("  - - 减少1度");
  Serial.println("  ++ - 增加5度");
  Serial.println("  -- - 减少5度");
  Serial.println("  r - 重置到90度");
  Serial.println("  p - 显示当前位置");
  Serial.println("==================");
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    if (input.length() > 0) {
      processCommand(input);
    }
  }
}

void processCommand(String cmd) {
  int newPos = currentPos;
  
  if (cmd == "+") {
    // 增加1度
    newPos = currentPos + 1;
    Serial.println("增加1度");
  } 
  else if (cmd == "-") {
    // 减少1度
    newPos = currentPos - 1;
    Serial.println("减少1度");
  }
  else if (cmd == "++" || cmd == "++") {
    // 增加5度
    newPos = currentPos + 5;
    Serial.println("增加5度");
  }
  else if (cmd == "--" || cmd == "--") {
    // 减少5度
    newPos = currentPos - 5;
    Serial.println("减少5度");
  }
  else if (cmd == "r" || cmd == "R") {
    // 重置到90度
    newPos = 90;
    Serial.println("重置到90度");
  }
  else if (cmd == "p" || cmd == "P") {
    // 显示当前位置
    Serial.print("当前位置: ");
    Serial.print(currentPos);
    Serial.println("度");
    return;
  }
  else {
    // 尝试解析为数字
    int angle = cmd.toInt();
    if (angle >= 0 && angle <= 180) {
      newPos = angle;
      Serial.print("移动到 ");
      Serial.print(angle);
      Serial.println("度");
    } else {
      Serial.println("错误: 无效命令或角度超出范围 (0-180)");
      return;
    }
  }
  
  // 限制角度范围
  if (newPos < 0) {
    newPos = 0;
    Serial.println("警告: 已限制到最小角度 0度");
  }
  if (newPos > 180) {
    newPos = 180;
    Serial.println("警告: 已限制到最大角度 180度");
  }
  
  // 缓慢移动舵机（每次1度）
  moveServoSlowly(currentPos, newPos);
  currentPos = newPos;
  
  Serial.print("→ 当前位置: ");
  Serial.print(currentPos);
  Serial.println("度");
}

void moveServoSlowly(int fromPos, int toPos) {
  // 确定移动方向
  int step = (toPos > fromPos) ? 1 : -1;
  
  // 逐步移动
  for (int pos = fromPos; pos != toPos; pos += step) {
    myservo.write(pos);
    delay(15);  // 每度之间延迟15毫秒，可调整速度
  }
  
  // 移动到最终位置
  myservo.write(toPos);
}
