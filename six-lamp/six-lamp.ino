// Define Pins
const int PRESSURE_SENSOR_PIN = A0;

const int ENABLE_1_PIN = 3; // PWM pin for Group 1
const int IN1_1_PIN = 4;    // Direction control pin 1 for Group 1
const int IN2_1_PIN = 5;    // Direction control pin 2 for Group 1

const int ENABLE_2_PIN = 9; // PWM pin for Group 2
const int IN1_2_PIN = 7;    // Direction control pin 1 for Group 2
const int IN2_2_PIN = 8;    // Direction control pin 2 for Group 2

// State Variables
int group1Brightness = 255; // Initial brightness for Group 1 (constantly on)
int group2Brightness = 0;   // Initial brightness for Group 2 (constantly off)
const int TARGET_BRIGHTNESS_TOUCHED = 90; // Target brightness when triggered
bool isAdjusting = false; // Flag to indicate if brightness adjustment is in progress

// Configuration Parameters
const int PRESSURE_THRESHOLD = 50; // Pressure trigger threshold
const int FADE_STEP = 2;           // Brightness change step per loop
const int LOOP_DELAY = 20;         // Main loop delay (milliseconds)

void setup() {
  // Initialize Serial Monitor (for debugging)
  Serial.begin(9600);

  // Set L298N control pins
  pinMode(ENABLE_1_PIN, OUTPUT);
  pinMode(IN1_1_PIN, OUTPUT);
  pinMode(IN2_1_PIN, OUTPUT);
  
  pinMode(ENABLE_2_PIN, OUTPUT);
  pinMode(IN1_2_PIN, OUTPUT);
  pinMode(IN2_2_PIN, OUTPUT);

  // Set L298N direction (enable forward drive)
  digitalWrite(IN1_1_PIN, HIGH);
  digitalWrite(IN2_1_PIN, LOW);
  digitalWrite(IN1_2_PIN, HIGH);
  digitalWrite(IN2_2_PIN, LOW);

  // Initialize light states
  analogWrite(ENABLE_1_PIN, group1Brightness);
  analogWrite(ENABLE_2_PIN, group2Brightness);

}

void loop() {
  // Read pressure sensor value
  int sensorValue = analogRead(PRESSURE_SENSOR_PIN);
  Serial.println(sensorValue); // Print sensor value for debugging

  // --- Modify trigger condition ---
  // Check if pressure is detected (trigger when sensor value is less than threshold)
  if (sensorValue < PRESSURE_THRESHOLD) {
    isAdjusting = true; // Start adjusting to target brightness

  } else {
    // If no pressure is detected (sensor value >= threshold), adjust back to initial state
    if (group1Brightness < 255 || group2Brightness > 0) {
         isAdjusting = true; // Start adjusting back to initial brightness

    }
  }

  // If brightness adjustment is needed
  if (isAdjusting) {
    // Smoothly adjust brightness for Group 1
    if (sensorValue < PRESSURE_THRESHOLD) {
        // If triggered (< threshold), decrease brightness towards target
        if (group1Brightness > TARGET_BRIGHTNESS_TOUCHED) {
            group1Brightness = max(group1Brightness - FADE_STEP, TARGET_BRIGHTNESS_TOUCHED);
        }
    } else {
        // If not triggered (>= threshold), increase brightness back to 255
        if (group1Brightness < 255) {
            group1Brightness = min(group1Brightness + FADE_STEP, 255);
        }
    }

    // Smoothly adjust brightness for Group 2
    if (sensorValue < PRESSURE_THRESHOLD) {
        // If triggered (< threshold), increase brightness towards target
        if (group2Brightness < TARGET_BRIGHTNESS_TOUCHED) {
            group2Brightness = min(group2Brightness + FADE_STEP, TARGET_BRIGHTNESS_TOUCHED);
        }
    } else {
        // If not triggered (>= threshold), decrease brightness back to 0
        if (group2Brightness > 0) {
            group2Brightness = max(group2Brightness - FADE_STEP, 0);
        }
    }


    // Update PWM output
    analogWrite(ENABLE_1_PIN, group1Brightness);
    analogWrite(ENABLE_2_PIN, group2Brightness);

    // Check if adjustment is complete (based on current state)
    bool adjustmentComplete = false;
    if (sensorValue < PRESSURE_THRESHOLD) {
        // If triggered, check if both groups reached target brightness
        adjustmentComplete = (group1Brightness == TARGET_BRIGHTNESS_TOUCHED && group2Brightness == TARGET_BRIGHTNESS_TOUCHED);
    } else {
        // If not triggered, check if both groups returned to initial brightness
        adjustmentComplete = (group1Brightness == 255 && group2Brightness == 0);
    }

    if (adjustmentComplete) {
      isAdjusting = false; // Stop adjustment
    }
  }

  delay(LOOP_DELAY); // Delay to control loop speed and smoothness of brightness change
}



