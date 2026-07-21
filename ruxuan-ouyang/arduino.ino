#include <Servo.h>

Servo servoX;
Servo servoY;

int ledPin = 11;

// ===== LED状态 =====
enum LEDState {
  OFF,
  ON_HOLD,
  FADING
};

LEDState ledState = OFF;

unsigned long stateStartTime = 0;

const unsigned long ON_TIME = 3000;     // 亮3秒
const unsigned long FADE_TIME = 3000;   // 渐暗3秒

int ledBrightness = 0;

void setup()
{
    servoX.attach(9);
    servoY.attach(10);

    pinMode(ledPin, OUTPUT);

    Serial.begin(9600);
}

void loop()
{
    handleSerial();
    updateLED();
}

// =========================
// 接收串口指令
// =========================
void handleSerial()
{
    if (Serial.available())
    {
        char cmd = Serial.read();

        if (cmd == '1')
        {
            ledState = ON_HOLD;
            stateStartTime = millis();
            ledBrightness = 255;

            analogWrite(ledPin, ledBrightness); // ⭐ PWM点亮
        }

        if (cmd == '0')
        {
            ledState = OFF;
            ledBrightness = 0;
            analogWrite(ledPin, 0);
        }
    }
}

// =========================
// LED状态机（核心）
// =========================
void updateLED()
{
    if (ledState == ON_HOLD)
    {
        if (millis() - stateStartTime >= ON_TIME)
        {
            ledState = FADING;
            stateStartTime = millis();
        }
    }

    else if (ledState == FADING)
    {
        unsigned long t = millis() - stateStartTime;

        if (t >= FADE_TIME)
        {
            ledState = OFF;
            ledBrightness = 0;
            analogWrite(ledPin, 0);
            return;
        }

        float progress = 1.0 - (float)t / FADE_TIME;
        ledBrightness = (int)(255 * progress);

        analogWrite(ledPin, ledBrightness);
    }
}