// 定义超声波传感器的引脚
const int trigPin = 7;
const int echoPin = 6;

const int pumpPin = 9;

long duration;
int distance;

void setup() {
  // 初始化串口通信
  Serial.begin(9600);
  
  // 设置超声波传感器的引脚模式
  pinMode(trigPin, OUTPUT); 
  pinMode(echoPin, INPUT); 
  
  // 设置气泵引脚为输出模式
  pinMode(pumpPin, OUTPUT);
}

void loop() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);//us
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);


  distance = duration * 0.034 / 2;

  Serial.println(distance);

  if (distance >= 0 && distance < 12.5) {
    digitalWrite(pumpPin, HIGH); 
  } else if (distance < 25) {

    analogWrite(pumpPin, 170); 

  } else if (distance < 37.5) {

    analogWrite(pumpPin, 85); 

  } else if (distance <= 50) {

    digitalWrite(pumpPin, LOW);

  } else {

    digitalWrite(pumpPin, LOW);

  }
  delay(100);
}