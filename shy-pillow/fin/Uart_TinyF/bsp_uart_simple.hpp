#ifndef __BSP_UART_HPP_
#define __BSP_UART_HPP_

#include <stdio.h>
#include <string.h>
#include <Arduino.h>
#include <SoftwareSerial.h>

#ifdef __cplusplus
extern "C" {
#endif

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
#define DFPLAYER_RX_PIN 12
#define DFPLAYER_TX_PIN 13

void parseDistanceData(String &data);
void initLightControl();
void initAudioControl();
void controlLights(int distance);
void playAudio(int distance);
int getAudioLevel(int distance);
int calculateBrightness(int distance);
void updateBrightnessSmoothly();

#ifdef __cplusplus
}
#endif

#endif