#include <Wire.h>
#include "Adafruit_MPR121.h"

Adafruit_MPR121 cap = Adafruit_MPR121();

bool touched = false;
uint16_t baseline = 0;
uint16_t touchThreshold = 0;
uint16_t releaseThreshold = 0;

void setup() {
  Serial.begin(9600);

  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 ERROR");
    while (1);
  }

  delay(1000);
  uint32_t total = 0;

  for (int i = 0; i < 20; i++) {
    total += cap.filteredData(0);
    delay(50);
  }
  baseline = total / 20;
  touchThreshold = baseline * 0.7;
  releaseThreshold = baseline * 0.85;

  Serial.println("MPR121 READY");
}
void loop() {
  uint16_t value = cap.filteredData(0);

  if (!touched && value < touchThreshold) {
    Serial.println("100");
    touched = true;
  }
  if (touched && value >= releaseThreshold) {
    touched = false;
  }
  delay(20);
}
