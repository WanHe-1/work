#include <Wire.h>

#define MPU_ADDR         0x68
#define TRIGGER_PIN      3
#define ANGLE_THRESHOLD  50.0
#define HOLD_TIME        5000
#define I2C_SDA          8
#define I2C_SCL          9

float prev_pitch = 0;
float prev_roll  = 0;
unsigned long pin_high_start = 0;
bool pin_active = false;
bool first_read = true;

// 从 MPU6050 读取加速度和陀螺仪数据
void readMPU6050(float &ax, float &ay, float &az, float &gx, float &gy, float &gz) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);  // 从 ACCEL_XOUT_H 寄存器开始
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)MPU_ADDR, (uint8_t)14, (uint8_t)true);

  ax = (Wire.read() << 8 | Wire.read()) / 16384.0;  // 加速度 X
  ay = (Wire.read() << 8 | Wire.read()) / 16384.0;  // 加速度 Y
  az = (Wire.read() << 8 | Wire.read()) / 16384.0;  // 加速度 Z
  Wire.read() << 8 | Wire.read();                    // 温度（跳过）
  gx = (Wire.read() << 8 | Wire.read()) / 131.0;    // 陀螺仪 X
  gy = (Wire.read() << 8 | Wire.read()) / 131.0;    // 陀螺仪 Y
  gz = (Wire.read() << 8 | Wire.read()) / 131.0;    // 陀螺仪 Z
}

void setup(void) {
  pinMode(TRIGGER_PIN, OUTPUT);
  digitalWrite(TRIGGER_PIN, LOW);

  Serial.begin(115200);
  delay(2000);

  // 初始化 I2C，指定引脚
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000);  // 400kHz I2C 速率

  // 唤醒 MPU6050（退出睡眠模式）
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);  // PWR_MGMT_1 寄存器
  Wire.write(0x00);  // 写入0，唤醒传感器
  byte error = Wire.endTransmission();

  if (error != 0) {
    Serial.print("MPU6050 初始化失败，错误码: ");
    Serial.println(error);
    while (1) delay(10);
  }
  Serial.println("MPU6050 OK");
  delay(100);
}

void loop() {
  float ax, ay, az, gx, gy, gz;
  readMPU6050(ax, ay, az, gx, gy, gz);

  // 用加速度数据计算俯仰角和横滚角
  float pitch = atan2(-ax, sqrt(ay * ay + az * az)) * 180 / PI;
  float roll  = atan2(ay, az) * 180 / PI;

  // 检查是否需要拉低 GPIO3
  if (pin_active && (millis() - pin_high_start >= HOLD_TIME)) {
    digitalWrite(TRIGGER_PIN, LOW);
    pin_active = false;
    Serial.println("PIN3 LOW");
  }

  // 检测角度突变
  if (!first_read) {
    float pitch_change = fabs(pitch - prev_pitch);
    float roll_change  = fabs(roll - prev_roll);
    if ((pitch_change > ANGLE_THRESHOLD || roll_change > ANGLE_THRESHOLD) && !pin_active) {
      digitalWrite(TRIGGER_PIN, HIGH);
      pin_high_start = millis();
      pin_active = true;
      Serial.print("PIN3 HIGH 5s  pch=");
      Serial.print(pitch_change);
      Serial.print(" rch=");
      Serial.println(roll_change);
    }
  } else {
    first_read = false;
  }

  prev_pitch = pitch;
  prev_roll  = roll;

  Serial.print("P:");
  Serial.print(pitch);
  Serial.print(" R:");
  Serial.print(roll);
  Serial.print(" ");
  Serial.println(pin_active ? "O" : "-");
  delay(500);
}