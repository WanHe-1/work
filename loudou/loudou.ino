#include <Servo.h>

Servo myServo1;  // 第一个舵机对象
Servo myServo2;  // 第二个舵机对象

int servoPin1 = 9;   
int servoPin2 = 10; 
int pressurePin = A0; 
int pressureValue = 0;

int angle = 1;       
int direction = 1;   

void setup() {
  myServo1.attach(servoPin1); 
  myServo2.attach(servoPin2); 
  Serial.begin(9600);          // 初始化串口通信
}

void loop() {
  pressureValue = analogRead(pressurePin);  // 读取压力值

  Serial.print("Pressure Value: ");
  Serial.println(pressureValue);

  if (pressureValue < 500) {
    // 来回运动
    angle += direction * 1;

    if (angle >= 180 || angle <= 1) {
      direction *= -1;  // 改变方向
    }

    myServo1.write(angle);        // 控制第一个舵机
    myServo2.write(180 - angle);  
    delay(25);                    // 控制速度ms
  } 
  else 
  {

    myServo1.write(88);   
    myServo2.write(88);         
    delay(100);     
  }
}



