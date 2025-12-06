#include "bsp_uart.hpp"
#include <stdio.h>
#include <SoftwareSerial.h>

// 全局变量
SoftwareSerial dfplayerSerial(DFPLAYER_RX_PIN, DFPLAYER_TX_PIN);
// 串口激光传感器使用硬件串口(引脚0,1)
unsigned long lastAudioTime = 0;
int lastAudioLevel = 0;
unsigned long lastLaserRead = 0;

// 灯光平滑过渡相关变量
int currentBrightness = 0;
int targetBrightness = 0;
unsigned long lastBrightnessUpdate = 0;
const int BRIGHTNESS_STEP = 5;  // 每次亮度变化步长
const int BRIGHTNESS_UPDATE_INTERVAL = 50;  // 亮度更新间隔(ms)

void parseDistanceData(String &data) {
  // 去除首尾空白字符  Remove the first and last whitespace characters
  data.trim();
  
  // 查找逗号位置 Find the position of the comma
  int commaPos = data.indexOf(','); 
  if (commaPos == -1) {
    return;
  }
  
  // 提取距离  Extraction distance
  String distanceStr = data.substring(0, commaPos);
  distanceStr.trim();
  
  // 提取置信度  Extract the confidence level section
  String confidenceStr = data.substring(commaPos + 1);
  confidenceStr.trim();
  
  // 转换为整数 Convert to an integer 
  long distance = distanceStr.toInt();  
  long confidence = confidenceStr.toInt();
  
  if ((distance <=20 || distance>=4000) && distanceStr != "0") {
    return;
  }
  if (confidence == 0 && confidenceStr != "0") {
    return;
  }
  
  // 输出结果 output result
  Serial.print("Distance:");
  Serial.print(distance);
  Serial.print(" mm, Confidence: ");
  Serial.println(confidence);
  
  // 控制灯光和音频
  controlLights(distance);
  playAudio(distance);
}

// 初始化灯光控制
void initLightControl() {
  pinMode(LIGHT_ENA_PIN, OUTPUT);
  pinMode(LIGHT_ENB_PIN, OUTPUT);
  pinMode(LIGHT_IN1_PIN, OUTPUT);
  pinMode(LIGHT_IN2_PIN, OUTPUT);
  pinMode(LIGHT_IN3_PIN, OUTPUT);
  pinMode(LIGHT_IN4_PIN, OUTPUT);
  
  // 初始化所有引脚为低电平
  analogWrite(LIGHT_ENA_PIN, 0);
  analogWrite(LIGHT_ENB_PIN, 0);
  digitalWrite(LIGHT_IN1_PIN, LOW);
  digitalWrite(LIGHT_IN2_PIN, LOW);
  digitalWrite(LIGHT_IN3_PIN, LOW);
  digitalWrite(LIGHT_IN4_PIN, LOW);
  
  Serial.println("灯光初始化完成");
}

// 初始化音频控制
void initAudioControl() {
  dfplayerSerial.begin(9600);
  delay(1000);
  // 发送DFPlayer初始化命令
  byte initCmd[] = {0x7E, 0xFF, 0x06, 0x09, 0x00, 0x00, 0x02, 0xFE, 0xF7, 0xEF};
  for (int i = 0; i < 10; i++) {
    dfplayerSerial.write(initCmd[i]);
  }
  delay(1000);
  Serial.println("音频初始化完成");
}

// 尝试不同的波特率连接激光传感器
bool tryConnectLaser(long baudRate) {
  // 重新初始化硬件串口
  Serial.end();
  delay(100);
  Serial.begin(baudRate);
  delay(3000);  // 等待传感器响应
  
  // 检查是否有数据
  int dataCount = 0;
  for (int i = 0; i < 10; i++) {
    delay(500);
    if (Serial.available() > 0) {
      dataCount++;
      // 读取并显示数据
      String data = Serial.readStringUntil('\n');
      data.trim();
      if (data.length() > 0) {
        Serial.print("   收到数据: ");
        Serial.println(data);
        return true;
      }
    }
  }
  
  // 清空缓冲区
  while (Serial.available()) {
    Serial.read();
  }
  
  return false;
}


