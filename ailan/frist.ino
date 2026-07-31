#define PIN_TRIG       2
#define PIN_ECHO       3
#define PIN_MOTOR      4
#define PIN_TURBID     A0

bool motorRunning = false;
unsigned long lastDataTime = 0;
const unsigned long INTERVAL_MS = 300;

float cachedDistance = -1.0;
float cachedTurbidity = 0.0;

void readJSN() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  long duration = pulseIn(PIN_ECHO, HIGH, 30000);  // 超时30ms

  if (duration > 0) {
    cachedDistance = duration * 0.034 / 2.0;  // 单位 cm
  }
}

void readTurbidity() {
  int raw = analogRead(PIN_TURBID);
  float v = raw * (5.0 / 1023.0);

  float pct = (1.0 - (v / 5.0)) * 100;
  if (pct < 0)   pct = 0;
  if (pct > 100) pct = 100;
  cachedTurbidity = pct;
}

void setMotor(bool on) {
  motorRunning = on;
  digitalWrite(PIN_MOTOR, on ? HIGH : LOW);
}

void handleSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '1') {
      setMotor(true);
    } else if (c == '0') {
      setMotor(false);
    }
  }
}

void outputData() {
  Serial.print(cachedDistance, 1);
  Serial.print(",");
  Serial.print(cachedTurbidity, 1);
  Serial.print(",");
  Serial.println(motorRunning ? "1" : "0");
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_MOTOR, OUTPUT);
  digitalWrite(PIN_MOTOR, LOW);
}

void loop() {
  readJSN();

  handleSerial();

  unsigned long now = millis();
  if (now - lastDataTime >= INTERVAL_MS) {
    lastDataTime = now;
    readTurbidity();
    outputData();
  }
}