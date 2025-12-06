// 禁用看门狗配置（必须在最前面）
#include "sdkconfig.h"

// ESP32-S3 USB CDC 串口支持
#if ARDUINO_USB_CDC_ON_BOOT
#define HWSerial Serial0
#define USBSerial Serial
#else
#define HWSerial Serial
#endif

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
// 如果需要使用 MPU6050，请先安装库：工具 -> 管理库 -> 搜索 "Adafruit MPU6050"
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "esp_task_wdt.h"
#include "picture.h"

// TFT SPI 引脚
#define TFT_CS   7
#define TFT_DC   2
#define TFT_RST  1
#define TFT_MOSI 6
#define TFT_SCLK 4

// ESP32-S3 I2C 引脚定义（MPU6050）
#define I2C_SDA  8   // 根据你的实际连接修改
#define I2C_SCL  9   // 根据你的实际连接修改

// MPU6050 陀螺仪（需要先安装库）
Adafruit_MPU6050 mpu;

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

// 绘制图像 - Image2Lcd RGB565（逐行绘制，避免看门狗超时，180度旋转）
void drawImage(int16_t x, int16_t y, const unsigned char* img, int16_t w, int16_t h) {
  // 检测是否有 Image2Lcd 文件头 (第一个字节是 0x10)
  const unsigned char* data = img;
  if (pgm_read_byte(&img[0]) == 0x10 && pgm_read_byte(&img[1]) == 0x10) {
    data = img + 8;  // 跳过 8 字节头
  }
  
  // 逐行绘制，每行后让出CPU（180度旋转：从底部到顶部，从右到左）
  for (int16_t row = 0; row < h; row++) {
    tft.startWrite();
    tft.setAddrWindow(x, y + row, w, 1);
    
    // 从图像的底部行开始读取（180度旋转）
    int32_t srcRow = h - 1 - row;
    int32_t startPixel = (int32_t)srcRow * w;
    
    // 从右到左读取像素（180度旋转）
    for (int16_t col = w - 1; col >= 0; col--) {
      int32_t i = startPixel + col;
      int32_t idx = i * 2;
      uint8_t hi = pgm_read_byte(&data[idx]);
      uint8_t lo = pgm_read_byte(&data[idx + 1]);
      uint16_t color = ((uint16_t)hi << 8) | lo;
      color = bgrToRgb(color);  // BGR 转 RGB
      tft.pushColor(color);
    }
    
    tft.endWrite();
    
    // 每10行延迟一次，减少开销
    if (row % 10 == 0) {
      yield();  // 让出CPU
    }
  }
}

