#include "HX711.h"

const int pressurePin = A0;

HX711 scale;
const int LOADCELL_DOUT_PIN = 2;   
const int LOADCELL_SCK_PIN = 3;    


const int motorPWM = 9;
const int PRESSURE_THRESHOLD = 500;  
const long TENSION_LOW = 5000;        
const long TENSION_HIGH = 15000;   

void setup() {
  pinMode(motorPWM, OUTPUT);
  analogWrite(motorPWM, 0); 
  Serial.begin(9600);

  // Initialize HX711
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale();    
  scale.tare();     
}
void loop() {
  int pressureValue = analogRead(pressurePin);
  long tensionValue = scale.read(); 

  int motorSpeed = 0;

  if (pressureValue >= PRESSURE_THRESHOLD) {
    motorSpeed = 85; 

    if (tensionValue >= TENSION_LOW) {
      if (tensionValue >= TENSION_HIGH) {
        motorSpeed = 254;
      } else {
        motorSpeed = 170; 
      }
    }
  }

  analogWrite(motorPWM, motorSpeed);
  delay(100);
}