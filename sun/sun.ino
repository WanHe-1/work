#include <DFRobotDFPlayerMini.h>
#include <SoftwareSerial.h>

SoftwareSerial mySoftwareSerial(10, 11); // RX, TX
DFRobotDFPlayerMini myDFPlayer;//创建一个 DFPlayer Mini 类的对象，
 //命名为 myDFPlayer，用于控制 DFPlayer Mini 模块（导入一些函数）


#define A4_PIN A4  // A4 引脚
#define A5_PIN A5  // A5 引脚

void setup()
{
  // 初始化串口
  Serial.begin(9600);
  mySoftwareSerial.begin(9600);

    // 初始化DFPlayer Mini
  Serial.println("Initializing DFPlayer Mini...");
  delay(2500);  // 加入延时，确保模块有时间启动
  
  // 初始化DFPlayer Mini
  if (!myDFPlayer.begin(mySoftwareSerial)) {
    Serial.println("DFPlayer Mini not detected!");
    while(true);//一直循环
  }
  Serial.println("DFPlayer Mini ready!");//打印准备

  // 设置音量（范围：0到30）
  myDFPlayer.volume(30);  


    // 初始化 A4 和 A5 引脚为输入
  pinMode(A4_PIN, INPUT);
  pinMode(A5_PIN, INPUT);
}

void loop() 
{
  int a4Value = digitalRead(A4_PIN); // 读取 A4 引脚的电平
  int a5Value = digitalRead(A5_PIN); // 读取 A5 引脚的电平
  // 读取压力传感器的值
  if (a4Value == HIGH || a5Value == HIGH) {
    // 如果 A4 或 A5 任一引脚有电，播放音乐
    Serial.println("A4 或 A5 电平为高，播放音乐！");
    myDFPlayer.play(1);  // 播放第一个音频文件（需要根据你的音频模块配置）
    delay(5000);  // 防止重复播放
  } else {
    // 如果电平相同，可以选择不播放或暂停音乐
    Serial.println("A4 和 A5 电平相同，暂停播放！");
    myDFPlayer.stop();  // 停止播放音乐
  }
  delay(500);  // 延迟一段时间再读取
}
