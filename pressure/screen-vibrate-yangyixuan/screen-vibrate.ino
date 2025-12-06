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
const int PRESSURE_THRESHOLD = 300; // 压力传感器阈值，可根据实际情况调整

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

  // 显示初始加载画面
  showLoadingScreen();
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

  // --- 震动模块控制逻辑 ---
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
  } else {
    // 进度未满100%，暂停震动5秒
    digitalWrite(VIBRATE_PIN, LOW);
    vibrationActive = false;
    vibrationStopTime = millis(); // 记录暂停开始时间
    Serial.println("Pressure detected. Vibration paused for 5 seconds.");
  }
}

void updateDisplay() {
  // 清空屏幕缓冲区
  display.clearDisplay();

  // 显示进度百分比文字
  display.setCursor(0, 0);
  display.print("Progress: ");
  display.print(progress);
  display.println("%");

  // 绘制进度条边框
  int barX = 0;
  int barY = 16;
  int barWidth = SCREEN_WIDTH - 2; // 留出边框空间
  int barHeight = 10;
  display.drawRect(barX, barY, barWidth, barHeight, SSD1306_WHITE);

  // 计算并绘制进度条填充部分
  int fillWidth = (progress * (barWidth - 2)) / 100; // 计算填充宽度，减去边框
  display.fillRect(barX + 1, barY + 1, fillWidth, barHeight - 2, SSD1306_WHITE);

  // 将缓冲区内容刷新到屏幕上
  display.display();
}

// 显示加载画面
void showLoadingScreen() {
  display.clearDisplay();
  display.setCursor(0, 15);
  display.println("Loading...");
  display.display();
  delay(2000); // 显示2秒加载画面
}



