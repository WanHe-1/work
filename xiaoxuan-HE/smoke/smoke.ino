#include <OneWire.h>
#include <DallasTemperature.h>

const int smokePin = A1;
const int tempPin  = 11;

const int motorIn1 = 12;
const int motorIn2 = 13;
const int motorPWM = 10;    

const int SMOKE_THRESHOLD = 150;
const int MOTOR_SPEED = 100;   

OneWire oneWire(tempPin);
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(9600);
  sensors.begin();

  pinMode(motorIn1, OUTPUT);
  pinMode(motorIn2, OUTPUT);
  pinMode(motorPWM, OUTPUT);

  stopMotor();
}

void loop() {
  int smokeValue = analogRead(smokePin);

  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);
  Serial.print(smokeValue);
  Serial.print(temperature, 1);

  if (smokeValue > SMOKE_THRESHOLD && temperature > 31.0) {
    runMotorFor(1000);
    delay(2000);
  }

  delay(500);
}
void runMotor() {
  digitalWrite(motorIn1, HIGH);
  digitalWrite(motorIn2, LOW);
  analogWrite(motorPWM, MOTOR_SPEED);  
}
void stopMotor() {
  digitalWrite(motorIn1, LOW);
  digitalWrite(motorIn2, LOW);
  analogWrite(motorPWM, 0);
}
void runMotorFor(unsigned long ms) {
  runMotor();
  delay(ms);
  stopMotor();
}
