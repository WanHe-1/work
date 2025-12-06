#include "Wire.h"
#include "Arduino.h"
#include "DFRobotDFPlayerMini.h"

// VL53L0X配置参数
#define VL53L0X_ADDRESS 0x29     // VL53L0X默认I2C地址
#define SHUT_PIN 2               // SHUT引脚连接到数字引脚2
#define INT_PIN 3                // INT引脚连接到数字引脚3

// VL53L0X寄存器定义
#define VL53L0X_REG_IDENTIFICATION_MODEL_ID         0xC0
#define VL53L0X_REG_IDENTIFICATION_REVISION_ID      0xC2
#define VL53L0X_REG_PRE_RANGE_CONFIG_VCSEL_PERIOD   0x50
#define VL53L0X_REG_FINAL_RANGE_CONFIG_VCSEL_PERIOD 0x70
#define VL53L0X_REG_SYSRANGE_START                  0x00
#define VL53L0X_REG_RESULT_INTERRUPT_STATUS         0x13
#define VL53L0X_REG_RESULT_RANGE_STATUS             0x14

// 全局配置变量
uint8_t i2c_addr = VL53L0X_ADDRESS;  // 芯片地址，默认0x29
bool debug = false;                  // 调试模式，默认false
TwoWire* i2c_bus = &Wire;           // I2C总线，默认Wire
uint8_t vl_config = 0;              // 传感器配置，默认0（使用默认值）

// 控制系统引脚定义
const int motorENA = 9;
const int motorIN1 = 7;
const int motorIN2 = 8;
const int ledPin = 4;  // 改为引脚4，避免与INT_PIN冲突

#include <SoftwareSerial.h>
SoftwareSerial mySoftwareSerial(12, 13); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

// 距离设置
const int MAX_DISTANCE = 200;  // 200cm
const int MIN_DISTANCE = 2;    // 2cm

// 音频文件映射
const int AUDIO_FILE_1 = 1; // 40-60cm
const int AUDIO_FILE_2 = 2; // 30-40cm
const int AUDIO_FILE_3 = 3; // 20-30cm
const int AUDIO_FILE_4 = 4; // 10-20cm
const int AUDIO_FILE_5 = 5; // 0-10cm

// 音频控制变量
unsigned long lastPlayTime = 0;
const long playInterval = 1000;
unsigned long audioStartTime = 0;
const long audioDuration = 2000;
bool isAudioPlaying = false;

// 写入VL53L0X寄存器
void writeReg(uint8_t reg, uint8_t value) {
  i2c_bus->beginTransmission(i2c_addr);
  i2c_bus->write(reg);
  i2c_bus->write(value);
  i2c_bus->endTransmission();
}

// 读取VL53L0X寄存器
uint8_t readReg(uint8_t reg) {
  i2c_bus->beginTransmission(i2c_addr);
  i2c_bus->write(reg);
  i2c_bus->endTransmission();
  
  i2c_bus->requestFrom(i2c_addr, (uint8_t)1);
  if (i2c_bus->available()) {
    return i2c_bus->read();
  }
  return 0;
}

// 读取16位寄存器
uint16_t readReg16(uint8_t reg) {
  i2c_bus->beginTransmission(i2c_addr);
  i2c_bus->write(reg);
  i2c_bus->endTransmission();
  
  i2c_bus->requestFrom(i2c_addr, (uint8_t)2);
  if (i2c_bus->available() >= 2) {
    uint16_t value = i2c_bus->read() << 8;
    value |= i2c_bus->read();
    return value;
  }
  return 0;
}

