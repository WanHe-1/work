
const int analogPins[] = {A0, A1, A2, A3, A4, A5};
const int touchPin = 3;  
const int totalPins = 7;  
const bool enableD3 = true; 

const int analogThreshold = 190;
const unsigned long debounceDelay = 1200;

int lastAnalogValues[6];
int lastTouchState = HIGH;  
int touchStateCounter = 0;   
const int touchStableThreshold = 10;  
unsigned long lastTriggerTime[7] = {0};

unsigned long globalLastTriggerTime = 0;
const unsigned long globalDebounceDelay = 1000;
const unsigned long d3DebounceDelay = 3500;  

const int numReadings = 5;
int readings[6][5];
int readIndex[6] = {0};
int total[6] = {0};

void setup() {
  Serial.begin(9600);

  pinMode(touchPin, INPUT);
  lastTouchState = digitalRead(touchPin);

  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < numReadings; j++) {
      readings[i][j] = 0;
    }
    lastAnalogValues[i] = analogRead(analogPins[i]);
  }
  
  delay(1000);  
}
void loop() {
  unsigned long currentTime = millis();

  if (enableD3) {
    int currentTouchState = digitalRead(touchPin);
    static unsigned long lastD3DebugTime = 0;
    if (currentTime - lastD3DebugTime > 1000) {
      lastD3DebugTime = currentTime;
    }
    if (currentTouchState == HIGH && lastTouchState == LOW) {
      touchStateCounter++;
      if (touchStateCounter >= touchStableThreshold) {
        if ((currentTime - lastTriggerTime[6]) > d3DebounceDelay) {
          Serial.println(7);
          globalLastTriggerTime = currentTime;
          lastTriggerTime[6] = currentTime;
          lastTouchState = HIGH;
          touchStateCounter = 0;
        }
      }
    } else if (currentTouchState == LOW && lastTouchState == HIGH) {
      touchStateCounter++;
      if (touchStateCounter >= touchStableThreshold) {
        lastTouchState = LOW;
        touchStateCounter = 0;
      }
    } else if (currentTouchState == lastTouchState) {
      touchStateCounter = 0; 
    }
  }
  for (int i = 0; i < 6; i++) {
    total[i] = total[i] - readings[i][readIndex[i]];
    readings[i][readIndex[i]] = analogRead(analogPins[i]);
    total[i] = total[i] + readings[i][readIndex[i]];
    readIndex[i] = (readIndex[i] + 1) % numReadings;
    int smoothValue = total[i] / numReadings;
    int change = abs(smoothValue - lastAnalogValues[i]);
    if (change > analogThreshold) {
      if ((currentTime - globalLastTriggerTime) > globalDebounceDelay) {
        Serial.println(i + 1);
        globalLastTriggerTime = currentTime;
        lastTriggerTime[i] = currentTime;
      }
    }
    
    lastAnalogValues[i] = smoothValue;
  }

  delay(20);
}