/***
 * 淘宝    ：https://qhebot.taobao.com/
***/
int sensorPin = A0;    // select the input pin for the potentiometer
float sensorValue = 0;  // variable to store the value coming from the sensor

void setup() {
  Serial.begin(115200);
  Serial.println("turbidity TEST......");
}

void loop() {
  sensorValue = median_filtering();
  Serial.print("turbidity value is ");
  Serial.print(sensorValue, 2); // Format to two decimal places
  Serial.println();
  delay(300);
}

#define median_filtering_length 3

float median_filtering(){
  int senseV[median_filtering_length];

  for(int i = 0; i < median_filtering_length; i++){
    senseV[i] = analogRead(sensorPin);
    delay(3);
  }

  for(int i = 0; i < median_filtering_length; i++ ){
    for(int k = i; k < median_filtering_length; k++ ){
      if(senseV[i] > senseV[k]){
          int tmp = senseV[i];
          senseV[i] = senseV[k];
          senseV[k] = tmp;
      }
    }  
  }
  float sv = (senseV[median_filtering_length/2]*100.0/1024.0); // Convert to 0-100 range
  sv = map(sv, 0, 100, 0, 300) / 100.0; // Map to 0-3 range with two decimal places
  return sv;
}