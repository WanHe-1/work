// 定义引脚
const int RELAY_PUMP_INFLATE = 2;  
const int RELAY_PUMP_DEFLATE = 3;

// GY25传感器使用硬件串口（引脚0-RX, 引脚1-TX）
// GY25 角度传感器变量
float ypr[3] = {0, 0, 0}; // Yaw, Pitch, Roll
unsigned char Re_buf[8], counter = 0;

// 角度控制参数
float initialYaw = -108.0;     // 初始Yaw角度（向上位置）
float initialPitch = -52.0;    // 初始Pitch角度
float initialRoll = 152.0;     // 初始Roll角度
const float ANGLE_TOLERANCE = 15.0;  // 角度容差
const float ANGLE_CHANGE_THRESHOLD = 90.0;  // 角度变化阈值（90度）

// 角度控制状态
enum AngleState {
  ANGLE_UPWARD,   // 向上位置（初始角度）
  ANGLE_CHANGED   // 角度偏离90度
};

AngleState angleState = ANGLE_UPWARD;

void setup() {
  pinMode(RELAY_PUMP_INFLATE, OUTPUT);
  pinMode(RELAY_PUMP_DEFLATE, OUTPUT);

  digitalWrite(RELAY_PUMP_INFLATE, LOW);
  digitalWrite(RELAY_PUMP_DEFLATE, LOW);

  Serial.begin(115200);
  delay(2000);
  Serial.write(0xA5);
  Serial.write(0x52);  // 初始化GY25，连续输出模式
}

void loop() {
  // 读取GY25角度传感器数据
  readGY25Data();
  
  // 基于角度自动控制气泵
  controlByAngle();
}

// 读取GY25传感器数据
void readGY25Data() {
  while (Serial.available()) {
    Re_buf[counter] = (unsigned char)Serial.read();
    
    if (counter == 0 && Re_buf[0] != 0xAA) return;  // 检查帧头
    
    counter++;
    
    if (counter == 8) {  // 接收到数据
      counter = 0;  // 重新赋值，准备下一帧数据的接收
      
      if (Re_buf[0] == 0xAA && Re_buf[7] == 0x55) {  // 检查帧头，帧尾
        ypr[0] = (Re_buf[1] << 8 | Re_buf[2]) / 100.0;  // 合成数据，去掉小数点后2位
        ypr[1] = (Re_buf[3] << 8 | Re_buf[4]) / 100.0;
        ypr[2] = (Re_buf[5] << 8 | Re_buf[6]) / 100.0;
        
        Serial.print("YPR[0]: ");
        Serial.print(ypr[0]);  // 显示航向
        Serial.print(" YPR[1]: ");
        Serial.print(ypr[1]);
        Serial.print(" YPR[2]: ");
        Serial.print(ypr[2]);
        Serial.println("");
        delay(100);
      }
    }
  }
}

// 基于角度控制气泵
void controlByAngle() {
  // 计算当前角度与初始角度的差值
  float yawDiff = abs(ypr[0] - initialYaw);
  float pitchDiff = abs(ypr[1] - initialPitch);
  float rollDiff = abs(ypr[2] - initialRoll);
  
  // 处理角度跨越-180/180边界的情况
  if (yawDiff > 180) yawDiff = 360 - yawDiff;
  if (pitchDiff > 180) pitchDiff = 360 - pitchDiff;
  if (rollDiff > 180) rollDiff = 360 - rollDiff;
  
  // 计算总体角度变化（使用Pitch作为主要判断依据）
  float totalChange = pitchDiff;
  
  // 状态1: 在向上位置（初始角度附近）-> 充气
  if (totalChange < ANGLE_TOLERANCE && angleState != ANGLE_UPWARD) {
    digitalWrite(RELAY_PUMP_INFLATE, HIGH);
    digitalWrite(RELAY_PUMP_DEFLATE, LOW);
    angleState = ANGLE_UPWARD;
    Serial.println("[控制] 充气模式 - 向上位置");
  }
  
  // 状态2: 角度偏离90度 -> 放气
  else if (abs(totalChange - ANGLE_CHANGE_THRESHOLD) < ANGLE_TOLERANCE && angleState != ANGLE_CHANGED) {
    digitalWrite(RELAY_PUMP_INFLATE, LOW);
    digitalWrite(RELAY_PUMP_DEFLATE, HIGH);
    angleState = ANGLE_CHANGED;
    Serial.println("[控制] 放气模式 - 角度偏离90度");
  }
}
