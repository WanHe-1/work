/***
 * 淘宝    ：https://qhebot.taobao.com/
***/
int sensorPin = A0;    // select the input pin for the potentiometer
float sensorValue = 0;  // variable to store the value coming from the sensor

void setup() {
  Serial.begin(115200);
  Serial.println("turbidity TEST......");
}

void loop() {
  sensorValue = analogRead(sensorPin);
  sensorValue = (sensorValue*100/1024);
  Serial.print("turbidity value is ");
  Serial.println(sensorValue);
  delay(300);
}
