#ifndef __BSP_UART_HPP_
#define __BSP_UART_HPP_

#include <stdio.h>
#include <string.h>
#include <Arduino.h>
#include <SoftwareSerial.h>
#include <Wire.h>

#ifdef __cplusplus
extern "C" {
#endif

// 距离范围定义 (单位: mm)
#define DISTANCE_LEVEL_5_MIN 1400  // 140cm
#define DISTANCE_LEVEL_5_MAX 1800  // 180cm
#define DISTANCE_LEVEL_4_MIN 1200  // 120cm
#define DISTANCE_LEVEL_4_MAX 1400  // 140cm
#define DISTANCE_LEVEL_3_MIN 800   // 80cm
#define DISTANCE_LEVEL_3_MAX 1200  // 120cm
#define DISTANCE_LEVEL_2_MIN 400   // 40cm
#define DISTANCE_LEVEL_2_MAX 800   // 80cm
#define DISTANCE_LEVEL_1_MAX 400   // 0-40cm

// 音频距离范围定义 (单位: mm)
#define AUDIO_LEVEL_4_MIN 300  // 30cm
#define AUDIO_LEVEL_4_MAX 400  // 40cm
#define AUDIO_LEVEL_3_MIN 200  // 20cm
#define AUDIO_LEVEL_3_MAX 300  // 30cm
#define AUDIO_LEVEL_2_MIN 100  // 10cm
#define AUDIO_LEVEL_2_MAX 200  // 20cm
#define AUDIO_LEVEL_1_MAX 100  // 0-10cm

// 引脚定义
#define LIGHT_ENA_PIN 7
#define LIGHT_ENB_PIN 2
#define LIGHT_IN1_PIN 5
#define LIGHT_IN2_PIN 6
#define LIGHT_IN3_PIN 4
#define LIGHT_IN4_PIN 3
#define DFPLAYER_RX_PIN 12  // Arduino引脚11 → DFPlayer TX (Arduino接收)
#define DFPLAYER_TX_PIN 13  // Arduino引脚10 → DFPlayer RX (Arduino发送)
// TinyF传感器使用硬件串口(引脚0,1)
// 注意：使用硬件串口时无法进行串口调试
// #define TINYF_RX_PIN 8
// #define TINYF_TX_PIN 9

// 串口激光传感器使用硬件串口(引脚0,1)

void parseDistanceData(String &data);
void initLightControl();
void initAudioControl();
void initLaserSensor();
void readLaserData();
void controlLights(int distance);
void playAudio(int distance);
int getDistanceLevel(int distance);
int getAudioLevel(int distance);
int calculateBrightness(int distance);
void updateBrightnessSmoothly();
bool tryConnectLaser(long baudRate);
void manualTestMode();

#ifdef __cplusplus
}
#endif

#endif

