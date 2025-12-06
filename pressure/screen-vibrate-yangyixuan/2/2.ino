#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// 定义OLED屏幕尺寸
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
// 创建OLED对象，使用I2C接口，复位引脚设为-1（无复位引脚）
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// 定义引脚
#define PRESSURE_SENSOR_PIN A0    // 压力传感器连接到模拟引脚A0
#define VIBRATE_PIN 3   // 震动模块连接到数字引脚3

// 全局变量
int progress = 0; // 进度值，范围0-100
bool pressureDetected = false; // 压力检测标志
bool lastPressureState = false; // 上一次压力状态
unsigned long lastDebounceTime = 0; // 去抖动时间戳
const unsigned long DEBOUNCE_DELAY = 200; // 去抖动延时时间（毫秒），压力传感器响应较慢，延时可适当增加
bool vibrationActive = true; // 震动是否激活
unsigned long vibrationStopTime = 0; // 震动停止后开始计时的时间戳
const unsigned long VIBRATION_PAUSE_DURATION = 5000; // 震动暂停持续时间（5秒）
const int PRESSURE_THRESHOLD = 1; // 压力传感器阈值，可根据实际情况调整


// 用于缩放动画的变量
float scale = 1.0; // 当前缩放比例 (0.5 - 1.0)
float scaleDirection = -0.012; // 缩放变化方向和速度，进一步加快动画
unsigned long lastScaleUpdate = 0;
const unsigned long SCALE_UPDATE_INTERVAL = 3; // 控制缩放变化频率（毫秒），更快的更新

void setup() {
  // 初始化串口通信，用于调试
  Serial.begin(9600);

  // 初始化OLED屏幕
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // 0x3C是常见的I2C地址
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // 如果初始化失败，停止程序
  }

  // 清空屏幕
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // 设置压力传感器引脚为输入
  pinMode(PRESSURE_SENSOR_PIN, INPUT);
  // 设置震动模块引脚为输出
  pinMode(VIBRATE_PIN, OUTPUT);

  // 初始状态：震动开启
  digitalWrite(VIBRATE_PIN, HIGH);

  // 记录动画开始时间
  lastScaleUpdate = millis();

  // 进入初始动画循环
  showAnimatedLoadingScreen();
  
  // 显示初始进度画面
  updateDisplay();
}

void loop() {
  // 读取压力传感器当前值
  int sensorValue = analogRead(PRESSURE_SENSOR_PIN);
  bool currentPressureState = (sensorValue > PRESSURE_THRESHOLD);

  // --- 压力传感器去抖动逻辑 ---
  if (currentPressureState != lastPressureState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (currentPressureState != pressureDetected) {
      pressureDetected = currentPressureState;
      if (pressureDetected) { // 检测到压力（模拟按钮被按下）
        handlePressureDetected();
      }
    }
  }
  lastPressureState = currentPressureState;
  // --- 压力传感器去抖动逻辑结束 ---

  // --- 震动模块控制逻辑 
  // 检查是否处于暂停震动的状态
  if (!vibrationActive) {
    // 检查暂停时间是否已到
    if (millis() - vibrationStopTime >= VIBRATION_PAUSE_DURATION) {
      // 暂停时间到了，恢复震动
      if (progress < 100) { // 只有进度未满100%才恢复
        digitalWrite(VIBRATE_PIN, HIGH);
        vibrationActive = true;
        Serial.println("Vibration resumed after pause.");
      }
      // 如果进度已满100%，则vibrationActive保持false，震动不会恢复
    }
  }
  // --- 震动模块控制逻辑结束 ---

  // 更新缩放动画效果
  unsigned long currentTime = millis();
  if (currentTime - lastScaleUpdate > SCALE_UPDATE_INTERVAL) {
    // 更新缩放值
    scale += scaleDirection;
    
    // 控制缩放在0.5到1.0之间循环
    if (scale >= 1.0) {
      scale = 1.0;
      scaleDirection = -0.012; 
    } 
    else if (scale <= 0.5) {
      scale = 0.5;
      scaleDirection = 0.012; 
    }
    
    lastScaleUpdate = currentTime;
    // 总是更新显示以实现缩放动画效果
    updateDisplay();
  }

  // 为了防止主循环过快执行，可以加个小延时
  delay(10);
}

