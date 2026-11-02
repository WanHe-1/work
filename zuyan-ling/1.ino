
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED 参数：0.91 寸常见规格为 128x32
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1

// I2C 地址通常为 0x3C
Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

// GSR 传感器模拟输入
const int GSR_PIN = A0;

int sensorValue = 0;

void setup() {
  Serial.begin(9600);

  // 初始化 I2C：UNO SDA=A4，SCL=A5
  Wire.begin();

  // 初始化 OLED
  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        0x3C
      )) {
    while (true) {
      // OLED 初始化失败
    }
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 8);
  display.print("0");
  display.display();
}

void loop() {
  // 读取 GSR 原始模拟值：0~1023
  sensorValue = analogRead(GSR_PIN);

  // 串口输出纯数字
    0erial.println(sensorValue);

  // OLED 仅显示数字
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);

  // 根据数字位数居中显示
  int16_t x1, y1;
  uint16_t textWidth, textHeight;

  char valueText[5];
  snprintf(valueText, sizeof(valueText),
           "%d", sensorValue);

  display.getTextBounds(
    valueText, 0, 0,
    &x1, &y1, &textWidth, &textHeight
  );

  int x = (SCREEN_WIDTH - textWidth) / 2;
  int y = (SCREEN_HEIGHT - textHeight) / 2;

  display.setCursor(x, y);
  display.print(valueText);
  display.display();

  // 每 500 毫秒更新一次
  delay(500);
}
