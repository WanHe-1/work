





const int sensorPins[] = {A0, A1, A2, A3};
const int numSensors = 4;          // Number of sensors
const int sampleCount = 5;         // Number of samples per sensor (for smoothing)

// Sensor calibration parameters (fixed range)
int sensorMin[] = {0, 0, 0, 0};    // Minimum ADC value
int sensorMax[] = {1023, 1023, 1023, 1023};  // Maximum ADC value
bool calibrated = true;            // Calibration status (fixed range, no calibration needed)

// Weight conversion parameters (adjust based on actual sensors)
const float maxWeight = 4500.0;    // Maximum weight per sensor (grams)
const int maxADC = 1023;           // Maximum ADC value

// Data storage
int sensorValues[numSensors];      // Raw sensor readings
int smoothedValues[numSensors];    // Smoothed sensor values
float weightValues[numSensors];    // Weight values (grams)
float totalWeight = 0;             // Total weight (grams)
float previousWeightValues[numSensors]; // Previous weight values for change detection
bool pressureChanged = false;      // Flag to indicate pressure change

// Pressure level for TouchDesigner (0-5)
int pressureLevel = 0;             // Current pressure level
const float maxPressureThreshold = 18000.0; // Maximum pressure for level 5 (grams)

void setup() {
  Serial.begin(9600);              // Initialize serial communication
  Serial.println("Pressure Sensor System - Range: 0-4500g per sensor");
  Serial.println("Debug Mode - ADC Values:");
  
  // Display initial ADC readings for debugging
  for (int i = 0; i < 10; i++) {
    Serial.print("ADC: ");
    for (int j = 0; j < numSensors; j++) {
      int adcValue = analogRead(sensorPins[j]);
      Serial.print("S");
      Serial.print(j + 1);
      Serial.print(":");
      Serial.print(adcValue);
      Serial.print(" ");
    }
    Serial.println();
    delay(500);
  }
  Serial.println("Start normal data output:");
}

void loop() {
  readSensors();        // Read all sensors
  smoothData();         // Smooth the data
  convertToWeight();    // Convert to weight (grams)
  calculateTotalWeight(); // Calculate total weight
  detectPressureChanges(); // Detect pressure changes
  calculatePressureLevel(); // Calculate pressure level for TouchDesigner
  
  // Print formatted sensor data with change indicator
  printSensorData();
  
  delay(100);  // 100ms sampling interval for improved responsiveness
}

// Read all sensor data
void readSensors() {
  for (int i = 0; i < numSensors; i++) {
    int sum = 0;
    // Take multiple samples and average them
    for (int j = 0; j < sampleCount; j++) {
      sum += analogRead(sensorPins[i]);
      delay(2);  // Short delay to ensure stable reading
    }
    sensorValues[i] = sum / sampleCount;
  }
}

// Smooth the sensor data with reduced smoothing for more responsive changes
void smoothData() {
  for (int i = 0; i < numSensors; i++) {
    // Reduced smoothing: more weight on new readings for better responsiveness
    // Changed from 0.7/0.3 to 0.4/0.6 for more sensitive pressure detection
    smoothedValues[i] = (smoothedValues[i] * 0.4) + (sensorValues[i] * 0.6);
  }
}

// Convert ADC values to weight (grams)
void convertToWeight() {
  for (int i = 0; i < numSensors; i++) {
    // Enhanced sensitivity: detect pressure changes from baseline (~715)
    // Lower threshold for more sensitive detection
    float weight = 0;
    int baseline = 715;  // Typical no-load ADC value
    int threshold = 700; // Lower threshold for pressure detection
    
    if (smoothedValues[i] < threshold) {
      // Map ADC range 0–threshold to weight range maxWeight–0g (inverse relationship)
      // Use smaller range for more sensitive weight detection
      weight = map(smoothedValues[i], 0, threshold, maxWeight, 0);
    } else if (smoothedValues[i] < baseline) {
      // Light pressure detection between threshold and baseline
      // Map this range to 0-100g for fine pressure detection
      weight = map(smoothedValues[i], threshold, baseline, 100, 0);
    } else {
      // No pressure detected → weight = 0
      weight = 0;
    }
    
    weight = constrain(weight, 0, maxWeight);  // Clamp between 0 and 4500g
    weight *= 8;  // Apply calibration factor to correct weight readings
    weightValues[i] = weight;
  }
}

// Calculate total weight from all sensors
void calculateTotalWeight() {
  totalWeight = 0;
  for (int i = 0; i < numSensors; i++) {
    totalWeight += weightValues[i];
  }
}

// Calculate pressure level (0-5) for TouchDesigner
void calculatePressureLevel() {
  // Divide total weight into 5 levels
  // Level 0: 0g (no pressure)
  // Level 1: 0-3600g (light pressure)
  // Level 2: 3600-7200g (light-medium pressure)
  // Level 3: 7200-10800g (medium pressure)
  // Level 4: 10800-14400g (medium-heavy pressure)
  // Level 5: 14400g+ (heavy pressure)
  
  if (totalWeight < 10) {
    pressureLevel = 0;
  } else if (totalWeight < 3600) {
    pressureLevel = 1;
  } else if (totalWeight < 7200) {
    pressureLevel = 2;
  } else if (totalWeight < 10800) {
    pressureLevel = 3;
  } else if (totalWeight < 14400) {
    pressureLevel = 4;
  } else {
    pressureLevel = 5;
  }
}

// Detect pressure changes for enhanced responsiveness
void detectPressureChanges() {
  pressureChanged = false;
  
  const float changeThreshold = 15.0;  // Minimum change to detect (grams)
  
  for (int i = 0; i < numSensors; i++) {
    float weightChange = abs(weightValues[i] - previousWeightValues[i]);
    if (weightChange > changeThreshold) {
      pressureChanged = true;
    }
    previousWeightValues[i] = weightValues[i];  // Update previous values
  }
}

// Print sensor and total weight data with change indicator
void printSensorData() {
  // Print individual sensor weights (grams)
  for (int i = 0; i < numSensors; i++) {
    Serial.print("S");
    Serial.print(i + 1);
    Serial.print(":");
    Serial.print(weightValues[i], 1);  // 1 decimal place
    Serial.print("g");
    if (i < numSensors - 1) Serial.print(" ");
  }
  
  // Print total weight
  Serial.print(" | Total:");
  Serial.print(totalWeight, 1);  // 1 decimal place
  Serial.print("g");
  
  // Print pressure level for TouchDesigner
  Serial.print(" | Level:");
  Serial.print(pressureLevel);
  
  // Print pressure change indicator
  if (pressureChanged) {
    Serial.print(" [CHANGE]");
  }
  
  Serial.println();
}