void sensor_init(uint8_t addr = VL53L0X_ADDRESS, bool debug_mode = false, TwoWire* bus = &Wire, uint8_t config = 0) {
  // 设置配置参数
  i2c_addr = addr;
  debug = debug_mode;
  i2c_bus = bus;
  vl_config = config;
  
  // 配置引脚
  pinMode(SHUT_PIN, OUTPUT);
  pinMode(INT_PIN, INPUT);
  
  // 重置传感器
  digitalWrite(SHUT_PIN, LOW);
  delay(10);
  digitalWrite(SHUT_PIN, HIGH);
  delay(100);  // 等待传感器启动
  
  i2c_bus->begin();
  
  if (debug) {
    Serial.println("[DEBUG] VL53L0X初始化开始...");
    
    // 读取设备ID验证
    uint8_t model_id = readReg(VL53L0X_REG_IDENTIFICATION_MODEL_ID);
    uint8_t revision_id = readReg(VL53L0X_REG_IDENTIFICATION_REVISION_ID);
    
    Serial.print("[DEBUG] 设备ID: 0x");
    Serial.print(model_id, HEX);
    Serial.print(", 版本ID: 0x");
    Serial.println(revision_id, HEX);
  }
  
  // 简化的VL53L0X初始化
  if (debug) {
    Serial.println("[DEBUG] 开始简化初始化序列...");
  }
  
  // 基本的数据初始化
  writeReg(0x88, 0x00);
  writeReg(0x80, 0x01);
  writeReg(0xFF, 0x01);
  writeReg(0x00, 0x00);
  writeReg(0x91, 0x3C);
  writeReg(0x00, 0x01);
  writeReg(0xFF, 0x00);
  writeReg(0x80, 0x00);
  
  // 设置基本测量参数
  writeReg(0x09, 0x00);
  writeReg(0x10, 0x00);
  writeReg(0x11, 0x00);
  writeReg(0x24, 0x01);
  writeReg(0x25, 0xff);
  writeReg(0x75, 0x00);
  
  // 设置时序预算
  writeReg(0x01, 0x80);
  writeReg(0x02, 0x00);
  
  delay(200);
  
  if (debug) {
    Serial.println("[DEBUG] 简化初始化完成");
  }
  
  if (debug) {
    Serial.println("VL53L0X传感器初始化完成");
  } else {
    Serial.println("VL53L0X传感器初始化完成");
  }
}

// 启用传感器
void sensor_enable() {
  digitalWrite(SHUT_PIN, HIGH);
  delay(100);
  if (debug) {
    Serial.println("[DEBUG] 传感器已启用");
  } else {
    Serial.println("传感器已启用");
  }
}

// 关闭传感器
void sensor_disable() {
  digitalWrite(SHUT_PIN, LOW);
  if (debug) {
    Serial.println("[DEBUG] 传感器已关闭");
  } else {
    Serial.println("传感器已关闭");
  }
}

// 检查INT引脚状态（数据就绪信号）
bool is_data_ready() {
  return digitalRead(INT_PIN) == HIGH;
}

// I2C设备扫描函数
void scan_i2c_devices() {
  Serial.println("[DEBUG] 扫描I2C设备...");
  int device_count = 0;
  
  for (byte address = 1; address < 127; address++) {
    i2c_bus->beginTransmission(address);
    byte error = i2c_bus->endTransmission();
    
    if (error == 0) {
      Serial.print("[DEBUG] 发现I2C设备，地址: 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      device_count++;
    }
  }
  
  if (device_count == 0) {
    Serial.println("[DEBUG] 警告: 未发现任何I2C设备！");
  } else {
    Serial.print("[DEBUG] 总共发现 ");
    Serial.print(device_count);
    Serial.println(" 个I2C设备");
  }
}

// 测试传感器通信
bool test_sensor_communication() {
  Serial.println("[DEBUG] 测试传感器通信...");
  
  i2c_bus->beginTransmission(i2c_addr);
  byte error = i2c_bus->endTransmission();
  
  if (error == 0) {
    Serial.println("[DEBUG] 传感器通信正常");
    return true;
  } else {
    Serial.print("[DEBUG] 传感器通信失败，错误代码: ");
    Serial.println(error);
    return false;
  }
}

// 简单的测量函数
int simple_measurement() {
  if (debug) {
    Serial.println("[DEBUG] 执行简单测量");
  }
  
  // 方法1: 直接启动测量并等待
  writeReg(0x00, 0x01);  // 启动测量
  delay(50);             // 等待测量完成
  
  // 读取距离
  uint16_t distance = readReg16(0x1E);
  
  if (debug) {
    Serial.print("[DEBUG] 简单测量结果: ");
    Serial.println(distance);
  }
  
  return distance;
}

int get_distance() {
  // 使用已验证有效的简单测量方法
  writeReg(0x00, 0x01);  // 启动测量
  delay(50);             // 等待测量完成
  
  // 读取距离
  uint16_t distance = readReg16(0x1E);
  
  // 检查测量是否有效
  if (distance == 8191 || distance == 0) {
    return -1;  // 无效测量
  }
  
  return distance;
}

// 测量距离函数 (返回厘米，兼容控制系统)
float measureDistance() {
  int distance_mm = get_distance();
  
  if (distance_mm < 0) {
    return MAX_DISTANCE;  // 测量失败时返回最大距离
  }
  
  float distance_cm = distance_mm / 10.0;  // 转换为厘米
  
  if (distance_cm < MIN_DISTANCE || distance_cm > MAX_DISTANCE) {
    return MAX_DISTANCE;
  }
  
  return distance_cm;
}

void setup() {
  Serial.begin(9600);
  mySoftwareSerial.begin(9600);
  delay(1000);

  Serial.println(F("=== TOF200C 控制系统 ==="));
  Serial.println(F("Initializing DFPlayer Mini..."));

  if (!myDFPlayer.begin(mySoftwareSerial)) {
    Serial.println(F("DFPlayer Mini initialization failed!"));
    while (true);
  }
  Serial.println(F("DFPlayer Mini initialized."));

  myDFPlayer.volume(30);
  myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);
  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);

  // Test playback
  myDFPlayer.play(1);
  delay(1000);
  myDFPlayer.stop();

  // 初始化TOF200C传感器
  sensor_init(0x29, false, &Wire, 0);

  pinMode(motorENA, OUTPUT);
  pinMode(motorIN1, OUTPUT);
  pinMode(motorIN2, OUTPUT);
  pinMode(ledPin, OUTPUT);

  Serial.println(F("TOF200C Control System Started"));
  Serial.println("====================");
}

