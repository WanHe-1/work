const int sensorPin = A0; // Analog pin connected to the turbidity sensor
const int motorPin = 9;   // PWM pin connected to the fogger control

float TU = 0.0;
float TU_value = 0.0;
float TU_calibration = 0.0;
float temp_data = 25.0;
float K_Value = 3347.19;

void setup() {
  Serial.begin(9600); // Baud rate: 9600
  pinMode(motorPin, OUTPUT);
}

void loop() {
  int sensorValue = analogRead(sensorPin); // Read the input on analog pin A0:
  float voltage = sensorValue * (5.0 / 1024.0); // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
  TU_calibration = -0.0192 * (temp_data - 25) + voltage;  
  TU_value = -865.68 * TU_calibration + K_Value;
  
  if (TU_value <= 0) { 
    TU_value = 0;
  }
  if (TU_value >= 3000) { 
    TU_value = 3000;
  }

  // Map TU_value to a PWM signal between 0 and 255
  int motorSpeed = map(TU_value, 0, 3000, 0, 255);

  analogWrite(motorPin, motorSpeed); // Set the motor speed

  Serial.print("TU Value:");
  Serial.print(TU_value); // Print out the value you read:
  Serial.println(" NTU");

  delay(500);
}



