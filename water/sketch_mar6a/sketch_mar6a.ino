#define SensorPin A2            // pH meter Analog output to Arduino Analog Input 2
#define Offset 21.677           // deviation compensate
#define LED 13
#define samplingInterval 20
#define printInterval 800
#define ArrayLenth  40    // times of collection
int pHArray[ArrayLenth];   // Store the average value of the sensor feedback
int pHArrayIndex = 0;  

float TU_value = 0.0;
float temp_data = 25.0;
float K_Value = 3347.19;

void setup(void)
{
  pinMode(LED, OUTPUT);  
  Serial.begin(9600);  
  Serial.println("pH meter experiment!");    // Test the serial monitor
}

void loop(void)
{
  static unsigned long samplingTime = millis();
  static unsigned long printTime = millis();
  static float pHValue, voltage;
  
  if(millis() - samplingTime > samplingInterval)
  {
      pHArray[pHArrayIndex++] = analogRead(SensorPin);
      if(pHArrayIndex == ArrayLenth) pHArrayIndex = 0;
      voltage = avergearray(pHArray, ArrayLenth) * 5.0 / 1024;
      pHValue = -5.8887 * voltage + Offset;
      if(pHValue <= 0.0) { pHValue = 0.0; }
      if(pHValue > 14.0) { pHValue = 14.0; }
      samplingTime = millis();
  }
  
  // Every 800 milliseconds, print pH value and TU value
  if(millis() - printTime > printInterval)
  {
    // Print pH value
    Serial.print("Voltage: ");
    Serial.print(voltage, 2);
    Serial.print("    pH value: ");
    Serial.println(pHValue, 2);
    
    // Calculate and print TU value (Turbidity Value)
    int sensorValue = analogRead(A0); // Read the input on analog pin 0:
    float TU = sensorValue * (5.0 / 1024.0); // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
    float TU_calibration = -0.0192 * (temp_data - 25) + TU;
    TU_value = -865.68 * TU_calibration + K_Value;
    TU_value = constrain(mapFloat(TU_value, 0, 3000, 0, 3), 0, 3);
    if (TU_value <= 0) { TU_value = 0; }
    if (TU_value >= 3000) { TU_value = 3000; }
    Serial.print("TU Value: ");
    Serial.print(TU_value,2);
    Serial.println(" NTU");
    
    // Toggle LED status
    digitalWrite(LED, digitalRead(LED) ^ 1);
    
    printTime = millis();
  }
}

double avergearray(int* arr, int number){
  int i;
  int max, min;
  double avg;
  long amount = 0;
  if(number <= 0){
    Serial.println("Error number for the array to averaging!/n");
    return 0;
  }
  if(number < 5){   // less than 5, calculated directly statistics
    for(i = 0; i < number; i++){
      amount += arr[i];
    }
    avg = amount / number;
    return avg;
  }else{
    if(arr[0] < arr[1]){
      min = arr[0]; max = arr[1];
    }
    else{
      min = arr[1]; max = arr[0];
    }
    for(i = 2; i < number; i++){
      if(arr[i] < min){
        amount += min;        // arr < min
        min = arr[i];
      }else {
        if(arr[i] > max){
          amount += max;    // arr > max
          max = arr[i];
        }else{
          amount += arr[i]; // min <= arr <= max
        }
      }//if
    }//for
    avg = (double)amount / (number - 2);
  }//if
  return avg;
}



