#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FastLED.h>


//==============================
// WiFi
//==============================

const char* ssid = "WanHe'phone";
const char* password = "000000qq";


ESP8266WebServer server(80);


//WiFi状态
bool wifiOK=false;

unsigned long wifiStart;



//==============================
// LED
//==============================

#define LED_PIN 4

#define LED_PER_STRIP 20
#define STRIP_COUNT 6

#define NUM_LEDS (LED_PER_STRIP*STRIP_COUNT)


CRGB leds[NUM_LEDS];



#define BLUE_R 80
#define BLUE_G 180
#define BLUE_B 255



#define FLOW_TIME 5000
#define FADE_IN_TIME 300
#define HOLD_TIME 1000


#define TRAIL_LENGTH 6


#define BRIGHTNESS 140



unsigned long effectStart;



String ledState="流水灯";




//==============================
// 初始化
//==============================

void setup()
{


Serial.begin(115200);



FastLED.addLeds<
WS2812B,
LED_PIN,
GRB
>
(
leds,
NUM_LEDS
);



FastLED.setBrightness(
BRIGHTNESS
);


FastLED.clear();
FastLED.show();



effectStart=millis();



//==============================
// WiFi非阻塞连接
//==============================

WiFi.begin(
ssid,
password
);


wifiStart=millis();



//网页

server.on("/",[](){

String html;


html+="<html>";
html+="<meta charset='utf-8'>";
html+="<body>";

html+="<h2>ESP8266 RGB灯控制状态</h2>";



if(wifiOK)
{

html+="WiFi状态: 已连接<br>";

html+="SSID:";
html+=ssid;
html+="<br>";

html+="IP:";
html+=WiFi.localIP().toString();
html+="<br>";

}
else
{

html+="WiFi状态: 未连接<br>";

}


html+="LED模式:";
html+=ledState;


html+="</body></html>";



server.send(
200,
"text/html",
html
);


});



server.begin();


}



//==============================
// WiFi后台任务
//==============================

void wifiTask()
{


if(wifiOK)
return;



//最多等待8秒

if(
WiFi.status()==WL_CONNECTED
)
{

wifiOK=true;

Serial.println("WiFi OK");

Serial.println(
WiFi.localIP()
);


}

else if(
millis()-wifiStart>8000
)
{

//放弃连接

WiFi.disconnect();

Serial.println(
"WiFi timeout"
);


}


}



//==============================
// 蓝色
//==============================

CRGB blueColor()
{

return CRGB(
BLUE_R,
BLUE_G,
BLUE_B
);

}



//==============================
// 流水
//==============================

void flowingWater()
{


ledState="流水灯";


float progress =
(float)(millis()-effectStart)
/FLOW_TIME;



if(progress>1)
progress=1;



float position =
progress*
(
LED_PER_STRIP+
TRAIL_LENGTH-
1
);



for(int s=0;s<STRIP_COUNT;s++)
{


int offset=
s*LED_PER_STRIP;



for(int i=0;i<LED_PER_STRIP;i++)
{


float pos=
position+s*0.8;



float distance=
pos-i;



if(
distance>=0 &&
distance<TRAIL_LENGTH
)
{


float fade=
1.0-
distance/
(float)TRAIL_LENGTH;


fade*=fade;



CRGB c=
blueColor();


c.nscale8(
255*fade
);



leds[offset+i]=c;


}

else
{

leds[offset+i]
.fadeToBlackBy(60);


}



}


}



FastLED.show();


}



//==============================
// 渐亮
//==============================

void fadeInAll()
{


ledState="全灯渐亮";


float p=
(float)
(
millis()
-effectStart
-FLOW_TIME
)
/
FADE_IN_TIME;



if(p>1)p=1;



for(int i=0;i<NUM_LEDS;i++)
{


CRGB c=
blueColor();


c.nscale8(
255*p
);


leds[i]=c;


}



FastLED.show();


}



//==============================
//保持
//==============================

void holdAll()
{


ledState="全亮保持";


for(int i=0;i<NUM_LEDS;i++)
{

leds[i]=blueColor();

}



FastLED.show();


}



//==============================
// LED任务
//==============================

void ledTask()
{


unsigned long t=
millis()-effectStart;



if(t<FLOW_TIME)
{

flowingWater();

}

else if(
t<FLOW_TIME+
FADE_IN_TIME
)
{

fadeInAll();

}

else if(
t<
FLOW_TIME+
FADE_IN_TIME+
HOLD_TIME
)
{

holdAll();

}

else
{

effectStart=millis();

}


}



//==============================
// 主循环
//==============================

void loop()
{


//LED最高优先级

ledTask();


//WiFi后台

wifiTask();


//网页处理

if(wifiOK)
{

server.handleClient();

}



delay(10);


}