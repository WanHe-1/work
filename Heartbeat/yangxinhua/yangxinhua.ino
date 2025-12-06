/*
  MAX30105 心率 + 震动传感器 双通道串口绘图仪
  功能: 同时监测心率(IR)和震动传感器数据，输出到串口绘图器进行可视化分析。

  操作步骤:
  1. 上传此代码到 Arduino 开发板。
  2. 按照下方硬件连接说明连接传感器。
  3. 打开 Arduino IDE -> 工具 -> 串口监视器。
  4. 在串口监视器中设置波特率为 115200。
  5. 点击串口监视器中的放大镜图标 (或 工具 -> 串口绘图器)。
  6. 将心率传感器用胶带或橡皮筋固定在手指上。
  7. 在串口绘图器中观察双通道波形：通道1=心率IR，通道2=震动传感器。

  硬件连接:
  MAX30105 Breakout -> Arduino:
  - VIN  = 5V   (或 3.3V)
  - GND  = GND
  - SDA  = A4   (Uno/Nano), 21 (ESP32)
  - SCL  = A5   (Uno/Nano), 22 (ESP32)
  - (INT) = 未连接 (此示例不使用中断)

  震动传感器 -> Arduino:
  - VCC = 5V (或 3.3V，按模块要求)
  - GND = GND
  - OUT = A0

  依赖库:
  - SparkFun MAX3010x Sensor Library
    安装方法: Arduino IDE -> 工具 -> 管理库... -> 搜索 "SparkFun MAX3010x"
*/

#include <Wire.h>
#include <MAX30105.h> // 确保已安装 SparkFun MAX3010x 库

MAX30105 particleSensor;

// ========== 震动传感器配置 ==========
const int VIBRATION_SENSOR_PIN = A0;  // 震动传感器模拟输入引脚
const int SMOOTHING_SAMPLES = 10;     // 平滑采样数量，数值越大平滑效果越明显

// ========== 震动传感器平滑处理变量 ==========
int vibrationReadings[SMOOTHING_SAMPLES];  // 存储最近N次震动传感器读数
int vibrationIndex = 0;                    // 当前读数索引位置
long vibrationTotal = 0;                   // 所有读数的总和
int vibrationAverage = 0;                  // 平滑后的平均值

// ========== 输出控制 ==========
const unsigned long OUTPUT_INTERVAL_MS = 50;  // 输出间隔(毫秒)，控制数据更新频率
unsigned long lastOutputTime = 0;             // 上次输出时间

// ========== 数据归一化参数 ==========
const int NORMALIZED_MAX = 1000;              // 归一化最大值
const int NORMALIZED_MIN = 0;                 // 归一化最小值

// 心率数据归一化范围（动态调整）
long heartRateMin = 2147483647L;              // 心率最小值
long heartRateMax = -2147483647L;             // 心率最大值

// 震动数据归一化范围（动态调整）
int vibrationMin = 1023;                      // 震动最小值
int vibrationMax = 0;                         // 震动最大值

// 归一化调整参数
const float RANGE_EXPAND_FACTOR = 1.2f;       // 量程扩展因子，避免数据贴边
const int MIN_RANGE = 100;                    // 最小量程，防止除零 