void loop() {
  float distance_cm = measureDistance();
  Serial.print(F("Distance: "));
  Serial.print(distance_cm);
  Serial.println(F(" cm"));

  // Motor speed control
  int motorSpeed;
  if (distance_cm < 40) {
    motorSpeed = 255;
  } else if (distance_cm > 180) {
    motorSpeed = 0;
  } else {
    motorSpeed = map(distance_cm, 180, 40, 0, 255);
    motorSpeed = constrain(motorSpeed, 0, 255);
  }

  digitalWrite(motorIN1, HIGH);
  digitalWrite(motorIN2, LOW);
  analogWrite(motorENA, motorSpeed);
  Serial.print(F("Motor Speed: "));
  Serial.println(motorSpeed);

  // LED brightness control
  int ledBrightness = 0;
  if (distance_cm > 180) {
    ledBrightness = 0;
  } else if (distance_cm >= 140) {
    ledBrightness = map(distance_cm, 140, 180, 40, 80);
    ledBrightness = constrain(ledBrightness, 40, 80);
  } else if (distance_cm > 120) {
    ledBrightness = map(distance_cm, 120, 140, 80, 120);
    ledBrightness = constrain(ledBrightness, 80, 120);
  } else if (distance_cm > 80) {
    ledBrightness = map(distance_cm, 80, 120, 120, 180);
    ledBrightness = constrain(ledBrightness, 120, 180);
  } else if (distance_cm > 40) {
    ledBrightness = map(distance_cm, 40, 80, 180, 220);
    ledBrightness = constrain(ledBrightness, 180, 220);
  } else {
    ledBrightness = map(distance_cm, 0, 40, 220, 255);
    ledBrightness = constrain(ledBrightness, 220, 255);
  }
  analogWrite(ledPin, ledBrightness);
  Serial.print(F("LED Brightness: "));
  Serial.println(ledBrightness);

  // Audio playback
  unsigned long currentMillis = millis();

  if (isAudioPlaying && (currentMillis - audioStartTime >= audioDuration)) {
    myDFPlayer.stop();
    isAudioPlaying = false;
    Serial.println(F("Audio stopped after 2s"));
  }

  if (!isAudioPlaying && (currentMillis - lastPlayTime > playInterval)) {
    bool played = false;
    if (distance_cm > 40 && distance_cm <= 60) {
      myDFPlayer.play(AUDIO_FILE_1);
      Serial.println(F("Playing Audio 1 (40-60cm)"));
      played = true;
    } else if (distance_cm > 30 && distance_cm <= 40) {
      myDFPlayer.play(AUDIO_FILE_2);
      Serial.println(F("Playing Audio 2 (30-40cm)"));
      played = true;
    } else if (distance_cm > 20 && distance_cm <= 30) {
      myDFPlayer.play(AUDIO_FILE_3);
      Serial.println(F("Playing Audio 3 (20-30cm)"));
      played = true;
    } else if (distance_cm > 10 && distance_cm <= 20) {
      myDFPlayer.play(AUDIO_FILE_4);
      Serial.println(F("Playing Audio 4 (10-20cm)"));
      played = true;
    } else if (distance_cm <= 10) {
      myDFPlayer.play(AUDIO_FILE_5);
      Serial.println(F("Playing Audio 5 (0-10cm)"));
      played = true;
    }

    if (played) {
      isAudioPlaying = true;
      audioStartTime = currentMillis;
      lastPlayTime = currentMillis;
    }
  }

  delay(300);
}