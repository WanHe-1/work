#include <Wire.h>
#include <U8g2lib.h>

// 初始化U8g2对象
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

void setup(void) {
  u8g2.begin();
}

unsigned long previousMillis = 0;
const long interval = 1000;

void loop(void) {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Get the current time using millis()
    unsigned long currentTime = currentMillis / 1000; // Convert milliseconds to seconds
    int hours = (currentTime / 3600) % 24;
    int minutes = (currentTime / 60) % 60;
    int seconds = currentTime % 60;

    u8g2.clearBuffer(); // Clear the buffer before updating

    // Format time as HH:MM:SS
    char timeStr[9];
    sprintf(timeStr, "%02d:%02d:%02d", hours, minutes, seconds);

    // Set font and draw time
    u8g2.setFont(u8g2_font_ncenB14_tr); // Choose a suitable font
    u8g2.drawStr(0, 20, timeStr);

    u8g2.sendBuffer(); // Send the buffer to the display
  }
}
