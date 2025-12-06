#include <LiquidCrystal.h>

// 初始化LCD：RS=2, EN=3, D4=4, D5=5, D6=6, D7=7
LiquidCrystal lcd(2, 3, 4, 5, 6, 7);

// 传感器连接的引脚
const int sensorPin = A0;

// 变量
int sensorValue = 0;        // 原始ADC值 (0~1023)
int HRVValue = 0;      // 映射后的压力值 (0~100)
int minValue = 0;           // 最小原始值（可校准）
int maxValue = 1023;        // 最大原始值（可校准）

// 校准标志（可选：首次上电自动校准）
bool calibrated = false;

void setup() {
  // 初始化LCD
  lcd.begin(16, 2);
  lcd.print("HRV Sensor");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");

  delay(1000);

  // 【可选】自动校准：在传感器静止时读取最小/最大值
  // 请在上电时确保处于“最小压力”状态
  if (!calibrated) {
    autoCalibrate();
  }

  lcd.clear();
  lcd.print("HRV:");
}

void loop() {
  // 读取传感器原始值
  sensorValue = analogRead(sensorPin);

  // 将原始值映射到 0~100 范围
  HRVValue = map(sensorValue, minValue, maxValue, 0, 100);
  HRVValue = constrain(HRVValue, 0, 100);  // 限制在0~100

  // 在LCD第二行显示数值
  lcd.setCursor(0, 1);
  lcd.print("      ");        // 清除旧数字（留6位）
  lcd.setCursor(0, 1);
  lcd.print(HRVValue);
  lcd.print("/100");

  // 可选：显示简易条形图（0~10个#）
  lcd.print(" [");
  int bars = HRVValue / 10;  // 0~10格
  for (int i = 0; i < 10; i++) {
    if (i < bars)
      lcd.print("#");
    else
      lcd.print(" ");
  }
  lcd.print("]");

  delay(200);  // 刷新频率适中
}

// 【可选】自动校准函数：用于确定传感器的动态范围
void autoCalibrate() {
  lcd.clear();
  lcd.print("Calibrating...");
  lcd.setCursor(0, 1);
  lcd.print("Min value...");

  minValue = 1023;
  maxValue = 0;

  // 采集50个样本，找出最小最大值
  for (int i = 0; i < 50; i++) {
    int val = analogRead(sensorPin);
    if (val < minValue) minValue = val;
    if (val > maxValue) maxValue = val;
    delay(10);
  }

  // 防止映射错误（避免max=min）
  if (minValue >= maxValue) {
    minValue = 0;
    maxValue = 1023;
  }

  lcd.clear();
  lcd.print("Min:");
  lcd.print(minValue);
  lcd.print(" Max:");
  lcd.print(maxValue);
  delay(1500);
  calibrated = true;
}