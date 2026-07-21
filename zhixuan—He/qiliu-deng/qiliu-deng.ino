const int SENSOR_PIN = 4;
const int LED_PIN = 3; 

int lastState = HIGH; 
bool wasAirflowDetected = false;

int ledBrightness = 0; 
int targetBrightness = 0; 
unsigned long holdStartTime = 0; 
bool shouldHold = false; 

void setup() {
  delay(1000);
  Serial.begin(9600);
  Serial.println("=== Setup Start ===");
  
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  analogWrite(LED_PIN, 0); 
  Serial.println("System Ready!");
  Serial.println("=== Setup End ===");
}

void loop() {
  int sensorVal = digitalRead(SENSOR_PIN);

  if (sensorVal != lastState) {
    lastState = sensorVal;
    
    if (sensorVal == LOW) {
      Serial.println("Airflow Detected!");
      targetBrightness = 255;
      shouldHold = true;
      holdStartTime = 0;
    } else {
      Serial.println("No Airflow.");
      targetBrightness = 0;
      shouldHold = false; 
    }
  }
  if (shouldHold && ledBrightness >= 254 && targetBrightness == 255) { // 当亮度接近255时
    if (holdStartTime == 0) {
      holdStartTime = millis(); 
      Serial.println("LED Full Brightness - Holding for 1 second...");
    }

    if (millis() - holdStartTime >= 1500) {
      Serial.println("1 second hold ended. Starting to fade out...");
      shouldHold = false; 
      targetBrightness = 0; 
    } else {
      analogWrite(LED_PIN, 255);
      return; 
    }
  }
、
  if (ledBrightness < targetBrightness) {
    ledBrightness += 5; 
    if (ledBrightness > targetBrightness) ledBrightness = targetBrightness;
  } else if (ledBrightness > targetBrightness) {
    ledBrightness -= 5; 
    if (ledBrightness < targetBrightness) ledBrightness = targetBrightness;
  }

  analogWrite(LED_PIN, ledBrightness);
  

  // if (ledBrightness % 50 == 0) {
  //   Serial.print("Current Brightness: ");
  //   Serial.println(ledBrightness);
  // }

  delay(20); 
}