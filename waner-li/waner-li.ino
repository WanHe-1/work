#include <Wire.h>
#include <Adafruit_VL6180X.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include "MAX30105.h"
#include "heartRate.h"

Adafruit_VL6180X tof = Adafruit_VL610X();
MAX30105 particleSensor;

SoftwareSerial dfSerial(2, 3); 
DFRobotDFPlayerMini dfplayer;

unsigned long lastBeat = 0;
float bpm = 0;

void setup() {
  Serial.begin(9600);
  Wire.begin();

  if (!tof.begin()) {
    Serial.println("VL6180X failed");
    while (1);
  }

  dfSerial.begin(9600);
  dfplayer.begin(dfSerial);
  dfplayer.volume(25);

  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("ERRO");
    while (1);
  }
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0); 
}
void loop() {
uint8_t range = tof.readRange();
if (tof.readRangeStatus() == VL6180X_ERROR_NONE) {
  Serial.print(range);
  if (range <= 50) {
    dfplayer.play(1);
  }
}

long irValue = particleSensor.getIR();

if (checkForBeat(irValue)) {
  unsigned long now = millis();
  if (lastBeat > 0) {
    float delta = (now - lastBeat) / 1000.0; 
    bpm = 60.0 / delta;
    Serial.print(bpm);
    if (bpm < 50 || bpm > 130) {
      dfplayer.play(2);
    }
  }
  lastBeat = now;
}
  delay(50); 
}