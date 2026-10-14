const byte muscleSensorPin = A0;
const byte ad8232SensorPin = A1;
const byte leadOffPositivePin = 10;
const byte leadOffNegativePin = 11;
const byte outputPin = 9;  // Arduino Nano PWM pin

// Analog input range on Arduino Nano is 0-1023. Adjust these after
// observing the values printed by Serial Monitor.
const int muscleThreshold = 700;
const int ad8232Threshold = 700;

const unsigned long rampTimeMs = 1000;  // Gradually reach 5 V
const unsigned long holdTimeMs = 500;   // Keep 5 V for 0.5 s

bool triggerArmed = true;
unsigned long outputStartedAt = 0;

void setup() {
  Serial.begin(115200);

  pinMode(muscleSensorPin, INPUT);
  pinMode(ad8232SensorPin, INPUT);
  pinMode(leadOffPositivePin, INPUT);
  pinMode(leadOffNegativePin, INPUT);
  pinMode(outputPin, OUTPUT);
  analogWrite(outputPin, 0);
}

void loop() {
  const int muscleValue = analogRead(muscleSensorPin);
  const int ad8232Value = analogRead(ad8232SensorPin);
  const bool leadOff = digitalRead(leadOffPositivePin) == HIGH ||
                       digitalRead(leadOffNegativePin) == HIGH;
  const bool bothValuesAreHigh = muscleValue >= muscleThreshold &&
                                 ad8232Value >= ad8232Threshold;
  const unsigned long now = millis();

  Serial.print("Muscle:");
  Serial.print(muscleValue);
  Serial.print('\t');
  Serial.print("AD8232:");
  Serial.println(ad8232Value);

  // Trigger only once for each high-level event. The sensors must return
  // below their thresholds before the next event can start.
  if (triggerArmed && !leadOff && bothValuesAreHigh) {
    triggerArmed = false;
    outputStartedAt = now;
  }

  if (!triggerArmed) {
    const unsigned long elapsed = now - outputStartedAt;

    if (elapsed < rampTimeMs) {
      const int pwmValue = (long)elapsed * 255 / rampTimeMs;
      analogWrite(outputPin, pwmValue);
    } else if (elapsed < rampTimeMs + holdTimeMs) {
      analogWrite(outputPin, 255);  // About 5 V through PWM
    } else {
      analogWrite(outputPin, 0);

      if (!bothValuesAreHigh || leadOff) {
        triggerArmed = true;
      }
    }
  }

  delay(2);
}
