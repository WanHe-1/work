#define AD1 A1

void setup() {
  Serial.begin(9600);
}

void loop() {
  int Intensity = analogRead(AD1);
  Serial.println(Intensity);
  delay(500);
}