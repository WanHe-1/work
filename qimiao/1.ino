
const int outputPin = A0;

unsigned long startTime = 0;
const unsigned long duration = 20000; // 20秒

bool isRunning = false;

void setup() {
  Serial.begin(9600);

  pinMode(outputPin, OUTPUT);
  digitalWrite(outputPin, LOW);
}

void loop() {

  // 接收串口数据
  if (Serial.available() > 0) {

    char data = Serial.read();

    if (data == '1') {
      Serial.println("Yellow Plate");

      digitalWrite(outputPin, HIGH);
      startTime = millis();
      isRunning = true;
    }

    else if (data == '2') {
      Serial.println("Green Plate");

      digitalWrite(outputPin, HIGH);
      startTime = millis();
      isRunning = true;
    }

    else if (data == '3') {
      Serial.println("Blue Plate");

      digitalWrite(outputPin, HIGH);
      startTime = millis();
      isRunning = true;
    }
  }

  // 判断20秒是否结束
  if (isRunning && millis() - startTime >= duration) {

    digitalWrite(outputPin, LOW);
    isRunning = false;

    Serial.println("Output LOW");
  }
}
