const int fruitPins[] = {A0, A1, A2, A3, A4, A5, A6};
const int pinCount = 7;

const int changeThreshold = 200;  
int lastValues[7];

unsigned long lastTriggerTime[7] = {0};
const unsigned long debounceDelay = 1500;

const int numReadings = 5;
int readings[7][5];
int readIndex[7] = {0};
int total[7] = {0};

void setup() {
  Serial.begin(9600);

  for (int i = 0; i < pinCount; i++) {
    for (int j = 0; j < numReadings; j++) {
      readings[i][j] = 0;
    }
    lastValues[i] = analogRead(fruitPins[i]);
  }
  
  delay(1000);
}
void loop() {
  int maxChange = 0;
  int maxChangePin = -1;
  
  for (int i = 0; i < pinCount; i++) {
    total[i] = total[i] - readings[i][readIndex[i]];
    readings[i][readIndex[i]] = analogRead(fruitPins[i]);
    total[i] = total[i] + readings[i][readIndex[i]];
    readIndex[i] = (readIndex[i] + 1) % numReadings;
    
    int smoothValue = total[i] / numReadings;
    int change = abs(smoothValue - lastValues[i]);

    if (change > changeThreshold && change > maxChange) {
      if ((millis() - lastTriggerTime[i]) > debounceDelay) {
        maxChange = change;
        maxChangePin = i;
      }
    }
    
    lastValues[i] = smoothValue;
  }
  
  if (maxChangePin >= 0) {
    Serial.println(maxChangePin + 1); 
    lastTriggerTime[maxChangePin] = millis();
  }
  
  delay(20);
}