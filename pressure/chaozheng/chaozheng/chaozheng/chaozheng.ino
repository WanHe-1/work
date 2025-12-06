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
int baselineValues[numSensors];    // Baseline ADC values (no load)
float weightValues[numSensors];    // Weight values (grams)
float totalWeight = 0;             // Total weight (grams)
float previousWeightValues[numSensors]; // Previous weight values for change detection
bool pressureChanged = false;      // Flag to indicate pressure change

void setup() {
  Serial.begin(9600);
  
  // Calibrate baseline: read initial ADC values (no load)
  delay(1000);  // Wait for sensors to stabilize
  
  for (int i = 0; i < numSensors; i++) {
    int sum = 0;
    // Take multiple samples for accurate baseline
    for (int j = 0; j < 20; j++) {
      sum += analogRead(sensorPins[i]);
      delay(10);
    }
    baselineValues[i] = sum / 20;
    smoothedValues[i] = baselineValues[i];  // Initialize smoothed values
  }
  
  delay(500);
}

void loop() {
  readSensors();        // Read all sensors
  smoothData();         // Smooth the data
  convertToWeight();    // Convert to weight (grams)
  calculateTotalWeight(); // Calculate total weight
  detectPressureChanges(); // Detect pressure changes
  
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
    // Calculate ADC change from baseline (pressure causes ADC to decrease)
    int adcChange = baselineValues[i] - smoothedValues[i];
    
    float weight = 0;
    int threshold = 15; // Minimum ADC change to detect pressure (noise filter)
    
    if (adcChange > threshold) {
      // Map ADC change to weight
      // Assuming ~300 ADC change = 4500g (adjust based on your sensors)
      weight = map(adcChange, threshold, 300, 0, maxWeight);
      weight = constrain(weight, 0, maxWeight);
      weight *= 8;  // Apply calibration factor
    } else {
      // Below threshold → no pressure detected
      weight = 0;
    }
    
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

  Serial.println(totalWeight, 1);  // 1 decimal place
  
  Serial.println();
}