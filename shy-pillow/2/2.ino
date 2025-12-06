#include "Arduino.h"
#include "DFRobotDFPlayerMini.h"

// Pin definitions
const int trigPin = 5;
const int echoPin = 6;
const int motorENA = 11;
const int motorIN1 = 10;
const int motorIN2 = 8;
const int ledPin = 3;

#include <SoftwareSerial.h> 
SoftwareSerial mySoftwareSerial(12, 13); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

// Distance settings
const int MAX_DISTANCE = 200;
const int MIN_DISTANCE = 2;

// Audio file mapping
const int AUDIO_FILE_1 = 1; // 40-60cm
const int AUDIO_FILE_2 = 2; // 30-40cm
const int AUDIO_FILE_3 = 3; // 20-30cm
const int AUDIO_FILE_4 = 4; // 10-20cm
const int AUDIO_FILE_5 = 5; // 0-10cm

float measureDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  float distance = duration * 0.034 / 2;

  if (distance < MIN_DISTANCE || distance > MAX_DISTANCE) {
    distance = MAX_DISTANCE;
  }
  return distance;
}
                                                                                                                                  
unsigned long lastPlayTime = 0;
const long playInterval = 1000;

unsigned long audioStartTime = 0;
const long audioDuration = 2000;
bool isAudioPlaying = false;

void setup() {
  Serial.begin(9600);
  mySoftwareSerial.begin(9600);

  Serial.println(F("Initializing DFPlayer Mini..."));
  delay(1000);

  if (!myDFPlayer.begin(mySoftwareSerial)) {
    Serial.println(F("DFPlayer Mini initialization failed!"));
    while (true);
  }
  Serial.println(F("DFPlayer Mini initialized."));

  myDFPlayer.volume(30);
  myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);
  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);

  // Test playback
  myDFPlayer.play(1);
  delay(1000);
  myDFPlayer.stop();

  pinMode(motorENA, OUTPUT);
  pinMode(motorIN1, OUTPUT);
  pinMode(motorIN2, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  Serial.println(F("Ultrasonic Sensor Control Started"));
}

void loop() {
  float distance_cm = measureDistance();
  Serial.print(F("Distance: "));
  Serial.print(distance_cm);
  Serial.println(F(" cm"));

  // Motor speed control
  int motorSpeed;
  if (distance_cm < 40) {
    motorSpeed = 255;
  } else if (distance_cm > 180) {
    motorSpeed = 0;
  } else {
    motorSpeed = map(distance_cm, 180, 40, 0, 255);
    motorSpeed = constrain(motorSpeed, 0, 255);
  }

  digitalWrite(motorIN1, HIGH);
  digitalWrite(motorIN2, LOW);
  analogWrite(motorENA, motorSpeed);
  Serial.print(F("Motor Speed: "));
  Serial.println(motorSpeed);

  // LED brightness control
  int ledBrightness = 0;
  if (distance_cm > 180) {
    ledBrightness = 0;
  } else if (distance_cm >= 140) {
    ledBrightness = map(distance_cm, 140, 180, 40, 80);
    ledBrightness = constrain(ledBrightness, 40, 80);
  } else if (distance_cm > 120) {
    ledBrightness = map(distance_cm, 120, 140, 80, 120);
    ledBrightness = constrain(ledBrightness, 80, 120);
  } else if (distance_cm > 80) {
    ledBrightness = map(distance_cm, 80, 120, 120, 180);
    ledBrightness = constrain(ledBrightness, 120, 180);
  } else if (distance_cm > 40) {
    ledBrightness = map(distance_cm, 40, 80, 180, 220);
    ledBrightness = constrain(ledBrightness, 180, 220);
  } else {
    ledBrightness = map(distance_cm, 0, 40, 220, 255);
    ledBrightness = constrain(ledBrightness, 220, 255);s
  }
  analogWrite(ledPin, ledBrightness);
  Serial.print(F("LED Brightness: "));
  Serial.println(ledBrightness);

  // Audio playback
  unsigned long currentMillis = millis();

  if (isAudioPlaying && (currentMillis - audioStartTime >= audioDuration)) {
    myDFPlayer.stop();
    isAudioPlaying = false;
    Serial.println(F("Audio stopped after 2s"));
  }

  if (!isAudioPlaying && (currentMillis - lastPlayTime > playInterval)) {
    bool played = false;
    if (distance_cm > 40 && distance_cm <= 60) {
      myDFPlayer.play(AUDIO_FILE_1);
      Serial.println(F("Playing Audio 1 (40-60cm)"));
      played = true;
    } else if (distance_cm > 30 && distance_cm <= 40) {
      myDFPlayer.play(AUDIO_FILE_2);
      Serial.println(F("Playing Audio 2 (30-40cm)"));
      played = true;
    } else if (distance_cm > 20 && distance_cm <= 30) {
      myDFPlayer.play(AUDIO_FILE_3);
      Serial.println(F("Playing Audio 3 (20-30cm)"));
      played = true;
    } else if (distance_cm > 10 && distance_cm <= 20) {
      myDFPlayer.play(AUDIO_FILE_4);
      Serial.println(F("Playing Audio 4 (10-20cm)"));
      played = true;
    } else if (distance_cm <= 10) {
      myDFPlayer.play(AUDIO_FILE_5);
      Serial.println(F("Playing Audio 5 (0-10cm)"));
      played = true;
    }

    if (played) {
      isAudioPlaying = true;
      audioStartTime = currentMillis;
      lastPlayTime = currentMillis;
    }
  }

  delay(300);
}