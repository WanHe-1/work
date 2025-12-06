#include <stdio.h>
#include "bsp_uart.hpp"

void setup() {
  // 初始化串口  Initialize the serial port
  Serial.begin(115200);
  while (!Serial) {
    ; // 等待串口连接 Wait for the serial port connection
  }
  
  Serial.println("系统启动中...");
  
  // 初始化灯光控制
  initLightControl();
  
  // 初始化音频控制
  initAudioControl();
  
  Serial.println("初始化完成");
}

void loop() {
  if (Serial.available() > 0) {
    // 读取一行数据 (以换行符结束) Read a line of data (ending with a newline character)
    String data = Serial.readStringUntil('\n');
    
    // 解析数据
    parseDistanceData(data);
  }
  
  // 平滑更新灯光亮度
  updateBrightnessSmoothly();
}