// 初始化串口激光传感器
void initLaserSensor() {
  Serial.println("=== 串口激光传感器检测 ===");
  Serial.println("接线: TX→0, RX→1, VCC→5V, GND→GND");
  
  // 尝试不同的波特率，115200优先
  long baudRates[] = {115200, 9600, 38400, 19200, 57600, 4800};
  int numBaudRates = sizeof(baudRates) / sizeof(baudRates[0]);
  
  bool connected = false;
  for (int i = 0; i < numBaudRates; i++) {
    Serial.print("尝试波特率: ");
    Serial.println(baudRates[i]);
    if (tryConnectLaser(baudRates[i])) {
      connected = true;
      Serial.print("✓ 连接成功! 波特率: ");
      Serial.println(baudRates[i]);
      break;
    }
  }
  
  if (!connected) {
    Serial.println("✗ 无法连接激光传感器");
    Serial.println("可能原因:");
    Serial.println("- 接线错误 (TX→0, RX→1)");
    Serial.println("- 传感器未供电");
    Serial.println("- 传感器故障");
    Serial.println("- 波特率不匹配");
    Serial.end();
    delay(100);
    Serial.begin(115200);
  }
  
  Serial.println("开始监听传感器数据...");
}

// 手动测试模式 - 模拟距离数据
void manualTestMode() {
  static unsigned long lastTestTime = 0;
  static int testDistance = 1000;  // 起始距离1000mm
  static bool increasing = false;
  
  if (millis() - lastTestTime > 2000) {  // 每2秒变化一次
    lastTestTime = millis();
    
    // 模拟距离变化：1000mm → 200mm → 1000mm
    if (increasing) {
      testDistance += 200;
      if (testDistance >= 1000) {
        testDistance = 1000;
        increasing = false;
      }
    } else {
      testDistance -= 200;
      if (testDistance <= 200) {
        testDistance = 200;
        increasing = true;
      }
    }
    
    Serial.print("测试距离: ");
    Serial.println(testDistance);
    
    // 直接控制灯光，避免创建字符串
    controlLights(testDistance);
  }
}

// 读取激光传感器数据
void readLaserData() {
  // 每500ms读取一次传感器数据
  if (millis() - lastLaserRead < 500) {
    return;
  }
  lastLaserRead = millis();
  
  // 检查激光传感器是否有数据可读（使用硬件串口）
  if (Serial.available() > 0) {
    String data = Serial.readStringUntil('\n');
    data.trim();
    
    if (data.length() > 0) {
      Serial.println("激光数据");
      
      // 解析激光数据并控制灯光和音频
      parseDistanceData(data);
    }
  }
  
  // 检查串口是否有手动输入的数据（用于测试）
  if (Serial.available() > 0) {
    String manualData = Serial.readStringUntil('\n');
    manualData.trim();
    
    if (manualData.length() > 0) {
      Serial.print("手动输入: ");
      Serial.println(manualData);
      
      // 解析手动输入的数据
      parseDistanceData(manualData);
    }
  }
  
  // 每5秒输出一次状态信息
  static unsigned long lastStatusTime = 0;
  if (millis() - lastStatusTime > 5000) {
    lastStatusTime = millis();
    Serial.println("运行中...");
    
    // 如果长时间没有数据，启动测试模式
    static unsigned long startTime = millis();
    if (millis() - startTime > 10000) {
      Serial.println("无数据，启动测试模式");
      Serial.println("提示: 可通过串口手动输入测试数据");
      Serial.println("格式: 距离,置信度 (如: 500,100)");
      manualTestMode();
    }
  }
}

// 平滑更新亮度
void updateBrightnessSmoothly() {
  if (millis() - lastBrightnessUpdate < BRIGHTNESS_UPDATE_INTERVAL) {
    return;
  }
  lastBrightnessUpdate = millis();
  
  // 如果目标亮度与当前亮度不同，进行平滑过渡
  if (currentBrightness != targetBrightness) {
    if (currentBrightness < targetBrightness) {
      currentBrightness += BRIGHTNESS_STEP;
      if (currentBrightness > targetBrightness) {
        currentBrightness = targetBrightness;
      }
    } else {
      currentBrightness -= BRIGHTNESS_STEP;
      if (currentBrightness < targetBrightness) {
        currentBrightness = targetBrightness;
      }
    }
    
    // 应用新的亮度值
    analogWrite(LIGHT_ENA_PIN, currentBrightness);
    analogWrite(LIGHT_ENB_PIN, currentBrightness);
  }
}

