#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include "U8glib.h"

// U8glib Setup
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);
const char *text = "Mountain stream";  // 要显示的文本
int textWidth;  // 文字宽度

// DFPlayer Setup
int analogPin = A0;     // Analog pin for reading infrared signal (0-1023)
int val = 0;            // Variable to store the infrared signal value
SoftwareSerial mySoftwareSerial(10, 11); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

void draw(void) {
  // graphic commands to redraw the complete screen should be placed here  
  u8g.setFont(u8g_font_unifont);  // 设置字体
  textWidth = u8g.getStrWidth(text);  // 获取文本宽度

  // 计算文本起始位置以实现居中显示，并向右偏移5像素
  int startX = (u8g.getWidth() - textWidth) / 2 + 3;

  u8g.drawStr(startX, 30, text);  // 绘制文本
}

void setup() {
  Serial.begin(9600);
  mySoftwareSerial.begin(9600); // Initialize software serial for communication with DFPlayer

  // Initialize DFPlayer Mini
  if (!myDFPlayer.begin(mySoftwareSerial)) {
    Serial.println("DFPlayer Mini initialization failed!");
    while (true); // Stop here if initialization fails
  }
  Serial.println("DFPlayer Mini initialized successfully!");

  myDFPlayer.volume(25); // Set volume (0-30), adjust as needed

  pinMode(8, OUTPUT);  // 设置引脚8为输出模式
}

void loop() {
  // picture loop for U8glib
  u8g.firstPage();  
  do {
    draw();
  } while( u8g.nextPage() );

  val = analogRead(analogPin); // Read the infrared signal (0-1023)
  Serial.print(val);

  if (val < 30) { // Threshold for detecting an infrared signal
    Serial.println("Infrared signal detected, playing music...");
    myDFPlayer.play(1); // Play the first song, ensure you have the corresponding audio file
    delay(10000); // Delay in milliseconds
  } 
  else {
    Serial.println("No signal detected");
    myDFPlayer.stop(); // stop the music if no signal
  }

  delay(1000); // Delay for 1 second before continuing detection
}
