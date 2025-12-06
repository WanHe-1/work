#include <Wire.h>
#include <U8g2lib.h>

// 初始化U8g2对象
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// 四个传感器连接的模拟引脚
const int sensorPins[4] = {A0, A1, A2, A3};

void setup() {
  Serial.begin(9600); // 启动串口通信

  // 初始化显示屏
  if (!u8g2.begin()) {
    Serial.println("Failed to initialize display");
    while (1); // 停止程序
  }

  u8g2.clearBuffer(); // 清空缓冲区
  u8g2.setFont(u8g2_font_ncenB08_tr); // 设置字体
  u8g2.drawStr(0, 10, "系统启动...");
  u8g2.sendBuffer(); // 发送缓冲区内容到显示屏
}

void loop() {
  long total = 0;

  // 读取四个传感器的值并求和
  for (int i = 0; i < 4; i++) {
    int sensorValue = analogRead(sensorPins[i]);
    total += sensorValue;
    delay(10); // 稳定读数
  }

  // 计算平均值
  float average = total / 4.0;

  // 打印到串口监视器
  Serial.print("平均压力值: ");
  Serial.println(average);

  // 显示到 OLED 屏幕
  u8g2.clearBuffer(); // 清空缓冲区
  u8g2.setFont(u8g2_font_ncenB14_tr); // 设置字体
  u8g2.drawStr(0, 30, "Hrv:");
  u8g2.setCursor(40, 30);
  u8g2.print(average);
  u8g2.sendBuffer(); // 发送缓冲区内容到显示屏

  delay(1000); // 每秒更新一次
}
