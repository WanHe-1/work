#include "bsp_uart.hpp"
#include <stdio.h>
#include <SoftwareSerial.h>

// 全局变量
SoftwareSerial dfplayerSerial(DFPLAYER_RX_PIN, DFPLAYER_TX_PIN);
unsigned long lastAudioTime = 0;
int lastAudioLevel = 0;

// 灯光平滑过渡相关变量
int currentBrightness = 0;
int targetBrightness = 0;
unsigned long lastBrightnessUpdate = 0;
const int BRIGHTNESS_STEP = 5;
const int BRIGHTNESS_UPDATE_INTERVAL = 50;

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
  digitalWrite(LIGHT_IN1_PIN, HIGH);
  digitalWrite(LIGHT_IN2_PIN, LOW);
  digitalWrite(LIGHT_IN3_PIN, HIGH);
  digitalWrite(LIGHT_IN4_PIN, LOW);
  
  Serial.print("目标亮度:");
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

// 获取音频等级
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
  int brightness = maxBrightness - (distance - minDistance) * (maxBrightness - minBrightness) / (maxDistance - minDistance);
  
  // 确保亮度在有效范围内
  if (brightness < minBrightness) {
    brightness = minBrightness;
  } else if (brightness > maxBrightness) {
    brightness = maxBrightness;
  }
  
  return brightness;
}