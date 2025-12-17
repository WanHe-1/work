const int RELAY_PUMP_INFLATE = 2;  
const int RELAY_PUMP_DEFLATE = 3;

float ypr[3] = {0, 0, 0}; // Yaw, Pitch, Roll
unsigned char Re_buf[8], counter = 0;

float initialYaw = -108.0;     
float initialPitch = -52.0;  
float initialRoll = 152.0;    
const float ANGLE_TOLERANCE = 20.0; 
const float ANGLE_CHANGE_THRESHOLD = 90.0;  
const float MIN_ANGLE_CHANGE = 60.0;  

enum AngleState {
  ANGLE_UPWARD,   
  ANGLE_CHANGED  
};
AngleState angleState = ANGLE_UPWARD;

void setup() {
  pinMode(RELAY_PUMP_INFLATE, OUTPUT);
  pinMode(RELAY_PUMP_DEFLATE, OUTPUT);

  digitalWrite(RELAY_PUMP_INFLATE, HIGH);
  digitalWrite(RELAY_PUMP_DEFLATE, LOW);

  Serial.begin(115200);
  delay(2000);
  Serial.write(0xA5);
  Serial.write(0x52); 
}

void loop() {
  readGY25Data();
  controlByAngle();
}
void readGY25Data() {
  while (Serial.available()) {
    Re_buf[counter] = (unsigned char)Serial.read();
    
    if (counter == 0 && Re_buf[0] != 0xAA) return;  
    
    counter++;
    
    if (counter == 8) {  
      counter = 0;  
      
      if (Re_buf[0] == 0xAA && Re_buf[7] == 0x55) {  
        ypr[0] = (Re_buf[1] << 8 | Re_buf[2]) / 100.0; 
        ypr[1] = (Re_buf[3] << 8 | Re_buf[4]) / 100.0;
        ypr[2] = (Re_buf[5] << 8 | Re_buf[6]) / 100.0;
        Serial.print("Pitch: ");
        Serial.println(ypr[1]);
        delay(100);
      }
    }
  }
}
void controlByAngle() {
  float yawDiff = abs(ypr[0] - initialYaw);
  float pitchDiff = abs(ypr[1] - initialPitch);
  float rollDiff = abs(ypr[2] - initialRoll);

  if (yawDiff > 180) yawDiff = 360 - yawDiff;
  if (pitchDiff > 180) pitchDiff = 360 - pitchDiff;
  if (rollDiff > 180) rollDiff = 360 - rollDiff;

  float totalChange = pitchDiff;
  
  Serial.print("角度差: ");
  Serial.print(totalChange);

  if (totalChange < ANGLE_TOLERANCE && angleState != ANGLE_UPWARD) {
    digitalWrite(RELAY_PUMP_DEFLATE, LOW);   
    digitalWrite(RELAY_PUMP_INFLATE, HIGH); 
    angleState = ANGLE_UPWARD;
    Serial.println("充气");
  }
  else if (totalChange >= MIN_ANGLE_CHANGE && angleState != ANGLE_CHANGED) {
    digitalWrite(RELAY_PUMP_INFLATE, LOW); 
    digitalWrite(RELAY_PUMP_DEFLATE, HIGH);  
    angleState = ANGLE_CHANGED;
    Serial.print("放气");
    Serial.print(totalChange);
    Serial.println("°)");
  }
}
