
#include "HPD482.h"
#include "picture.h"
#define LED_PIN 13

HPD482 printer = HPD482();

bool isPrinting = false;
void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  Serial.begin(9600);
  printer.Print_SetDeep(2);
}
void loop() {
  if (isPrinting) {
    return;
  }

  if (Serial.available() > 0) {
    char signal = Serial.read();

    if (signal == '1') {
      isPrinting = true;

      digitalWrite(LED_PIN, HIGH);

      printer.Print_Img2Lcd(2, (u8*)Image1, 1);

      while (Serial.available() > 0) {
        Serial.read();
      }

      digitalWrite(LED_PIN, LOW);

      isPrinting = false;
    }
  }
}
