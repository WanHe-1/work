#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 12
#define PWM_PIN 11

#define TEMP_MIN 65.0
#define TEMP_MAX 72.0

#define FADE_TIME 500

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

int currentPWM = 0;
int targetPWM = 0;

void setup() {
  Serial.begin(115200);

  sensors.begin();

  pinMode(PWM_PIN, OUTPUT);
  analogWrite(PWM_PIN, 0);
}

void loop() {

  sensors.requestTemperatures();

  float temperature = sensors.getTempCByIndex(0);

  // 串口绘图仪只输出温度
  Serial.println(temperature);

  // 温度在65~72℃之间
  if (temperature >= TEMP_MIN && temperature <= TEMP_MAX) {
    targetPWM = 255;
  }
  else {
    targetPWM = 0;
  }

  fadeToTarget();

  delay(100);
}

void fadeToTarget() {

  if (currentPWM == targetPWM) {
    return;
  }

  int startPWM = currentPWM;
  unsigned long startTime = millis();

  while (currentPWM != targetPWM) {

    unsigned long elapsed = millis() - startTime;

    if (elapsed >= FADE_TIME) {
      currentPWM = targetPWM;
    }
    else {
      float progress = (float)elapsed / FADE_TIME;

      currentPWM = startPWM +(targetPWM - startPWM) * progress;
    }

    analogWrite(PWM_PIN, currentPWM);

    // 同时继续输出温度，让串口绘图仪持续更新
    sensors.requestTemperatures();
    float tempNow = sensors.getTempCByIndex(0);
    Serial.println(tempNow);

    delay(5);
  }
}
