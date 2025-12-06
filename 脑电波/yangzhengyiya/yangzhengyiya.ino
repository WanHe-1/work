const int trigPin = 10;
const int echoPin = 11;
const int ledPin1 = 12;
const int ledPin2 = 13;

long duration;
int distance;
int ledState1 = LOW;
int ledState2 = LOW;

void setup() {
  Serial.begin(9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  digitalWrite(ledPin1, ledState1);
  digitalWrite(ledPin2, ledState2);

}

void loop() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;
  Serial.print("当前距离: ");
  Serial.print(distance);
  Serial.println(" cm");
  
  if (distance < 80 && distance > 0) {
    if (ledState1 != HIGH || ledState2 != HIGH) {
      ledState1 = HIGH;x
      ledState2 = HIGH;
      digitalWrite(ledPin1, HIGH);
      digitalWrite(ledPin2, HIGH);
    }
  } else if (distance < 100 && distance >= 80) {
    if (ledState1 != HIGH || ledState2 != LOW) {
      ledState1 = HIGH;
      ledState2 = LOW;
      digitalWrite(ledPin1, HIGH);
      digitalWrite(ledPin2, LOW);

    }
  } else {
    if (ledState1 != LOW || ledState2 != LOW) {
      ledState1 = LOW;
      ledState2 = LOW;
      digitalWrite(ledPin1, LOW);
      digitalWrite(ledPin2, LOW);

    }
  }

  delay(200);
}



