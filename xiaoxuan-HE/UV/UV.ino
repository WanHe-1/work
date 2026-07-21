#include <Wire.h>
#include "Adafruit_AHTX0.h"
Adafruit_AHTX0 aht;
const int uvPin = A0;
const int ledPin1 = 13;
const int ledPin2 = 12;

void setup() {
  Serial.begin(9600);

  if (!aht.begin()) {
    Serial.println("erro");
    while (1) delay(10);
  }

  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
}

void loop() {
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);
  float temperature = temp.temperature;

  int uvRaw = analogRead(uvPin);
  Serial.print(temperature, 1);
  Serial.println(uvRaw);

  if (temperature > 31.0 && uvRaw > 1) {
    digitalWrite(ledPin1, HIGH);
    digitalWrite(ledPin2, HIGH);
  } else {
    digitalWrite(ledPin1, LOW);
    digitalWrite(ledPin2, LOW);
    // digitalWrite(4, HIGH);
  }
  delay(1000); 
}