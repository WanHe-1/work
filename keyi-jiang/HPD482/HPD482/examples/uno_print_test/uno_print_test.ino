#include "HPD482.h"//打印机头文件
#include "picture.h"//待打印的图片数据

#define LED_PIN 13

HPD482 printer=HPD482();//实例化打印机模块

void setup() {
  pinMode(LED_PIN,OUTPUT);//LED

  //*********************以下部分调用打印函数开始打印**********************************//
  printer.Print_SetDeep(2);//大面积图片先降低加热深度，避免电源压降或打印头过热
  // printer.Print_Lines(5,2);//打印5条虚线，虚线间距2mm
  // printer.Motor_Run(24,0);//步进电机旋转进纸24/8=3mm
  // printer.Print_SStr((u8*)"HPD482 test.YOU LI diy.https://shop587795831.taobao.com/",12,6);//打印12号英文字符串
  // printer.Print_SStr((u8*)"HPD482 test.YOU LI diy.https://shop587795831.taobao.com/",16,8);//打印16号英文字符串
  // printer.Print_SStr((u8*)"HPD482 test.YOU LI diy.https://shop587795831.taobao.com/",24,16);//打印24号英文字符串
  // printer.Print_Img2Lcd(0,(u8*)Image1,1);//在坐标0位置打印图片1
  printer.Print_Img2Lcd(2,(u8*)Image1,1);//在坐标为20位置打印图片1
  // printer.Print_Img2Lcd(44,(u8*)Image1,1);//在坐标为44位置打印图片1（超出打印范围部分会被裁掉）
  // printer.Print_Img2Lcd(1,(u8*)Image2,1);//打印图片2，图片存放在Flash（PROGMEM）
  //************************************打印结束**************************************//
}

void loop() {
  // setup()中已经完成一次打印，之后不再执行任何操作。
}