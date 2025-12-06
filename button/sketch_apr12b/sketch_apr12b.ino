const int buttonPin = 2; // 按钮连接到D2引脚
const int ledPin = 9;    // LED连接到D9引脚 (支持PWM)

int buttonState = 0;         // 当前按钮状态
int lastButtonState = 0;     // 上一次按钮状态
unsigned long lastDebounceTime = 0;  // 最后一次按键去抖动的时间
unsigned long debounceDelay = 50;    // 去抖动延迟时间

int brightness = 0;          // LED亮度值
int fadeAmount = 25;           // 每次按下的亮度变化量

void setup() {
  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);

  digitalWrite(buttonPin, HIGH); // 内部上拉电阻
}

void loop() {
  int reading = digitalRead(buttonPin); // 读取按钮状态

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == LOW) {
        brightness += fadeAmount;
        if (brightness > 255) {
          brightness = 255;
        }
      }
    }
  }

  lastButtonState = reading;

  analogWrite(ledPin, brightness);
}



