/******************************************************************************
 * 无锡市思知瑞科技有限公司
 * 淘宝店铺：大脑实验室
 * 店铺网址：http://brainlab.taobao.com
 * 工作日联系电话：0510-66759621
******************************************************************************/
#include <Arduino.h>
#include <Adafruit_SSD1306.h>

#define OLED_Address 0x3C
Adafruit_SSD1306 oled(128, 64);

#define DEBUG  //取消注释，在上位机显示

typedef struct  // 曲线参数
{
  float Draw_Buf[128];  // 曲线数据缓存
  float Draw_Min;       // 缓存数据的最小值  单位为百分比
  float Draw_Max;       // 缓存数据的最大值
} _DrawCurve;           // 曲线参数

static _DrawCurve DrawCurve;

void PlotDataInput(_DrawCurve *Draw, float val) {
  uint8_t i;
  for (i = 1; i < 128; i++) {
    Draw->Draw_Buf[i - 1] = Draw->Draw_Buf[i];
  }
  Draw->Draw_Buf[127] = val;

  if (Draw->Draw_Buf[127] > 1)
    Draw->Draw_Buf[127] = 1;
  else if (Draw->Draw_Buf[127] < 0)
    Draw->Draw_Buf[127] = 0;
/******************************************************************************
 * 无锡市思知瑞科技有限公司
 * 淘宝店铺：大脑实验室
 * 店铺网址：http://brainlab.taobao.com
 * 工作日联系电话：0510-66759621
******************************************************************************/
  Draw->Draw_Max = 0;
  Draw->Draw_Min = 1;
  for (i = 0; i < 128; i++)  // 获取最大最小范围
  {
    if (Draw->Draw_Min > (Draw->Draw_Buf[i])) {
      Draw->Draw_Min = Draw->Draw_Buf[i];
      Draw->Draw_Min = (float)((uint8_t)(Draw->Draw_Min * 100) / 5) / 20;

      if (Draw->Draw_Min < 0)
        Draw->Draw_Min = 0;
      if ((Draw->Draw_Max - Draw->Draw_Min) < 0.05)
        Draw->Draw_Max = Draw->Draw_Min + 0.05;
    } else if (Draw->Draw_Max < Draw->Draw_Buf[i]) {
      Draw->Draw_Max = Draw->Draw_Buf[i];
      Draw->Draw_Max = (float)((uint8_t)(Draw->Draw_Max * 100) / 5 + 1) / 20;
      if (Draw->Draw_Max > 1.0)
        Draw->Draw_Max = 1.0;
      if ((Draw->Draw_Max - Draw->Draw_Min) < 0.05)
        Draw->Draw_Min = Draw->Draw_Max - 0.05;
    }
  }
}

void PlotDataPrint(_DrawCurve *Draw, int fllor, int upper, int lineColor) {

  for (int x = 1; x < 128; x++) {
    float k = (upper - fllor) / (Draw->Draw_Max - Draw->Draw_Min);  // Y轴缩放系数

    float last_y = upper - ((Draw->Draw_Buf[x - 1] - Draw->Draw_Min) * k);
    float now_y = upper - ((Draw->Draw_Buf[x] - Draw->Draw_Min) * k);

    oled.drawLine(x - 1, last_y, x, now_y, lineColor);
  }
}

void setup() {

#ifndef DEBUG
  oled.begin(SSD1306_SWITCHCAPVCC, OLED_Address);
  oled.clearDisplay();
#else
  Serial.begin(115200);

#endif
  delay(100);
}

void loop() {
  static uint16_t value = 0;
  value = analogRead(A0);

#ifndef DEBUG
  PlotDataInput(&DrawCurve, value / 1024.0);  // 传入数据
  oled.clearDisplay();
  PlotDataPrint(&DrawCurve, 10, 60, WHITE);  // 打印曲线
  oled.display();

#else
  Serial.println(value);
  delay(5);
#endif
}
/******************************************************************************
 * 无锡市思知瑞科技有限公司
 * 淘宝店铺：大脑实验室
 * 店铺网址：http://brainlab.taobao.com
 * 工作日联系电话：0510-66759621
******************************************************************************/