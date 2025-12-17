#define TRIG_PIN     11
#define ECHO_PIN     10
#define L298N_ENA    9
#define L298N_IN1    13
#define L298N_IN2    12

const int minDistance = 20;   // 建议从 20cm 开始（避开盲区）
const int maxDistance = 50;

int currentBrightness = 0;

int readDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);e:\code\study\foc study\foc

  long duration = pulseIn(ECHO_PIN, HIGH, 25000); // 25ms timeout

  if (duration == 0) {
    // 无回波 → 物体极近（< 3cm），视为 2cm
    return 2;
  }
  return (int)(duration * 0.0343 / 2);
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(L298N_ENA, OUTPUT);
  pinMode(L298N_IN1, OUTPUT);
  pinMode(L298N_IN2, OUTPUT);
  digitalWrite(L298N_IN1, HIGH);
  digitalWrite(L298N_IN2, LOW);
}

void loop() {
  int distance = (readDistance() + readDistance()) / 2;

  int targetBrightness;
  if (distance <= minDistance) {
    targetBrightness = 255;   // 很近 → 最亮
  } else if (distance >= maxDistance) {
    targetBrightness = 0;     // 很远 → 熄灭
  } else {
    targetBrightness = map(distance, minDistance, maxDistance, 255, 0);
  }

  // 快速平滑
  int diff = targetBrightness - currentBrightness;
  if (diff > 15) currentBrightness += 15;
  else if (diff < -15) currentBrightness -= 15;
  else currentBrightness = targetBrightness;

  analogWrite(L298N_ENA, currentBrightness);

  // 打印调试
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.print(" cm | Brightness: ");
  Serial.println(currentBrightness);

  delay(10);
}