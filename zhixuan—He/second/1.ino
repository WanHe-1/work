// 定义引脚
const int relayPin1 = 9; // 支持 PWM
const int relayPin2 = 8; // 不支持 PWM

// 用于记录 PWM 输出开始时间的变量
unsigned long startTime = 0;
// 标记定时器是否正在运行
bool isTimerRunning = false;
// 20秒的毫秒数 (20 * 1000)
const long duration = 20000; 

void setup() {
  // 初始化串口通信，波特率设置为9600
  Serial.begin(9600);
  
  // 设置引脚为输出模式
  pinMode(relayPin1, OUTPUT);
  pinMode(relayPin2, OUTPUT);
  
  // 初始状态设为低电平（关闭）
  digitalWrite(relayPin1, LOW);
  digitalWrite(relayPin2, LOW);
  
  Serial.println("Arduino已就绪。请在串口监视器发送数字 '1' 来触发。");
}

void loop() {
  // 1. 检查串口是否有数据传入
  if (Serial.available() > 0) {
    // 读取传入的数据
    char receivedChar = Serial.read();
    
    // 2. 判断是否收到了字符 '1'，且当前定时器没有在运行
    if (receivedChar == '1' && !isTimerRunning) {
      // 引脚 9 输出 PWM 值 125
      analogWrite(relayPin1, 125);
      // 引脚 8 输出高电平 255 (因为引脚8不支持PWM)
      digitalWrite(relayPin2, HIGH);
      
      // 记录当前时间并启动定时器
      startTime = millis();
      isTimerRunning = true;
      
      Serial.println("收到信号 '1'，引脚9输出PWM(125)，引脚8输出高电平，持续20秒。");
    }
  }

  // 3. 检查20秒是否已经过去（非阻塞式延时）
  if (isTimerRunning && (millis() - startTime >= duration)) {
    // 时间到，将引脚拉低
    digitalWrite(relayPin1, LOW);
    digitalWrite(relayPin2, LOW);
    
    // 停止定时器
    isTimerRunning = false;
    
    Serial.println("20秒结束，引脚已关闭。");
  }
}