// 控制灯光亮度
void controlLights(int distance) {
  // 计算目标亮度
  targetBrightness = calculateBrightness(distance);
  
  // 设置IN1-IN4控制电机方向
  // 对于灯光控制，我们设置正向旋转
  digitalWrite(LIGHT_IN1_PIN, HIGH);   // 电机A正转
  digitalWrite(LIGHT_IN2_PIN, LOW);
  digitalWrite(LIGHT_IN3_PIN, HIGH);   // 电机B正转
  digitalWrite(LIGHT_IN4_PIN, LOW);
  
  // 简化输出以节省内存
  Serial.print("距离:");
  Serial.print(distance);
  Serial.print(" 亮度:");
  Serial.println(targetBrightness);
}

// 播放音频
void playAudio(int distance) {
  int audioLevel = getAudioLevel(distance);
  
  // 防止重复播放相同音频
  if (audioLevel == lastAudioLevel && (millis() - lastAudioTime) < 2000) {
    return;
  }
  
  if (audioLevel > 0) {
    // 发送播放命令到DFPlayer
    byte playCmd[] = {0x7E, 0xFF, 0x06, 0x03, 0x00, 0x00, (byte)audioLevel, 0xFE, 0xF7, 0xEF};
    for (int i = 0; i < 10; i++) {
      dfplayerSerial.write(playCmd[i]);
    }
    
    lastAudioTime = millis();
    lastAudioLevel = audioLevel;
    
    Serial.print("音频:");
    Serial.println(audioLevel);
  }
}

// 获取距离等级 (1-5, 距离越近等级越高)
int getDistanceLevel(int distance) {
  if (distance <= DISTANCE_LEVEL_1_MAX) {
    return 1;  // 0-40cm
  } else if (distance >= DISTANCE_LEVEL_2_MIN && distance < DISTANCE_LEVEL_2_MAX) {
    return 2;  // 40-80cm
  } else if (distance >= DISTANCE_LEVEL_3_MIN && distance < DISTANCE_LEVEL_3_MAX) {
    return 3;  // 80-120cm
  } else if (distance >= DISTANCE_LEVEL_4_MIN && distance < DISTANCE_LEVEL_4_MAX) {
    return 4;  // 120-140cm
  } else if (distance >= DISTANCE_LEVEL_5_MIN && distance <= DISTANCE_LEVEL_5_MAX) {
    return 5;  // 140-180cm
  }
  return 0;  // 超出范围
}

// 获取音频等级 (1-4, 距离越近等级越高)
int getAudioLevel(int distance) {
  if (distance <= AUDIO_LEVEL_1_MAX) {
    return 1;  // 0-10cm
  } else if (distance >= AUDIO_LEVEL_2_MIN && distance < AUDIO_LEVEL_2_MAX) {
    return 2;  // 10-20cm
  } else if (distance >= AUDIO_LEVEL_3_MIN && distance < AUDIO_LEVEL_3_MAX) {
    return 3;  // 20-30cm
  } else if (distance >= AUDIO_LEVEL_4_MIN && distance < AUDIO_LEVEL_4_MAX) {
    return 4;  // 30-40cm
  }
  return 0;  // 超出范围
}

// 计算亮度 (距离越近亮度越强，平滑渐变)
int calculateBrightness(int distance) {
  // 定义有效距离范围 (20mm - 1800mm)
  const int minDistance = 20;   // 最小距离 2cm
  const int maxDistance = 1800; // 最大距离 180cm
  const int maxBrightness = 255; // 最大亮度
  const int minBrightness = 10;  // 最小亮度（不完全关闭）
  
  // 限制距离范围
  if (distance < minDistance) {
    distance = minDistance;
  } else if (distance > maxDistance) {
    distance = maxDistance;
  }
  
  // 使用反比例函数计算亮度，距离越近亮度越高
  // brightness = maxBrightness - (distance - minDistance) * (maxBrightness - minBrightness) / (maxDistance - minDistance)
  int brightness = maxBrightness - (distance - minDistance) * (maxBrightness - minBrightness) / (maxDistance - minDistance);
  
  // 确保亮度在有效范围内
  if (brightness < minBrightness) {
    brightness = minBrightness;
  } else if (brightness > maxBrightness) {
    brightness = maxBrightness;
  }
  
  return brightness;
}