// 全局标志
bool mpu_available = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // ESP32-S3 禁用看门狗的正确方法
  esp_task_wdt_deinit();  // 先反初始化
  
  // 禁用硬件看门狗
  disableCore0WDT();
  disableCore1WDT();
  
  Serial.println("\n\n\n=== 系统启动 (ESP32-S3) ===");
  Serial.println("串口初始化完成");
  Serial.flush();

  // 初始化 I2C
  Serial.print("初始化 I2C (SDA=");
  Serial.print(I2C_SDA);
  Serial.print(", SCL=");
  Serial.print(I2C_SCL);
  Serial.println(")...");
  Serial.flush();
  
  bool i2c_ok = Wire.begin(I2C_SDA, I2C_SCL);
  if (!i2c_ok) {
    Serial.println("警告: I2C 初始化可能失败");
  }
  delay(100);
  
  // 初始化 MPU6050
  Serial.println("正在初始化 MPU6050...");
  Serial.flush();
  
  bool mpu_ok = false;
  for (int retry = 0; retry < 3; retry++) {
    if (mpu.begin(0x68, &Wire)) {
      mpu_ok = true;
      break;
    }
    Serial.print("尝试 ");
    Serial.print(retry + 1);
    Serial.println("/3...");
    delay(500);
  }
  
  if (!mpu_ok) {
    Serial.println("警告: MPU6050 初始化失败!");
    Serial.println("程序将继续运行，但不会读取陀螺仪数据");
    Serial.println("请检查:");
    Serial.print("1. SDA->");
    Serial.print(I2C_SDA);
    Serial.print(", SCL->");
    Serial.println(I2C_SCL);
    Serial.println("2. VCC -> 3.3V, GND -> GND");
    
    // 扫描 I2C 设备
    Serial.println("\n扫描 I2C 设备...");
    byte error, address;
    int nDevices = 0;
    for(address = 1; address < 127; address++ ) {
      Wire.beginTransmission(address);
      error = Wire.endTransmission();
      if (error == 0) {
        Serial.print("找到设备: 0x");
        if (address < 16) Serial.print("0");
        Serial.println(address, HEX);
        nDevices++;
      }
    }
    if (nDevices == 0) {
      Serial.println("未找到任何 I2C 设备!");
    }
    Serial.println();
  } else {
    Serial.println("✓ MPU6050 初始化成功");
  
    // 配置 MPU6050
    Serial.println("配置 MPU6050 参数...");
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    Serial.println("✓ MPU6050 配置完成");
  }

  // 软件 SPI 不需要手动 SPI.begin()
  Serial.println("初始化 TFT 屏幕...");
  Serial.flush();
  
  try {
    tft.init(LCD_W, LCD_H, SPI_MODE3);
    tft.setRotation(0);
    tft.invertDisplay(true); 
    tft.fillScreen(ST77XX_BLACK);
    Serial.println("✓ TFT 屏幕初始化完成");
  } catch (...) {
    Serial.println("警告: TFT 初始化可能失败");
  }
  
  // 检查 MPU6050 是否可用
  sensors_event_t a, g, temp;
  mpu_available = mpu.getEvent(&a, &g, &temp);
  
  Serial.println("\n=== 系统就绪，开始运行 ===\n");
  Serial.flush();
}

// 读取并输出陀螺仪数据到串口
void printGyroData() {
  if (!mpu_available) {
    Serial.println("MPU6050 不可用");
    return;
  }
  
  sensors_event_t a, g, temp;
  if (!mpu.getEvent(&a, &g, &temp)) {
    Serial.println("读取 MPU6050 失败");
    return;
  }
  
  // 显示陀螺仪 XYZ 轴数据（单位：rad/s）
  Serial.print("陀螺仪 X: "); Serial.print(g.gyro.x);
  Serial.print(" Y: "); Serial.print(g.gyro.y);
  Serial.print(" Z: "); Serial.println(g.gyro.z);
  
  // 显示加速度计数据（可选）
  Serial.print("加速度 X: "); Serial.print(a.acceleration.x);
  Serial.print(" Y: "); Serial.print(a.acceleration.y);
  Serial.print(" Z: "); Serial.println(a.acceleration.z);
  
  // 显示温度（可选）
  Serial.print("温度: "); Serial.print(temp.temperature);
  Serial.println(" °C");
  Serial.println("---");
}

uint8_t currentImage = 0;
unsigned long lastImageChange = 0;
unsigned long lastGyroRead = 0;
const unsigned long IMAGE_INTERVAL = 5000;  // 图片切换间隔（毫秒）
const unsigned long GYRO_INTERVAL = 100;     // 陀螺仪读取间隔（毫秒）

void loop() {
  unsigned long currentTime = millis();
  
  // 定时读取并输出陀螺仪数据
  if (currentTime - lastGyroRead >= GYRO_INTERVAL) {
    printGyroData();
    lastGyroRead = currentTime;
  }
  
  // 定时自动切换图片
  if (currentTime - lastImageChange >= IMAGE_INTERVAL) {
    currentImage = (currentImage + 1) % IMAGE_COUNT;  // 循环切换
    
    int16_t w = images[currentImage].width;
    int16_t h = images[currentImage].height;
    int16_t x = (LCD_W - w) / 2;
    int16_t y = (LCD_H - h) / 2;
    
    tft.fillScreen(ST77XX_BLACK);  // 清屏
    drawImage(x, y, images[currentImage].data, w, h);
    lastImageChange = currentTime;
  }
}
