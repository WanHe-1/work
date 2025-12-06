#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include "U8glib.h"

// 创建屏幕对象，指定 I2C 地址为 0x3C
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);


const char *text = "The tide washing against the shore";

int analogPin = A0;     // Analog pin for reading infrared signal (0-1023)
int val = 0;            // Variable to store the infrared signal value

int textWidth;
int startY;
int scrollPosition = 5;

// Define software serial port for communication with DFPlayer Mini
SoftwareSerial mySoftwareSerial(10, 11); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

void setup() {
  Serial.begin(9600);
  mySoftwareSerial.begin(9600); // Initialize software serial for communication with DFPlayer

  // 设置字体
  u8g.setFont(u8g_font_7x14);
  // 计算文本宽度
  textWidth = u8g.getStrWidth(text)+5;
  
  // 设置Y坐标以居中显示
  startY = (u8g.getHeight() - u8g.getFontAscent()) / 2 + u8g.getFontAscent();
  // Initialize DFPlayer Mini
  if (!myDFPlayer.begin(mySoftwareSerial)) {
    Serial.println("DFPlayer Mini initialization failed!");
    while (true); // Stop here if initialization fails
  }
  Serial.println("DFPlayer Mini initialized successfully!");
  myDFPlayer.volume(30); // Set volume (0-30), adjust as needed
}
void loop() {
  val = analogRead(analogPin); // Read the infrared signal (0-1023)
  Serial.print("IR Value: ");
  Serial.println(val);
  u8g.firstPage();  
  do {
    draw();
  } while( u8g.nextPage() );
  scrollPosition -= 5;
  if (scrollPosition < -textWidth) {
    scrollPosition = u8g.getWidth();
  }
  if (val < 30) { // Threshold for detecting an infrared signal
    Serial.println("Infrared signal detected, playing music...");
    myDFPlayer.play(1); // Play the first song, ensure you have the corresponding audio file
   delay(10000);
  } 
  else {
    Serial.println("No signal detected");
    myDFPlayer.stop(); // Stop playback
  }
  delay(1000); // Delay for 1 second before continuing detection
}
void draw(void) {
  // Clear the screen
  u8g.setColorIndex(0); // Black background
  u8g.drawBox(0, 0, u8g.getWidth(), u8g.getHeight());
  // Set color index to white for text
  u8g.setColorIndex(1);
  // Draw the text
  u8g.drawStr(scrollPosition, startY, text);
}