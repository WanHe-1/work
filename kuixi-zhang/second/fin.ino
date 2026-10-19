
/*
  Optical Heart Rate Detection (PBA Algorithm) using the MAX30105 Breakout
  By: Nathan Seidle @ SparkFun Electronics
  Date: October 2nd, 2016
  https://github.com/sparkfun/MAX30105_Breakout

  This is a demo to show the reading of heart rate or beats per minute (BPM) using
  a Penpheral Beat Amplitude (PBA) algorithm.

  Hardware Connections:
  MAX30102:
  - VIN = 3.3V or supported supply voltage
  - GND = GND
  - SDA = D2 (GPIO4, ESP8266)
  - SCL = D1 (GPIO5, ESP8266)

  RGB LED Strip:
  - DIN = D5 (GPIO14, ESP8266)
  - 5V = External 5V power supply
  - GND = Common GND
*/

#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <Adafruit_NeoPixel.h>

// ===== MAX30102 =====
#define SDA_PIN D2
#define SCL_PIN D1

MAX30105 particleSensor;

const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred

float beatsPerMinute;
int beatAvg;

// ===== RGB LED Strip =====
#define LED_PIN D5
#define LED_COUNT 160
#define IR_THRESHOLD 10000

#define MAX_BRIGHTNESS 160
#define FADE_STEP 2
#define FADE_INTERVAL 12

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

int currentBrightness = 0;
int targetBrightness = 0;
unsigned long lastFadeTime = 0;

void setup()
{
  Serial.begin(115200);

  // Initialize sensor
  Wire.begin(SDA_PIN, SCL_PIN);

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST))
  {
    while (1);
  }

  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED

  // Initialize RGB LED strip
  strip.begin();
  strip.setBrightness(0);
  strip.clear();
  strip.show();

  // Set warm white color
  for (int i = 0; i < LED_COUNT; i++)
  {
    strip.setPixelColor(i, strip.Color(255, 180, 100));
  }
  strip.show();
}

void loop()
{
  long irValue = particleSensor.getIR();

  if (checkForBeat(irValue) == true)
  {
    //We sensed a beat!
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20)
    {
      rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
      rateSpot %= RATE_SIZE; //Wrap variable

      //Take average of readings
      beatAvg = 0;
      for (byte x = 0; x < RATE_SIZE; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }

  // Only output IR value for Serial Plotter
  Serial.println(irValue);

  // Control RGB LED strip according to IR value
  if (irValue > IR_THRESHOLD)
    targetBrightness = MAX_BRIGHTNESS;
  else
    targetBrightness = 0;

  // Smooth fade in and fade out
  unsigned long now = millis();

  if (now - lastFadeTime >= FADE_INTERVAL)
  {
    lastFadeTime = now;

    if (currentBrightness < targetBrightness)
    {
      currentBrightness += FADE_STEP;

      if (currentBrightness > targetBrightness)
        currentBrightness = targetBrightness;
    }
    else if (currentBrightness > targetBrightness)
    {
      currentBrightness -= FADE_STEP;

      if (currentBrightness < targetBrightness)
        currentBrightness = targetBrightness;
    }

    strip.setBrightness(currentBrightness);
    strip.show();
  }
}
