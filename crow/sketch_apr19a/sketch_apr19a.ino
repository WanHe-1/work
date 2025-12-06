#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

// Define pins
const int soundSensorPin = 9;     // Sound sensor connected to D2
const int relayPin = 12;           // Relay controlling air pump connected to D3
const int dfPlayerRxPin = 10;     // DFPlayer RX connected to D10
const int dfPlayerTxPin = 11;     // DFPlayer TX connected to D11

// Create software serial object
SoftwareSerial mySoftwareSerial(dfPlayerRxPin, dfPlayerTxPin);

// Create DFPlayer object
DFRobotDFPlayerMini myDFPlayer;

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  mySoftwareSerial.begin(9600);

  // Initialize relay pin
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);  // Turn off air pump by default

  // Initialize sound sensor pin
  pinMode(soundSensorPin, INPUT);

  // Initialize DFPlayer
  if (!myDFPlayer.begin(mySoftwareSerial)) {
    Serial.println(F("DFPlayer not found!"));
    while (true) {
      delay(100);
    }
  }

  Serial.println(F("DFPlayer initialized successfully!"));

  // Set DFPlayer volume (range: 0-30)
  myDFPlayer.volume(30);
}
void loop() {
  // Detect sound sensor status
  int soundState = digitalRead(soundSensorPin);

  if (soundState == HIGH) {  // If sound is detected
    if (digitalRead(relayPin) == LOW) {  // If the air pump is not running
      Serial.println("Sound detected!");

      // Turn on the air pump
      digitalWrite(relayPin, HIGH);
      Serial.println("Air pump started!");

      // Play music
      myDFPlayer.play(1);  // Play the first song
      Serial.println("Playing music...");
      delay(244000);
      myDFPlayer.stop(); // stop
    }
  } else {  // If no sound is detected
    if (digitalRead(relayPin) == HIGH) {  // If the air pump is running

      // Keep the air pump on for now (can be changed to turn off after a delay)

      digitalWrite(relayPin, LOW);

      // Stop playing music
    }
  }

  // Slight delay to prevent frequent triggering
  delay(200);
}