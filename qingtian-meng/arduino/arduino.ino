#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

const int SOUND_SENSOR_PIN = 2; 
const int MP3_TX_PIN = 10;    
const int MP3_RX_PIN = 11;    

const unsigned long THRESHOLD_SHORT = 2000;  
const unsigned long THRESHOLD_MEDIUM = 8000;  
--
SoftwareSerial softSerial(MP3_TX_PIN, MP3_RX_PIN);
DFRobotDFPlayerMini myDFPlayer;

bool isMonitoring = true;       
unsigned long soundStartTime = 0; 
bool isSoundActive = false;  
unsigned long lastActionTime = 0; 
const unsigned long COOLDOWN_TIME = 1000; 

void setup() {
  Serial.begin(9600);
  softSerial.begin(9600);

  pinMode(SOUND_SENSOR_PIN, INPUT_PULLUP);

  Serial.println(F("..."));
  
  if (!myDFPlayer.begin(softSerial)) {
    Serial.println(F("DFPlayer err!"));
    while (true);
  }
  
  Serial.println(F("ready."));
  myDFPlayer.volume(30); 
}

void loop() {
  if (millis() - lastActionTime < COOLDOWN_TIME) {
    return; 
  }

  int sensorState = digitalRead(SOUND_SENSOR_PIN);

  if (sensorState == LOW) {
    if (!isSoundActive) {
      isSoundActive = true;
      soundStartTime = millis();
      Serial.println(F(">>"));
    }
  } 
  else {
    if (isSoundActive) {
      unsigned long duration = millis() - soundStartTime;
      
      Serial.print(F("<<  "));
      Serial.print(duration);
      Serial.println(F(" ms"));

      int trackToPlay = 0;
      if (duration < THRESHOLD_SHORT) {
        trackToPlay = 1;
        Serial.println(F("->  1)"));
      } 
      else if (duration >= THRESHOLD_SHORT && duration < THRESHOLD_MEDIUM) {
        trackToPlay = 2;
        Serial.println(F("->  2)"));
      } 
      else {
        trackToPlay = 3;
        Serial.println(F("->  3)"));
      }
      if (trackToPlay > 0) {
        myDFPlayer.playFolder(1, trackToPlay);
        lastActionTime = millis(); 
      }
      isSoundActive = false;
    }
  }
  if (isSoundActive && (millis() - soundStartTime > 3000)) {
    Serial.println(F("! "));
    isSoundActive = false;
  }
}