#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <pgmspace.h>

#include "picture.h"

// 根据你的实际接线修改这些引脚
constexpr uint8_t PIN_LCD_CS  = 7;
constexpr uint8_t PIN_LCD_DC  = 2;
constexpr uint8_t PIN_LCD_RES = 1;
constexpr uint8_t PIN_LCD_SCL = 4;  // ESP32-C3 HSPI SCK
constexpr uint8_t PIN_LCD_SDA = 6;  // ESP32-C3 HSPI MOSI
constexpr uint8_t PIN_LCD_BLK = 10; // 背光，接 3.3V 或经 IO 控制

constexpr uint16_t PANEL_WIDTH  = 240;
constexpr uint16_t PANEL_HEIGHT = 240;
constexpr uint16_t IMAGE_ORIGIN_X = (PANEL_WIDTH - IMAGE_WIDTH) / 2;
constexpr uint16_t IMAGE_ORIGIN_Y = (PANEL_HEIGHT - IMAGE_HEIGHT) / 2;
constexpr uint32_t FRAME_INTERVAL_MS = 5000;

Adafruit_ST7789 tft = Adafruit_ST7789(PIN_LCD_CS, PIN_LCD_DC, PIN_LCD_RES);

uint8_t currentFrame = 0;
uint32_t previousTick = 0;

const uint16_t* getFramePtr(uint8_t index) {
  index %= kImageFrameCount;
  return reinterpret_cast<const uint16_t*>(pgm_read_ptr(&kImageFrames[index]));
}

void pushFrame(const uint16_t* frame) {
  if (frame == nullptr) {
    return;
  }

  tft.startWrite();
  tft.setAddrWindow(IMAGE_ORIGIN_X, IMAGE_ORIGIN_Y, IMAGE_WIDTH, IMAGE_HEIGHT);
  for (uint32_t i = 0; i < IMAGE_PIXELS; ++i) {
    uint16_t color = pgm_read_word(frame + i);
    tft.pushColor(color);
  }
  tft.endWrite();
}

void hardwareReset() {
  pinMode(PIN_LCD_RES, OUTPUT);
  digitalWrite(PIN_LCD_RES, HIGH);
  delay(5);
  digitalWrite(PIN_LCD_RES, LOW);
  delay(20);
  digitalWrite(PIN_LCD_RES, HIGH);
  delay(120);
}

void setup() {
  pinMode(PIN_LCD_BLK, OUTPUT);
  digitalWrite(PIN_LCD_BLK, HIGH);

  hardwareReset();

  SPI.begin(PIN_LCD_SCL, -1, PIN_LCD_SDA, PIN_LCD_CS);

  tft.init(PANEL_WIDTH, PANEL_HEIGHT);
  tft.setSPISpeed(40000000);
  tft.setRotation(0);

  // 纯色测试，方便确认背光与 SPI 是否正常
  tft.fillScreen(ST77XX_RED);
  delay(300);
  tft.fillScreen(ST77XX_GREEN);
  delay(300);
  tft.fillScreen(ST77XX_BLUE);
  delay(300);
  tft.fillScreen(ST77XX_BLACK);

  pushFrame(getFramePtr(currentFrame));
  previousTick = millis();
  while (true);
}

void loop() {
  uint32_t now = millis();
  if (now - previousTick >= FRAME_INTERVAL_MS) {
    currentFrame = (currentFrame + 1) % kImageFrameCount;
    pushFrame(getFramePtr(currentFrame));
    previousTick = now;
  }
}
 
