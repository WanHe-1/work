
void setup() {
  Serial.begin(9600);
}

void loop() {

  if (Serial.available() > 0) {

    char data = Serial.read();

    if (data == '1') {
      Serial.println("Yellow Plate");
    }

    else if (data == '2') {
      Serial.println("Green Plate");
    }

    else if (data == '3') {
      Serial.println("Blue Plate");
    }
  }
}
