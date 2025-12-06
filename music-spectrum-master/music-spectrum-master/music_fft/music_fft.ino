#include <ArduinoFFT.h>

const int micPin = A1;  // 麦克风输入引脚
const int redPin = 9;    // 红色LED
const int greenPin = 10; // 绿色LED
const int bluePin = 11;  // 蓝色LED

#define SAMPLES 128             // FFT样本数量
#define SAMPLING_FREQUENCY 1000 // 采样频率 (Hz)

ArduinoFFT FFT = ArduinoFFT();  // 创建 FFT 对象

double vReal[SAMPLES];   // 实部
double vImag[SAMPLES];   // 虚部

void setup() {
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  Serial.begin(9600);  // 启动串口监视器
}

void loop() {
  // 读取麦克风输入的数据
  for (int i = 0; i < SAMPLES; i++) {
    vReal[i] = analogRead(micPin);  // 读取模拟输入
    vImag[i] = 0;                   // 初始化虚部为零
    delayMicroseconds(1000);         // 延迟，控制采样频率
  }

  // 执行 FFT 计算
  FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD); // 加窗
  FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);  // 执行 FFT
  FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);  // 计算幅度

  // 寻找峰值频率
  double peak = 0;
  int peakIndex = 0;
  for (int i = 0; i < (SAMPLES / 2); i++) {
    if (vReal[i] > peak) {
      peak = vReal[i];
      peakIndex = i;
    }
  }

  // 计算频率
  double frequency = (peakIndex * 1.0 * SAMPLING_FREQUENCY) / SAMPLES;
  
  // 根据频率设置LED颜色
  if (frequency < 200) {    // 低频段 (红色)
    analogWrite(redPin, 255);    
    analogWrite(greenPin, 0);           
    analogWrite(bluePin, 0);            
  } else if (frequency < 500) {  // 中频段 (绿色)
    analogWrite(redPin, 0);          
    analogWrite(greenPin, 255);  
    analogWrite(bluePin, 0);  
  } else {    // 高频段 (蓝色)
    analogWrite(redPin, 0);   
    analogWrite(greenPin, 0);  
    analogWrite(bluePin, 255);
  }

  // 输出频率值到串口
  Serial.print("Frequency: ");
  Serial.print(frequency);
  Serial.println(" Hz");

  delay(100);  // 延时
}
