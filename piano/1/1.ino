// 定义引脚：A0-A5作为模拟输入（对应音频1-6），2-9作为数字输入（对应音频7-14）
const int analogPins[] = {A0, A1, A2, A3, A4, A5};  // 6个模拟引脚
const int digitalPins[] = {2, 3, 4, 5, 6, 7, 8, 9};  // 8个数字引脚
const int totalPins = 14;  // 总共14个输入

// 模拟输入的阈值和去抖设置
const int analogThreshold = 190;
const unsigned long debounceDelay = 1200;

// 模拟输入相关变量
int lastAnalogValues[6];
// 数字输入相关变量
int lastDigitalStates[8];  // 8个数字引脚的上一次状态
int digitalStateCounter[8] = {0};  // 8个数字引脚的状态稳定计数器
const int digitalStableThreshold = 10;  // 需要连续检测到10次才认为状态改变
unsigned long lastTriggerTime[14] = {0};  // 14个引脚的触发时间

// 全局触发锁定：确保同一时刻只能有一个引脚触发
unsigned long globalLastTriggerTime = 0;
const unsigned long globalDebounceDelay = 1000;
const unsigned long digitalDebounceDelay = 3500;  // 数字引脚专用延迟

// 模拟输入平滑滤波
const int numReadings = 5;
int readings[6][5];
int readIndex[6] = {0};
int total[6] = {0};

void setup() {
  Serial.begin(9600);
  
  // 初始化数字引脚2-9为输入模式（不使用上拉电阻）
  for (int i = 0; i < 8; i++) {
    pinMode(digitalPins[i], INPUT);
    lastDigitalStates[i] = digitalRead(digitalPins[i]);
  }
  
  // 初始化模拟引脚A0-A5
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < numReadings; j++) {
      readings[i][j] = 0;
    }
    lastAnalogValues[i] = analogRead(analogPins[i]);
  }
  
  delay(1000);  // 等待串口稳定
}

void loop() {
  unsigned long currentTime = millis();
  
  // 1. 检查模拟引脚A0-A5（对应音频1-6）
  for (int i = 0; i < 6; i++) {
    // 更新移动平均值
    total[i] = total[i] - readings[i][readIndex[i]];
    readings[i][readIndex[i]] = analogRead(analogPins[i]);
    total[i] = total[i] + readings[i][readIndex[i]];
    readIndex[i] = (readIndex[i] + 1) % numReadings;
    
    // 计算平滑后的值
    int smoothValue = total[i] / numReadings;
    int change = abs(smoothValue - lastAnalogValues[i]);
    
    // 检查是否有足够大的变化
    if (change > analogThreshold) {
      if ((currentTime - globalLastTriggerTime) > globalDebounceDelay) {
        // 发送对应的编号（1-6）
        Serial.println(i + 1);
        globalLastTriggerTime = currentTime;
        lastTriggerTime[i] = currentTime;
      }
    }
    
    lastAnalogValues[i] = smoothValue;
  }
  
  // 2. 检查数字引脚2-9（对应音频7-14）
  for (int i = 0; i < 8; i++) {
    int currentState = digitalRead(digitalPins[i]);
    
    // 检测从 LOW 到 HIGH 的跳变，但需要状态稳定
    if (currentState == HIGH && lastDigitalStates[i] == LOW) {
      digitalStateCounter[i]++;
      if (digitalStateCounter[i] >= digitalStableThreshold) {
        // 数字引脚使用独立的更长延迟时间
        if ((currentTime - lastTriggerTime[6 + i]) > digitalDebounceDelay) {
          Serial.println(7 + i);  // 数字引脚对应编号7-14
          globalLastTriggerTime = currentTime;
          lastTriggerTime[6 + i] = currentTime;
          lastDigitalStates[i] = HIGH;
          digitalStateCounter[i] = 0;
        }
      }
    } else if (currentState == LOW && lastDigitalStates[i] == HIGH) {
      digitalStateCounter[i]++;
      if (digitalStateCounter[i] >= digitalStableThreshold) {
        lastDigitalStates[i] = LOW;
        digitalStateCounter[i] = 0;
      }
    } else if (currentState == lastDigitalStates[i]) {
      digitalStateCounter[i] = 0;  // 状态一致，重置计数器
    }
  }
  
  // 添加小延迟以稳定读取
  delay(20);
}