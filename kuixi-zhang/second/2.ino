
/*
  Optical Heart Rate Detection (PBA Algorithm) using the MAX30105 Breakout
  By: Nathan Seidle @ SparkFun Electronics
  Date: October 2nd, 2016
  https://github.com/sparkfun/MAX30105_Breakout

  This is a demo to show the reading of heart rate or beats per minute (BPM)
  using a Peripheral Beat Amplitude (PBA) algorithm.

  Hardware Connections (ESP8266 NodeMCU):
  MAX30102:
  - VIN = Supported supply voltage
  - GND = GND
  - SDA = D2 (GPIO4)
  - SCL = D1 (GPIO5)
  - INT = Not connected

  WS2812B RGB LED Strip:
  - DIN = D5 (GPIO14)
  - 5V = External 5V power supply
  - GND = Common GND
*/

#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <Adafruit_NeoPixel.h>

MAX30105 particleSensor;

// ===== Original heart-rate detection variables =====
const byte RATE_SIZE = 4; // Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE];    // Array of heart rates
byte rateSpot = 0;
long lastBeat = 0;        // Time at which the last beat occurred

float beatsPerMinute;
int beatAvg;

// ===== RGB LED strip settings =====
#define LED_PIN 14          // D5 = GPIO14 on ESP8266 NodeMCU
#define LED_COUNT 160       // Change to match your LED count
#define IR_THRESHOLD 10000

#define MAX_BRIGHTNESS 160  // Maximum brightness: 0-255
#define FADE_STEP 2
#define FADE_INTERVAL 12    // Fade update interval in milliseconds

Adafruit_NeoPixel strip(
  LED_COUNT,
  LED_PIN,
  NEO_GRB + NEO_KHZ800
);

int currentBrightness = 0;
int targetBrightness = 0;
unsigned long lastFadeTime = 0;

void setup()
{
  Serial.begin(115200);

  // Initialize sensor using the original default I2C configuration
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST))
  {
    while (1)
    {
      delay(100);
    }
  }

  particleSensor.setup(); // Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); // Red LED at low brightness
  particleSensor.setPulseAmplitudeGreen(0);  // Turn off Green LED

  // Initialize RGB LED strip
  strip.begin();
  strip.setBrightness(0);
  strip.clear();

  // Set all LEDs to warm white
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
    // We sensed a beat!
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20)
    {
      rates[rateSpot++] = (byte)beatsPerMinute;
      rateSpot %= RATE_SIZE;

      // Take average of readings
      beatAvg = 0;
      for (byte x = 0; x < RATE_SIZE; x++)
      {
        beatAvg += rates[x];
      }
      beatAvg /= RATE_SIZE;
    }
  }

  // Output only IR value for Arduino Serial Plotter
  Serial.println(irValue);

  // Control LED strip according to IR value
  if (irValue > IR_THRESHOLD)
  {
    targetBrightness = MAX_BRIGHTNESS;
  }
  else
  {
    targetBrightness = 0;
  }

  // Smooth fade in and fade out
  unsigned long now = millis();

  if (now - lastFadeTime >= FADE_INTERVAL)
  {
    lastFadeTime = now;

    if (currentBrightness < targetBrightness)
    {
      currentBrightness += FADE_STEP;

      if (currentBrightness > targetBrightness)
      {
        currentBrightness = targetBrightness;
      }
    }
    else if (currentBrightness > targetBrightness)
    {
      currentBrightness -= FADE_STEP;

      if (currentBrightness < targetBrightness)
      {
        currentBrightness = targetBrightness;
      }
    }

    strip.setBrightness(currentBrightness);
    strip.show();
  }
}
