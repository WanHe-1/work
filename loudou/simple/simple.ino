
#include <Servo.h>

Servo myServo;  // 创建一个Servo对象

void setup() {
  myServo.attach(9);  // 将舵机连接到数字引脚9
}

void loop() {
  myServo.write(50);    // 将舵机旋转到0°

}