void handlePressureDetected() {
  // 每次检测到压力，进度增加20%
  progress += 20;
  if (progress > 100) {
    progress = 100; // 确保最大值为100%
  }

  // 更新OLED显示
  updateDisplay();

  // 控制震动模块
  if (progress == 100) {
    // 进度达到100%，永久停止震动
    digitalWrite(VIBRATE_PIN, LOW);
    vibrationActive = false;
    Serial.println("Progress reached 100%. Vibration stopped permanently.");
  } 
  else {
    // 进度未满100%，暂停震动5秒
    digitalWrite(VIBRATE_PIN, LOW);
    vibrationActive = false;
    vibrationStopTime = millis(); // 记录暂停开始时间
    Serial.println("Pressure detected. Vibration paused for 5 seconds.");
  }
}

void updateDisplay() {

  display.clearDisplay();

  drawCircles();

  display.display();
}

void drawCircles() {
  // 定义圆圈参数
  const uint8_t numCircles = 5;
  // 调整圆圈半径到10像素（直径20像素），为间距留出空间
  const uint8_t calculatedRadius = 10; // 半径为10像素，直径20像素
  const uint8_t centerY = SCREEN_HEIGHT / 2; // 圆圈垂直居中
  
  // 每个圆圈直径20像素，5个圆圈总共100像素，进一步增加间距让圆圈完全分开
  const uint8_t totalWidth = SCREEN_WIDTH - 8; // 减少边距，确保有足够空间
  const uint8_t circleDiameter = calculatedRadius * 2; // 圆圈直径
  const uint8_t totalCircleWidth = numCircles * circleDiameter; // 所有圆圈的总宽度
  const uint8_t availableSpacing = totalWidth - totalCircleWidth; // 可用间距
  const uint8_t spacing = availableSpacing / (numCircles - 1); // 每个间隙的宽度
  
  // 计算起始X坐标
  const uint8_t leftMargin = (SCREEN_WIDTH - totalWidth) / 2; // 左右边距
  const uint8_t startX = leftMargin;
  
  for (uint8_t i = 0; i < numCircles; i++) {
    // 计算每个圆圈的中心X坐标
    uint8_t x = startX + i * (circleDiameter + spacing) + calculatedRadius;
    
    uint8_t progressLevel = (progress / 20); // 计算当前点亮的圆圈数量

    // 判断当前圆圈是否应该显示为完成状态
    if (i < progressLevel) {
      // 完成的圆圈：绘制粗边框并添加对号
      drawThickCircle(x, centerY, calculatedRadius);
      drawCheckmark(x, centerY, calculatedRadius);
    } 
    // 未完成的圆圈：显示缩放动画
    else {
        // 根据当前缩放值绘制圆圈 - 实现缩放动画效果
        drawScalingCircle(x, centerY, calculatedRadius, scale);
    }
  }
}

void drawScalingCircle(uint8_t centerX, uint8_t centerY, uint8_t radius, float scale) {
    // 根据缩放值绘制圆圈，实现缓慢变小再缓慢变回原状的动画效果
    uint8_t scaledRadius = (uint8_t)(radius * scale);
    
    // 确保缩放后的半径不会太小
    if (scaledRadius < 2) {
        scaledRadius = 2;
    }
    
    // 绘制缩放后的圆圈
    display.drawCircle(centerX, centerY, scaledRadius, SSD1306_WHITE);
}

void drawThickCircle(uint8_t centerX, uint8_t centerY, uint8_t radius) {
    // 绘制细边框圆圈，为对号留出更多空间
    // 只绘制主圆和一层外圆，线条更细
    
    // 外层圆（稍微粗一点）
    if (radius + 1 < SCREEN_WIDTH/2 && radius + 1   < SCREEN_HEIGHT/2) {
        display.drawCircle(centerX, centerY, radius + 1, SSD1306_WHITE);
    }
    // 主圆
    display.drawCircle(centerX, centerY, radius, SSD1306_WHITE);
}

