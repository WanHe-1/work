#define SIGNAL_OUT 16
#define DETECT_IN  4

#define BUZZER_PIN 18
#define VIB_PIN    19

bool alarmState = false;

void setup() {
    Serial.begin(115200);

    pinMode(SIGNAL_OUT, OUTPUT);
    pinMode(DETECT_IN, INPUT_PULLDOWN);

    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(VIB_PIN, OUTPUT);

    digitalWrite(SIGNAL_OUT, HIGH);

    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(VIB_PIN, LOW);
}

void loop() {
    if (digitalRead(DETECT_IN) == LOW) {
        alarmState = true;
    }

    if (digitalRead(DETECT_IN) == HIGH) {
        alarmState = false;
    }

    digitalWrite(BUZZER_PIN, alarmState);
    digitalWrite(VIB_PIN, alarmState);

    delay(20);
}