
/*
  MAX30102 IR / Heart Rate Detection
  Arduino Uno + WS2812B LED Strip

  MAX30102:
  VIN = Supported supply voltage
  GND = GND
  SDA = A4
  SCL = A5
  INT = Not connected

  WS2812B:
  DIN = D5
  5V  = External 5V power supply
  GND = Common GND
*/

#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <Adafruit_NeoPixel.h>

MAX30105 particleSensor;

// ===== Original heart-rate detection variables =====
const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;

float beatsPerMinute;
int beatAvg;

// ===== RGB LED strip settings =====
#define LED_PIN 5
#define LED_COUNT 160
#define IR_THRESHOLD 10000

#define MAX_BRIGHTNESS 160
#define FADE_STEP 2
#define FADE_INTERVAL 12

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

  // Arduino Uno I2C: SDA = A4, SCL = A5
  Wire.begin();

  // Initialize MAX30102
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST))
  {
    // Sensor initialization failed.
    // Output no extra text to keep Serial Plotter clean.
    while (1)
    {
      delay(100);
    }
  }

  // Keep the original sensor configuration
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0);

  // Initialize WS2812B strip
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

  // Original heart-rate detection algorithm
  if (checkForBeat(irValue) == true)
  {
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20)
    {
      rates[rateSpot++] = (byte)beatsPerMinute;
      rateSpot %= RATE_SIZE;

      // Calculate average heart rate
      beatAvg = 0;

      for (byte x = 0; x < RATE_SIZE; x++)
      {
        beatAvg += rates[x];
      }

      beatAvg /= RATE_SIZE;
    }
  }

  // Output only IR value for Serial Plotter
  Serial.println(irValue);

  // Control LED brightness according to IR value
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
