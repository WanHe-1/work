#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "picture.h"

// TFT SPI 引脚
#define TFT_CS   7
#define TFT_DC   2
#define TFT_RST  1
#define TFT_MOSI 6
#define TFT_SCLK 4

// 使用软件 SPI，明确指定所有引脚
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

#define LCD_W 240
#define LCD_H 240

// 图片数据和尺寸
struct ImageInfo {
  const unsigned char* data;
  int16_t width;
  int16_t height;
};

const ImageInfo images[] = {
  { gImage_1, IMG1_W, IMG1_H },
  { gImage_2, IMG2_W, IMG2_H },
  { gImage_3, IMG3_W, IMG3_H },
  { gImage_4, IMG4_W, IMG4_H }
};
const uint8_t IMAGE_COUNT = 4;

// BGR565 转 RGB565
uint16_t bgrToRgb(uint16_t bgr) {
  uint16_t r = bgr & 0x001F;          // 低5位是B(原)→R(新)
  uint16_t g = bgr & 0x07E0;          // 中间6位是G，不变
  uint16_t b = (bgr >> 11) & 0x001F;  // 高5位是R(原)→B(新)
  return (r << 11) | g | b;
}

// 绘制图像 - Image2Lcd RGB565
void drawImage(int16_t x, int16_t y, const unsigned char* img, int16_t w, int16_t h) {
  tft.startWrite();
  tft.setAddrWindow(x, y, w, h);
  
  // 检测是否有 Image2Lcd 文件头 (第一个字节是 0x10)
  const unsigned char* data = img;
  if (pgm_read_byte(&img[0]) == 0x10 && pgm_read_byte(&img[1]) == 0x10) {
    data = img + 8;  // 跳过 8 字节头
  }
  
  for (int32_t i = 0; i < (int32_t)w * h; i++) {
    int32_t idx = i * 2;
    uint8_t hi = pgm_read_byte(&data[idx]);
    uint8_t lo = pgm_read_byte(&data[idx + 1]);
    uint16_t color = ((uint16_t)hi << 8) | lo;
    color = bgrToRgb(color);  // BGR 转 RGB
    tft.pushColor(color);
  }
  
  tft.endWrite();
}

void setup() {
  Serial.begin(115200);
  delay(200);

  // 软件 SPI 不需要手动 SPI.begin()
  tft.init(LCD_W, LCD_H, SPI_MODE3);  // 尝试 SPI_MODE3，部分屏幕需要
  tft.setRotation(0);

  tft.invertDisplay(true); 

  tft.fillScreen(ST77XX_BLACK);
}

void loop() {
  for (uint8_t i = 0; i < IMAGE_COUNT; i++) {
    int16_t w = images[i].width;
    int16_t h = images[i].height;
    int16_t x = (LCD_W - w) / 2;
    int16_t y = (LCD_H - h) / 2;
    
    tft.fillScreen(ST77XX_BLACK);  // 清屏
    drawImage(x, y, images[i].data, w, h);
    delay(5000);
  }
}
