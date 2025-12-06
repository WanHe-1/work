#include <stdio.h>
#include "bsp_uart.hpp"

void setup() {
  // 初始化串口  Initialize the serial port
  Serial.begin(115200);  // 使用115200波特率
  while (!Serial) {
    ; // 等待串口连接 Wait for the serial port connection
  }
  
  Serial.println("系统启动中...");
  
  // 初始化激光传感器
  initLaserSensor();
  
  // 初始化灯光控制
  initLightControl();
  
  // 初始化音频控制
  initAudioControl();
  
  Serial.println("初始化完成");
}

void loop() {
  // 读取激光传感器数据
  readLaserData();
  
  // 平滑更新灯光亮度
  updateBrightnessSmoothly();
  
  // 注意: 硬件串口被激光传感器占用，无法进行串口调试
  // 如需调试，请使用软件串口方案或LED指示灯
}

