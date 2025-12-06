#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

#define OLED_ADDRESS 0x3C

Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
const int PRESSURE_SENSOR_1 = A0;
const int PRESSURE_SENSOR_2 = A1;
const int PRESSURE_SENSOR_3 = A2;

// Array to store sensor readings
int sensorValues[3];
int sensorReadings[3] = {0, 0, 0}; // For accumulating readings to compute average
int sampleCount = 0;
const int numSamples = 10; // Number of samples for averaging

void setup() {
  Serial.begin(9600);

  // Initialize I2C
  Wire.begin();
  delay(100);

  // Initialize OLED display
  Serial.println(F("Initializing SH1106 OLED..."));
  
  // SH1106 initialization (address, reset required?, I2C clock frequency)
  if (!display.begin(0x3C, true)) {
    Serial.println(F("SH1106 initialization failed!"));
    Serial.println(F("Please check:"));
    Serial.println(F("1. I2C address (try 0x3C or 0x3D)"));
    Serial.println(F("2. Wiring (SDA, SCL)"));
    Serial.println(F("3. Library installation (Adafruit SH110X)"));
    for (;;); // Halt program if initialization fails
  }
  
  Serial.println(F("SH1106 initialized successfully!"));
  
  // Configure display settings
  display.setRotation(0);           // Set display orientation
  display.invertDisplay(false);     // Normal display (non-inverted colors)
  
  // Clear display multiple times to ensure clean screen
  for(int i = 0; i < 3; i++) {
    display.clearDisplay();
    display.display();
    delay(100);
  }

  // Display initialization message
  display.setTextSize(1);
  display.setTextColor(1);
  display.setCursor(0, 0);
  display.println("Init OK!");
  display.display();
  delay(1000);
  
  // Clear screen again to prepare for data display
  display.clearDisplay();
  display.display();

  // Configure sensor pins as inputs
  pinMode(PRESSURE_SENSOR_1, INPUT);
  pinMode(PRESSURE_SENSOR_2, INPUT);
  pinMode(PRESSURE_SENSOR_3, INPUT);
}

void loop() {
  // Read values from the three pressure sensors
  sensorValues[0] = analogRead(PRESSURE_SENSOR_1);
  sensorValues[1] = analogRead(PRESSURE_SENSOR_2);
  sensorValues[2] = analogRead(PRESSURE_SENSOR_3);

  // Accumulate current readings
  for (int i = 0; i < 3; i++) {
    sensorReadings[i] += sensorValues[i];
  }
  sampleCount++;

  // Once enough samples are collected, compute averages and display
  if (sampleCount >= numSamples) {
    int avg1 = sensorReadings[0] / numSamples;
    int avg2 = sensorReadings[1] / numSamples;
    int avg3 = sensorReadings[2] / numSamples;

    // Clear display
    display.clearDisplay();

    // Set text properties – use larger font
    display.setTextSize(2);
    display.setTextColor(1);
    
    // Display Sensor 1
    display.setCursor(0, 0);
    display.print("P1:");
    display.println(avg1);

    // Display Sensor 2
    display.setCursor(0, 22);
    display.print("P2:");
    display.println(avg2);

    // Display Sensor 3
    display.setCursor(0, 44);
    display.print("P3:");
    display.println(avg3);

    display.display();

    // Reset accumulators and counter
    for (int i = 0; i < 3; i++) {
      sensorReadings[i] = 0;
    }
    sampleCount = 0;

    // Print to Serial for debugging
    Serial.print("P1: ");
    Serial.print(avg1);
    Serial.print(", P2: ");
    Serial.print(avg2);
    Serial.print(", P3: ");
    Serial.println(avg3);
  }

  // Short delay for stable sampling
  delay(10);
}