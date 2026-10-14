
const int TRIG_PIN = 9;
const int ECHO_PIN = 10;

const int RELAY_PIN = 12;

const float TRIGGER_DISTANCE = 50.0;

const unsigned long OFF_DELAY = 1000;
unsigned long outOfRangeStart = 0;
bool waitingToTurnOff = false;
bool relayState = false;

void setup() {
  Serial.begin(9600);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(RELAY_PIN, OUTPUT);

  // 初始关闭继电器
  digitalWrite(RELAY_PIN, LOW);

}

void loop() {

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0) {
    Serial.print(0);
    Serial.print('\t');
    Serial.println(relayState ? 1 : 0);
    delay(50);
    return;
  }

  float distance = duration * 0.0343 / 2.0;

  if (distance < TRIGGER_DISTANCE) {

    waitingToTurnOff = false;
    if (!relayState) {
      relayState = true;
      digitalWrite(RELAY_PIN, HIGH);
    }
  }

  else {
    if (relayState) {

      if (!waitingToTurnOff) {

        waitingToTurnOff = true;
        outOfRangeStart = millis();
      }
      if (millis() - outOfRangeStart >= OFF_DELAY) {

        relayState = false;
        digitalWrite(RELAY_PIN, LOW);

        waitingToTurnOff = false;
      }
    }
  }
。
  Serial.print(distance, 1);
  Serial.print('\t');
  Serial.println(relayState ? 1 : 0);

  delay(50);
}