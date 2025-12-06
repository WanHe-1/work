#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// ========= SCL/SDA接口引脚配置 =========
constexpr int TFT_CS_PIN  = 10;  // CS 引脚
constexpr int TFT_DC_PIN  = 11;  // DC 引脚
constexpr int TFT_RST_PIN = 12;  // RES 引脚
constexpr int TFT_SCK_PIN = 13;  // SCL 引脚 (SPI Clock)
constexpr int TFT_MOSI_PIN = 14; // SDA 引脚 (SPI Data)
constexpr int TFT_MISO_PIN = -1; // ST7789不需要MISO

constexpr uint16_t SCREEN_WIDTH  = 240;
constexpr uint16_t SCREEN_HEIGHT = 240;

// 使用包含MOSI和SCK引脚的构造函数
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS_PIN, TFT_DC_PIN, TFT_MOSI_PIN, TFT_SCK_PIN, TFT_RST_PIN);

// 图片状态枚举
enum ImageState {
  IMAGE_1,
  IMAGE_2,
  IMAGE_3
};

// 全局变量
ImageState currentImage = IMAGE_1;
unsigned long lastSwitchTime = 0;
const unsigned long DISPLAY_DURATION = 5000; // 每张图片显示5秒

// 绘制第一张"图片" - 彩虹条纹
void drawImage1() {
  tft.fillScreen(ST77XX_BLACK);
  
  // 绘制彩虹条纹
  int barHeight = SCREEN_HEIGHT / 7;
  tft.fillRect(0, 0, SCREEN_WIDTH, barHeight, ST77XX_RED);
  tft.fillRect(0, barHeight, SCREEN_WIDTH, barHeight, ST77XX_ORANGE);
  tft.fillRect(0, 2*barHeight, SCREEN_WIDTH, barHeight, ST77XX_YELLOW);
  tft.fillRect(0, 3*barHeight, SCREEN_WIDTH, barHeight, ST77XX_GREEN);
  tft.fillRect(0, 4*barHeight, SCREEN_WIDTH, barHeight, ST77XX_BLUE);
  tft.fillRect(0, 5*barHeight, SCREEN_WIDTH, barHeight, ST77XX_MAGENTA);
  tft.fillRect(0, 6*barHeight, SCREEN_WIDTH, barHeight, ST77XX_CYAN);
  
  // 添加标题
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, SCREEN_HEIGHT/2 - 10);
  tft.println("Image 1");
}

// 绘制第二张"图片" - 几何图形
void drawImage2() {
  tft.fillScreen(ST77XX_BLACK);
  
  // 绘制中心圆形
  int centerX = SCREEN_WIDTH / 2;
  int centerY = SCREEN_HEIGHT / 2;
  tft.fillCircle(centerX, centerY, 80, ST77XX_BLUE);
  tft.drawCircle(centerX, centerY, 85, ST77XX_WHITE);
  
  // 绘制内部图案
  tft.fillCircle(centerX, centerY, 40, ST77XX_YELLOW);
  tft.drawLine(centerX - 60, centerY, centerX + 60, centerY, ST77XX_WHITE);
  tft.drawLine(centerX, centerY - 60, centerX, centerY + 60, ST77XX_WHITE);
  
  // 添加标题
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("Image 2");
}

// 绘制第三张"图片" - 渐变背景
void drawImage3() {
  tft.fillScreen(ST77XX_BLACK);
  
  // 绘制垂直渐变
  for(int i = 0; i < SCREEN_HEIGHT; i++) {
    uint16_t color = tft.color565(0, map(i, 0, SCREEN_HEIGHT, 0, 255), map(i, 0, SCREEN_HEIGHT, 255, 0));
    tft.drawFastHLine(0, i, SCREEN_WIDTH, color);
  }
  
  // 绘制中心文字
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(3);
  int textWidth = 8 * 7 * 3; // 估算文字宽度 (字符数 * 字符宽度 * 缩放)
  int x = (SCREEN_WIDTH - textWidth) / 2;
  tft.setCursor(x, SCREEN_HEIGHT/2 - 15);
  tft.println("IMAGE 3");
  
  // 添加边框
  tft.drawRect(5, 5, SCREEN_WIDTH - 10, SCREEN_HEIGHT - 10, ST77XX_WHITE);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing ST7789 display for image slideshow...");

  // 初始化屏幕
  tft.init(SCREEN_WIDTH, SCREEN_HEIGHT);
  
  // 设置屏幕旋转（根据实际需要调整）
  tft.setRotation(0);  // 0-3 可选，改变显示方向
  
  // 显示启动信息
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("Image");
  tft.setCursor(10, 35);
  tft.println("Slideshow");
  Serial.println("Display initialized successfully!");
  
  delay(2000);
  
  // 显示第一张图片
  drawImage1();
  lastSwitchTime = millis();
  Serial.println("Starting slideshow...");
}

void loop() {
  unsigned long currentTime = millis();
  
  // 检查是否需要切换图片
  if(currentTime - lastSwitchTime >= DISPLAY_DURATION) {
    // 切换到下一张图片
    switch(currentImage) {
      case IMAGE_1:
        drawImage2();
        currentImage = IMAGE_2;
        Serial.println("Showing Image 2");
        break;
      case IMAGE_2:
        drawImage3();
        currentImage = IMAGE_3;
        Serial.println("Showing Image 3");
        break;
      case IMAGE_3:
        drawImage1();
        currentImage = IMAGE_1;
        Serial.println("Showing Image 1");
        break;
    }
    lastSwitchTime = currentTime;
  }
}



