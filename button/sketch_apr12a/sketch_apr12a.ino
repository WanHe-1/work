#include <SPI.h>

const int dataPin = 2;    // Data pin connected to DS on 74HC595
const int latchPin = 4;   // Latch pin connected to ST_CP on 74HC595
const int clockPin = 3;   // Clock pin connected to SH_CP on 74HC595
const int buttonPin = 5;  // Button connected to D5

int buttonState = 0;         // Current button state
int lastButtonState = 0;     // Last button state
unsigned long lastDebounceTime = 0;  // Last debounce time
unsigned long debounceDelay = 50;    // Debounce delay

int pressCount = 0;          // Number of button presses
//定义引脚与参数
void setup() {
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP); // Internal pull-up resistor

  SPI.begin();
}
//变为输出信号
void loop() {
  int reading = digitalRead(buttonPin); // Read button state

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) { // Check debounce delay
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == LOW) { // If button is pressed
        pressCount++;
        updateLasers();
      }
    }
  }

  lastButtonState = reading; // Update last button state
}
//防抖动
void updateLasers() {
  byte laserStates[19] = {0}; // 19 bytes for 152 bits (150 lasers + 2 extra)

  for (int i = 0; i < 150; i++) {
    if (i < min(pressCount, 50)) {
      setBitInArray(laserStates, i, 1); // Turn on green lasers
    } else if (i < min(pressCount, 100)) {
      setBitInArray(laserStates, i, 1); // Turn on blue lasers
    } else {
      setBitInArray(laserStates, i, 0); // Turn off lasers
    }
  }

  digitalWrite(latchPin, LOW); // Prepare to send data
  for (int i = 18; i >= 0; i--) {
    shiftOut(dataPin, clockPin, MSBFIRST, laserStates[i]);
  }
  digitalWrite(latchPin, HIGH); // Send data
}
//亮灯
void setBitInArray(byte array[], int bitIndex, int value) {
  int byteIndex = bitIndex / 8;
  int bitPosition = bitIndex % 8;

  if (value == 1) {
    array[byteIndex] |= (1 << bitPosition);
  } else {
    array[byteIndex] &= ~(1 << bitPosition);
  }
}
//计算灯


