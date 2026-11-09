#include <Wire.h>
#include "Adafruit_MPR121.h"
#include "FastLED.h"
#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

#define NUM_LEDS 9 
#define LED_DT 8 
#define LED_TYPE WS2812 
#define COLOR_ORDER RGB

CRGB leds[NUM_LEDS]; 
CHSV myHSVcolor(45, 255, 200);

// You can have up to 4 on one i2c bus but one is enough for testing!
Adafruit_MPR121 cap = Adafruit_MPR121();

// Keeps track of the last pins touched
// so we know when buttons are 'released'
uint8_t parseNum = 0;
uint8_t paletteNum = 0;
uint8_t startIndex = 0;
uint8_t chromatism = 50;
uint16_t lasttouched = 0;
uint16_t currtouched = 0;

void setup() {
  LEDS.addLeds<LED_TYPE, LED_DT, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(128);

  Serial.begin(9600);

  while (!Serial) { // needed to keep leonardo/micro from starting too fast!
    delay(10);
  }

  Serial.println("Adafruit MPR121 Capacitive Touch sensor test");

  // Default address is 0x5A, if tied to 3.3V its 0x5B
  // If tied to SDA its 0x5C and if SCL then 0x5D
  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
    while (1);
  }
  Serial.println("MPR121 found!");
}

void loop() {
  // Get the currently touched pads
  currtouched = cap.touched();

  for (uint8_t i = 0; i < 12; i++) {
    // it if *is* touched and *wasnt* touched before, alert!
    if ((currtouched & _BV(i)) && !(lasttouched & _BV(i)) ) {
      switch (i) {
        case 0:
          stateLed_ON(parseNum++);
          if (parseNum > 10) {
            parseNum = 0;
          }
          break;
        case 1: stateLed_OFF(); break;
        case 2:
          dynamicLED(paletteNum, startIndex);
          paletteNum++;
          if (paletteNum > 1) {
            paletteNum = 0;
          }
          break;
      }
    }
  }

  // reset our state
  lasttouched = currtouched;

  // comment out this line for detailed data from the sensor!
  return;

  // debugging info, what
  Serial.print("\t\t\t\t\t\t\t\t\t\t\t\t\t 0x"); Serial.println(cap.touched(), HEX);
  Serial.print("Filt: ");
  for (uint8_t i = 0; i < 12; i++) {
    Serial.print(cap.filteredData(i)); Serial.print("\t");
  }
  Serial.println();
  Serial.print("Base: ");
  for (uint8_t i = 0; i < 12; i++) {
    Serial.print(cap.baselineData(i)); Serial.print("\t");
  }
  Serial.println();

  // put a delay so it isn't overwhelming
  delay(100);
}

void stateLed_ON(uint8_t parseNum) {   //点亮单色调颜色
  switch (parseNum) {
    case 0: fill_solid(leds, NUM_LEDS, CRGB::Crimson);              FastLED.show(); break;
    case 1: fill_solid(leds, NUM_LEDS, CRGB::Aqua);                 FastLED.show(); break;
    case 2: fill_solid(leds, NUM_LEDS, CRGB::Amethyst);             FastLED.show(); break;
    case 3: fill_solid(leds, NUM_LEDS, CRGB::Blue);                 FastLED.show(); break;
    case 4: fill_solid(leds, NUM_LEDS, CRGB::Chartreuse);           FastLED.show(); break;
    case 5: fill_solid(leds, NUM_LEDS, CRGB::DarkOrange);           FastLED.show(); break;
    case 6: fill_solid(leds, NUM_LEDS, CRGB::DeepPink);             FastLED.show(); break;
    case 7: fill_solid(leds, NUM_LEDS, CRGB::GhostWhite);           FastLED.show(); break;
    case 8: fill_solid(leds, NUM_LEDS, CRGB::Gold);                 FastLED.show(); break;
    case 9: fill_solid(leds, NUM_LEDS, CRGB::GreenYellow);          FastLED.show(); break;
    case 10: fill_solid(leds, NUM_LEDS, CRGB::MediumSpringGreen);   FastLED.show(); break;
  }
  FastLED.show();
}

void stateLed_OFF() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
}

void dynamicLED(uint8_t paletteNum, uint8_t colorIndex) {
  switch (paletteNum) {
    case 0: gradientflowingLED(); break;
    case 1: FillLEDsFromPaletteColors(colorIndex); break;
  }
}

void gradientflowingLED() {
  for (int i = 0; i < NUM_LEDS; i++) {
    fill_solid(leds + i, 1, myHSVcolor);
    FastLED.show();
    myHSVcolor.h += 10;
    delay(50);

    fill_solid(leds + i, 1, CRGB::Black);
    FastLED.show();
    delay(50);
  }

  for (int i = NUM_LEDS; i > 0; i--) {
    fill_solid(leds + i, 1, myHSVcolor);
    FastLED.show();
    myHSVcolor.h += 10;
    delay(50);

    fill_solid(leds + i, 1, CRGB::Black);
    FastLED.show();
    delay(50);
  }
}

void FillLEDsFromPaletteColors(uint8_t colorIndex)
{
  for (int i = 0; i < chromatism; i++) {
    colorIndex++;
    for ( int j = 0; j < NUM_LEDS; ++j) {
      leds[j] = ColorFromPalette(RainbowColors_p, colorIndex, 255, LINEARBLEND);
      colorIndex += 3;
      FastLED.show();
      FastLED.delay(10);
    }
  }
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
}
