#include <Adafruit_NeoPixel.h>

const int turbidityPin = A0;
const int turbidityThreshold = 90;
const int lightCount = 173;

const int neoPixelPins[] = {
  8,
  9,
  10,
  11,
  12,
  13
};


const int stripCount = 6;

Adafruit_NeoPixel strips[stripCount] = {

  Adafruit_NeoPixel(
    lightCount,
    8,
    NEO_GRB + NEO_KHZ800
  ),

  Adafruit_NeoPixel(
    lightCount,
    9,
    NEO_GRB + NEO_KHZ800
  ),

  Adafruit_NeoPixel(
    lightCount,
    10,
    NEO_GRB + NEO_KHZ800
  ),

  Adafruit_NeoPixel(
    lightCount,
    11,
    NEO_GRB + NEO_KHZ800
  ),

  Adafruit_NeoPixel(
    lightCount,
    12,
    NEO_GRB + NEO_KHZ800
  ),

  Adafruit_NeoPixel(
    lightCount,
    13,
    NEO_GRB + NEO_KHZ800
  )
};

const unsigned long transitionTime = 500;
const unsigned long confirmTime = 1000;
const unsigned long sensorInterval = 50;
bool currentRed = false;
bool targetRed = false;
bool transitioning = false;
unsigned long transitionStartTime = 0;
unsigned long conditionStartTime = 0;
bool conditionTiming = false;
unsigned long lastSensorRead = 0;

void setRgbColor(
  uint8_t redValue,
  uint8_t greenValue,
  uint8_t blueValue
) {

  uint32_t color;
  for (int s = 0; s < stripCount; s++) {

    color = strips[s].Color(
      redValue,
      greenValue,
      blueValue
    );

    for (int i = 0; i < lightCount; i++) {

      strips[s].setPixelColor(
        i,
        color
      );
    }
  }

  for (int s = 0; s < stripCount; s++) {
    strips[s].show();
  }
}
void updateTransition() {

  if (!transitioning) {
    return;
  }


  unsigned long elapsed =
    millis() - transitionStartTime;

  if (elapsed >= transitionTime) {

    if (targetRed) {

      setRgbColor(
        255,
        0,
        0
      );

      currentRed = true;

    } else {

      setRgbColor(
        0,
        255,
        0
      );

      currentRed = false;
    }


    transitioning = false;

    return;
  }

  float progress =
    (float)elapsed / transitionTime;


  // SmoothStep
  float smoothProgress =
    progress *
    progress *
    (3.0 - 2.0 * progress);


  uint8_t redValue;

  uint8_t greenValue;

  if (targetRed) {

    redValue =
      255.0 * smoothProgress;

    greenValue =
      255.0 * (1.0 - smoothProgress);

  }

  else {

    redValue =
      255.0 * (1.0 - smoothProgress);

    greenValue =
      255.0 * smoothProgress;
  }

  setRgbColor(
    redValue,
    greenValue,
    0
  );
}
void startTransition(bool toRed) {
  targetRed = toRed;
  transitioning = true;
  transitionStartTime = millis();
  if (toRed) {
    Serial.println("r");
  } else {
    Serial.println("g");
  }
}
void setup() {
  Serial.begin(9600);
  for (int s = 0; s < stripCount; s++) {
    strips[s].begin();
    strips[s].setBrightness(100);
    strips[s].show();
  }

  setRgbColor(
    0,
    255,
    0
  );


  delay(500);


  Serial.println(
    turbidityThreshold
  );
}

void loop() {

  unsigned long now = millis();
  updateTransition();

  if (
    now - lastSensorRead >=
    sensorInterval
  ) {

    lastSensorRead = now;


    int turbidityValue =
      analogRead(turbidityPin);


    Serial.print(
      "Turbidity sensor value: "
    );

    Serial.println(
      turbidityValue
    );

    if (
      turbidityValue >
      turbidityThreshold
    ) {

      // 已经是红色
      if (
        currentRed ||
        (transitioning && targetRed)
      ) {

        conditionTiming = false;

      } else {

        // 开始计时
        if (!conditionTiming) {

          conditionTiming = true;

          conditionStartTime = now;
        }


        // 持续超过 confirmTime
        if (
          conditionTiming &&
          now - conditionStartTime >=
          confirmTime
        ) {

          startTransition(true);

          conditionTiming = false;
        }
      }
    }

    else {
      // 
      if (
        !currentRed ||
        (transitioning && !targetRed)
      ) {

        conditionTiming = false;

      } else {

        // 
        if (!conditionTiming) {

          conditionTiming = true;

          conditionStartTime = now;
        }

        if (
          conditionTiming &&
          now - conditionStartTime >=
          confirmTime
        ) {

          startTransition(false);

          conditionTiming = false;
        }
      }
    }
  }
}