void drawCheckmark(uint8_t centerX, uint8_t centerY, uint8_t radius) {

    uint8_t checkSize = radius - 2; // 对号大小接近圆圈半径
    if (checkSize < 3) checkSize = 3;
    
    // 定义对号的三个关键点
    int leftX = centerX - checkSize/2;
    int leftY = centerY - 1;
    int centerBottomX = centerX - 1;
    int centerBottomY = centerY + checkSize/2;
    int rightX = centerX + checkSize/2;
    int rightY = centerY - checkSize/2;
    
    // 绘制对号√的左半部分：从左中到中心下方
    int steps1 = abs(centerBottomX - leftX);
    for (int i = 0; i <= steps1; i++) {
        int x = leftX + i;
        int y = leftY + (centerBottomY - leftY) * i / steps1;
        // 绘制粗线
        for (int j = -1; j <= 1; j++) {
            for (int k = -1; k <= 1; k++) {
                if (x+j >= 0 && x+j < SCREEN_WIDTH && y+k >= 0 && y+k < SCREEN_HEIGHT) {
                    display.drawPixel(x+j, y+k, SSD1306_WHITE);
                }
            }
        }
    }
    
    // 绘制对号√的右半部分：从中心下方到右上
    int steps2 = abs(rightX - centerBottomX);
    for (int i = 0; i <= steps2; i++) {
        int x = centerBottomX + i;
        int y = centerBottomY + (rightY - centerBottomY) * i / steps2;
        // 绘制粗线
        for (int j = -1; j <= 1; j++) {
            for (int k = -1; k <= 1; k++) {
                if (x+j >= 0 && x+j < SCREEN_WIDTH && y+k >= 0 && y+k < SCREEN_HEIGHT) {
                    display.drawPixel(x+j, y+k, SSD1306_WHITE);
                }
            }
        }
    }
}

void showAnimatedLoadingScreen() {
  // 持续显示动画直到有压力输入或达到一定时间
  while (progress == 0) {
    // 读取压力传感器当前值
    int sensorValue = analogRead(PRESSURE_SENSOR_PIN);
    bool currentPressureState = (sensorValue > PRESSURE_THRESHOLD);

    // 去抖动逻辑
    if (currentPressureState != lastPressureState) {
      lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
      if (currentPressureState != pressureDetected) {
        pressureDetected = currentPressureState;
        if (pressureDetected) { // 检测到压力，退出加载动画
          handleInitialPressure();
          break;
        }
      }
    }
    lastPressureState = currentPressureState;

    // 更新缩放动画效果
    unsigned long currentTime = millis();
    if (currentTime - lastScaleUpdate > SCALE_UPDATE_INTERVAL) {
      // 更新缩放值
      scale += scaleDirection;
      
      // 控制缩放在0.5到1.0之间循环
      if (scale >= 1.0) {
        scale = 1.0;
        scaleDirection = -0.012; // 开始缩小，进一步加快速度
      } else if (scale <= 0.5) {
        scale = 0.5;
        scaleDirection = 0.012; // 开始放大，进一步加快速度
      }
      
      lastScaleUpdate = currentTime;
      updateDisplay();
    }

    delay(10);
  }
}

void handleInitialPressure() {
  // 每次检测到压力，进度增加20%
  progress += 20;
  if (progress > 100) {
    progress = 100; // 确保最大值为100%
  }

  // 更新OLED显示
  updateDisplay();

  // 控制震动模块
  if (progress == 100) {
    // 进度达到100%，永久停止震动
    digitalWrite(VIBRATE_PIN, LOW);
    vibrationActive = false;
    Serial.println("Progress reached 100%. Vibration stopped permanently.");
  } else {
    // 进度未满100%，暂停震动5秒
    digitalWrite(VIBRATE_PIN, LOW);
    vibrationActive = false;
    vibrationStopTime = millis(); // 记录暂停开始时间
    Serial.println("Pressure detected. Vibration paused for 5 seconds.");
  }
}