void setup() {
  // 初始化串口通信
  Serial.begin(115200);
  Serial.println("==========================================");
  Serial.println("MAX30105 心率 + 震动传感器 双通道监测器");
  Serial.println("==========================================");
  
  // 初始化震动传感器平滑数组
  Serial.println("初始化震动传感器平滑处理...");
  for (int i = 0; i < SMOOTHING_SAMPLES; i++) {
    vibrationReadings[i] = 0;
  }

  // 初始化 MAX30105 心率传感器
  Serial.println("正在初始化 MAX30105 心率传感器...");
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    while (1); // 停止程序执行
  }
  Serial.println("✅ MAX30105 传感器初始化成功");

  // 配置心率传感器参数
  Serial.println("配置心率传感器参数...");
  byte ledBrightness = 0x1F; // LED 亮度 (0-255)
  byte sampleAverage = 32;    // 采样平均 (1, 2, 4, 8, 16, 32)
  byte ledMode = 2;          // LED 模式 (2 = Red + IR)
  int sampleRate = 400;      // 采样率 (Hz)
  int pulseWidth = 411;      // 脉冲宽度 (us)
  int adcRange = 4096;       // ADC 量程

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
  Serial.println("✅ 心率传感器配置完成");

  // 传感器预热和信号稳定
  Serial.println("传感器预热中，请稍候...");
  delay(1000);

  // 读取初始值以稳定信号
  for (int i = 0; i < 30; i++) {
    particleSensor.getIR();
    delay(50);
  }

}

void loop() {
  // 检查是否到了输出时间
  unsigned long currentTime = millis();
  if (currentTime - lastOutputTime < OUTPUT_INTERVAL_MS) {
    return; // 还没到输出时间，直接返回
  }
  lastOutputTime = currentTime;

  // ========== 读取心率传感器数据 ==========
  long heartRateIR = particleSensor.getIR();
  
  // ========== 读取并平滑震动传感器数据 ==========
  // 移除最旧的读数
  vibrationTotal = vibrationTotal - vibrationReadings[vibrationIndex];
  
  // 读取新的震动传感器值
  int rawVibrationValue = analogRead(VIBRATION_SENSOR_PIN);
  
  // 将新值存入循环数组
  vibrationReadings[vibrationIndex] = rawVibrationValue;
  
  // 将新值累加到总和
  vibrationTotal = vibrationTotal + vibrationReadings[vibrationIndex];
  
  // 更新数组索引（循环数组）
  vibrationIndex = (vibrationIndex + 1) % SMOOTHING_SAMPLES;
  
  // 计算平滑后的平均值
  vibrationAverage = vibrationTotal / SMOOTHING_SAMPLES;

  // ========== 动态更新数据范围 ==========
  // 更新心率数据范围
  if (heartRateIR < heartRateMin) heartRateMin = heartRateIR;
  if (heartRateIR > heartRateMax) heartRateMax = heartRateIR;
  
  // 更新震动数据范围
  if (vibrationAverage < vibrationMin) vibrationMin = vibrationAverage;
  if (vibrationAverage > vibrationMax) vibrationMax = vibrationAverage;

  // ========== 数据归一化 ==========
  // 心率数据归一化到 0-1000
  int normalizedHeartRate = normalizeValue(heartRateIR, heartRateMin, heartRateMax);
  
  // 震动数据归一化到 0-1000
  int normalizedVibration = normalizeValue(vibrationAverage, vibrationMin, vibrationMax);

  // ========== 输出数据到串口绘图器 ==========
  // 格式: 心率IR值(归一化),震动传感器值(归一化)
  Serial.print(normalizedHeartRate);
  Serial.print(",");
  Serial.println(normalizedVibration);
}

// ========== 归一化函数 ==========
int normalizeValue(long value, long minVal, long maxVal) {
  // 确保量程有效
  if (maxVal <= minVal) {
    return NORMALIZED_MIN; // 如果量程无效，返回最小值
  }
  
  // 计算扩展后的量程（避免数据贴边）
  long range = maxVal - minVal;
  long expandedRange = (long)(range * RANGE_EXPAND_FACTOR);
  
  // 确保最小量程
  if (expandedRange < MIN_RANGE) {
    expandedRange = MIN_RANGE;
  }
  
  // 计算归一化值
  long normalized = ((value - minVal) * (NORMALIZED_MAX - NORMALIZED_MIN)) / expandedRange + NORMALIZED_MIN;
  
  // 限制在有效范围内
  if (normalized < NORMALIZED_MIN) normalized = NORMALIZED_MIN;
  if (normalized > NORMALIZED_MAX) normalized = NORMALIZED_MAX;
  
  return (int)normalized;
}