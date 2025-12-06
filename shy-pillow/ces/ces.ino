#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

// --- 引脚定义 ---
#define DFPLAYER_RX_PIN 12  // Arduino RX (接 DFPlayer TX)
#define DFPLAYER_TX_PIN 13  // Arduino TX (接 DFPlayer RX)

#define L298N_ENA 7   // PWM
#define L298N_ENB 3   // PWM
#define L298N_IN1 5
#define L298N_IN2 6
#define L298N_IN3 4
#define L298N_IN4 2

// --- 全局对象 ---
SoftwareSerial dfSerial(DFPLAYER_RX_PIN, DFPLAYER_TX_PIN); // RX, TX
DFRobotDFPlayerMini dfPlayer;

// --- 状态变量 ---
int currentBrightness = 0;    // 当前亮度
int targetBrightness = 0;     // 目标亮度
unsigned long lastUpdateTime = 0;  // 上次更新时间
const int SMOOTH_DELAY = 50;       // 亮度变化间隔(ms) - 调高避免卡顿
const int BRIGHTNESS_STEP = 3;     // 每次亮度变化步长 - 调小更顺滑

// 音频播放控制变量
bool audio1Playing = false;
bool audio2Playing = false;
bool audio3Playing = false;
bool audio4Playing = false;

unsigned long audio1StartTime = 0;
unsigned long audio2StartTime = 0;
unsigned long audio3StartTime = 0;
unsigned long audio4StartTime = 0;

const unsigned long AUDIO_DURATION = 1000; // 音频播放时长1秒

void setup() {
  // 初始化硬件串口（用于接收 TinyF 数据）
  Serial.begin(115200);
  Serial.println("System started. Waiting for TinyF data...");

  // 初始化 L298N 控制引脚
  pinMode(L298N_ENA, OUTPUT);
  pinMode(L298N_ENB, OUTPUT);
  pinMode(L298N_IN1, OUTPUT);
  pinMode(L298N_IN2, OUTPUT);
  pinMode(L298N_IN3, OUTPUT);
  pinMode(L298N_IN4, OUTPUT);

  // 设置电机方向（灯亮：IN1/IN3 = HIGH, IN2/IN4 = LOW）
  digitalWrite(L298N_IN1, HIGH);
  digitalWrite(L298N_IN2, LOW);
  digitalWrite(L298N_IN3, HIGH);
  digitalWrite(L298N_IN4, LOW);
  
  // 初始化PWM输出为0（确保两个灯都从关闭状态开始）
  analogWrite(L298N_ENA, 0);
  analogWrite(L298N_ENB, 0);
  

  // 初始化 DFPlayer Mini
  dfSerial.begin(9600);
  Serial.println("Initializing DFPlayer Mini...");
  if (!dfPlayer.begin(dfSerial)) {
    Serial.println("❌ DFPlayer initialization failed!");
    Serial.println("Check wiring, SD card, and power.");
    while (true) delay(1000); // 卡死提示
  }
  dfPlayer.volume(25);      // 音量 0~30
  dfPlayer.loop(false);     // 不循环播放
  Serial.println("✅ DFPlayer Mini ready.");
}

void loop() {
  // 顺滑亮度更新
  updateSmoothBrightness();
  
  // 检查音频播放时间，1秒后自动停止
  checkAudioTiming();
  
  if (Serial.available() > 0) {
    String data = Serial.readStringUntil('\n');
    parseDistanceData(data);
  }
}

