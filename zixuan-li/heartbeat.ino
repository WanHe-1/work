/*
  MAX30105 心率串口绘图仪 (简化版)
  功能: 配置 MAX30105 传感器并通过串口输出红外(IR)数据，用于Arduino串口绘图器显示。

  操作步骤:
  1. 上传此代码到 Arduino 开发板。
  2. 按照下方硬件连接说明连接 MAX30105 传感器。
  3. 打开 Arduino IDE -> 工具 -> 串口监视器。
  4. 在串口监视器中设置波特率为 115200。
  5. 点击串口监视器中的放大镜图标 (或 工具 -> 串口绘图器)。
  6. 将传感器用胶带或橡皮筋固定在手指上。
  7. 在串口绘图器中观察心跳波形。

  硬件连接 (MAX30105 Breakout -> Arduino):
  - VIN  = 5V   (或 3.3V)
  - GND  = GND
  - SDA  = A4   (Uno/Nano), 21 (ESP32)
  - SCL  = A5   (Uno/Nano), 22 (ESP32)
  - (INT) = 未连接 (此示例不使用中断)

  依赖库:
  - SparkFun MAX3010x Sensor Library
    安装方法: Arduino IDE -> 工具 -> 管理库... -> 搜索 "SparkFun MAX3010x"
*/

#include <Wire.h>
#include <MAX30105.h> // 确保已安装 SparkFun MAX3010x 库

MAX30105 particleSensor;

void setup() {
  // 初始化串口，波特率 115200 用于串口绘图器
  Serial.begin(115200);
  Serial.println("正在初始化 MAX30105 传感器...");

  // 初始化传感器
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("错误: 未找到 MAX30105 传感器。请检查连线和电源。");
    while (1); // 停止程序
  }
  Serial.println("MAX30105 传感器初始化成功");

  // --- 配置传感器 ---
  byte ledBrightness = 0x1F; // LED 亮度 (0-255)
  byte sampleAverage = 32;    // 采样平均 (1, 2, 4, 8, 16, 32)
  byte ledMode = 2;          // LED 模式 (2 = Red + IR)
  int sampleRate = 400;      // 采样率 (Hz) (50, 100, 200, 400, 800, 1000, 1600, 3200)
  int pulseWidth = 411;      // 脉冲宽度 (us) (69, 118, 215, 411)
  int adcRange = 4096;       // ADC 量程 (2048, 4096, 8192, 16384)

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
  Serial.println("传感器配置完成");

  // --- 预热和基线设置 ---
  Serial.println("传感器预热中...");
  delay(1000); // 短暂预热

  // 读取几个初始值以稳定信号
  for (int i = 0; i < 30; i++) {
    particleSensor.getIR();
    delay(50);
  }

  Serial.println("开始数据传输到串口绘图器...");
  Serial.println("---"); // 分隔符，标记数据开始
}

void loop() {
  // --- 选项 1: 发送原始 IR 数据 (推荐用于观察波形细节) ---
  long irValue = particleSensor.getIR();
  Serial.println(irValue);

  // --- 选项 2: 发送平滑/平均后的数据 (可减少噪声，但可能损失细节) ---
  // static const int avgSamples = 10;
  // static long irSum = 0;
  // static int sampleCount = 0;
  // static unsigned long lastPrintTime = 0;
  // const long printInterval = 1000; // 每1000ms (1秒) 打印一次平均值

  // irSum += particleSensor.getIR();
  // sampleCount++;

  // if (millis() - lastPrintTime >= printInterval) {
  //   if (sampleCount > 0) {
  //     long averageIR = irSum / sampleCount;
  //     Serial.println(averageIR);
  //   } else {
  //     Serial.println(0); // 防止除以零
  //   }
  //   // 重置累加器和计数器
  //   irSum = 0;
  //   sampleCount = 0;
  //   lastPrintTime = millis();
  // }

  // --- 控制采样率 ---
  // delay(10); // 可根据 sampleRate 调整，或移除以获取最大速度
  // 注意：MAX30105 内部的 sampleRate 设置会控制传感器本身的采样，
  // 这里的 delay 主要影响发送到串口的频率。
  // 如果 sampleRate=400Hz, delay(10) 大约也是 100Hz 的输出。
}



