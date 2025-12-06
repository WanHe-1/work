#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

int analogPin = A0;     // Analog pin for reading infrared signal (0-1023)
int val = 0;            // Variable to store the infrared signal value

// Define software serial port for communication with DFPlayer Mini
SoftwareSerial mySoftwareSerial(12, 13); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

void setup() {
  Serial.begin(9600);
  mySoftwareSerial.begin(9600); // Initialize software serial for communication with DFPlayer

  // Initialize DFPlayer Mini
  if (!myDFPlayer.begin(mySoftwareSerial)) {
    Serial.println("DFPlayer Mini initialization failed!");
    while (true); // Stop here if initialization fails
  }
  Serial.println("DFPlayer Mini initialized successfully!");

  myDFPlayer.volume(30); // Set volume (0-30), adjust as needed
}


void loop() {
  val = analogRead(analogPin); // Read the infrared signal (0-1023)
  Serial.print(val);

  if (val < 30) { // Threshold for detecting an infrared signal
    Serial.println("Infrared signal detected, playing music...");
    myDFPlayer.play(1); // Play the first song, ensure you have the corresponding audio file
    delay(10000); // Delay in milliseconds

  } 
  else {
    Serial.println("No signal detected");
    myDFPlayer.stop(); // stop
  }

  delay(1000); // Delay for 1 second before continuing detection
}