void parseDistanceData(String &data) {
  // 清理整行数据
  data.trim();

  // 查找逗号分隔符
  int commaPos = data.indexOf(',');
  if (commaPos == -1) return;

  // 提取距离部分（注意：trim() 是 void，不能链式调用！）
  String distStr = data.substring(0, commaPos);
  distStr.trim();

  // 提取置信度部分
  String confStr = data.substring(commaPos + 1);
  confStr.trim();

  // 转为数值
  long distance_mm = distStr.toInt();
  long confidence = confStr.toInt();

  // 数据有效性校验
  if ((distance_mm <= 20 || distance_mm >= 4000) && distStr != "0") return;
  if (confidence == 0 && confStr != "0") return;

  // 转换为厘米
  float distance_cm = distance_mm / 10.0;

  // ====== 顺滑灯光亮度控制 ======
  // 使用连续函数计算目标亮度，避免阶梯式跳跃
  if (distance_cm >= 0 && distance_cm <= 200) {
    // 距离越近，亮度越高，使用指数衰减函数
    // 在0-40cm范围内达到最大亮度255
    // 在40-200cm范围内逐渐衰减到0
    if (distance_cm <= 40) {
      // 0-40cm: 线性增长到最大亮度
      targetBrightness = map(distance_cm, 0, 40, 255, 200);
    } else {
      // 40-200cm: 指数衰减
      float ratio = (distance_cm - 40) / 160.0; // 0到1的比值
      targetBrightness = 200 * exp(-ratio * 3.0); // 指数衰减
      if (targetBrightness < 10) targetBrightness = 0; // 低于10就熄灭
    }
  } else {
    targetBrightness = 0; // 超出200cm，灯灭
  }

  // ====== 音频播放控制（1秒播放） ======
  // 检查当前距离范围，启动对应音频播放
  if (distance_cm >= 30 && distance_cm < 40 && !audio1Playing) {
    dfPlayer.play(1);
    audio1Playing = true;
    audio1StartTime = millis();
    Serial.println("🔊 Audio 1 (30-40cm)");
  } else if (distance_cm >= 20 && distance_cm < 30 && !audio2Playing) {
    dfPlayer.play(2);
    audio2Playing = true;
    audio2StartTime = millis();
    Serial.println("🔊 Audio 2 (20-30cm)");
  } else if (distance_cm >= 10 && distance_cm < 20 && !audio3Playing) {
    dfPlayer.play(3);
    audio3Playing = true;
    audio3StartTime = millis();
    Serial.println("🔊 Audio 3 (10-20cm)");
  } else if (distance_cm >= 0 && distance_cm < 10 && !audio4Playing) {
    dfPlayer.play(4);
    audio4Playing = true;
    audio4StartTime = millis();
    Serial.println("🔊 Audio 4 (0-10cm)");
  }

  // 调试输出（简化）
  Serial.print("📏 ");
  Serial.print(distance_cm, 1);
  Serial.print("cm | 💡 ");
  Serial.println(currentBrightness);
}

// 顺滑亮度更新函数
void updateSmoothBrightness() {
  unsigned long currentTime = millis();
  
  // 检查是否需要更新亮度
  if (currentTime - lastUpdateTime >= SMOOTH_DELAY) {
    if (currentBrightness != targetBrightness) {
      // 计算亮度变化方向
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
      
      // 应用亮度到PWM输出
      analogWrite(L298N_ENA, currentBrightness);
      analogWrite(L298N_ENB, currentBrightness);
    }
    
    lastUpdateTime = currentTime;
  }
}


// 音频时间检查函数
void checkAudioTiming() {
  unsigned long currentTime = millis();
  
  // 检查每个音频是否播放超过1秒
  if (audio1Playing && (currentTime - audio1StartTime >= AUDIO_DURATION)) {
    dfPlayer.stop();
    audio1Playing = false;
  }
  
  if (audio2Playing && (currentTime - audio2StartTime >= AUDIO_DURATION)) {
    dfPlayer.stop();
    audio2Playing = false;
  }
  
  if (audio3Playing && (currentTime - audio3StartTime >= AUDIO_DURATION)) {
    dfPlayer.stop();
    audio3Playing = false;
  }
  
  if (audio4Playing && (currentTime - audio4StartTime >= AUDIO_DURATION)) {
    dfPlayer.stop();
    audio4Playing = false;
  }
}


