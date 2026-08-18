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
bool wifiOK = false;
unsigned long wifiStart;


//==============================
// LED
//==============================

#define LED_PIN 4
#define LED_PER_STRIP 20
#define STRIP_COUNT 6
#define NUM_LEDS (LED_PER_STRIP * STRIP_COUNT)

CRGB leds[NUM_LEDS];

#define BLUE_R 80
#define BLUE_G 180
#define BLUE_B 255

unsigned long flowDurationMs = 5000;
unsigned long fadeInTimeMs = 300;
unsigned long holdTimeMs = 1000;

#define TRAIL_LENGTH 6
#define BRIGHTNESS 140

unsigned long effectStart;

String ledState = "流水灯";


//==============================
// 工具
//==============================

unsigned long readArgUL(const String& name, unsigned long current, unsigned long minValue, unsigned long maxValue)
{
  if (!server.hasArg(name))
  {
    return current;
  }

  String value = server.arg(name);
  value.trim();

  if (value.length() == 0)
  {
    return current;
  }

  unsigned long parsed = value.toInt();

  if (parsed < minValue)
  {
    parsed = minValue;
  }

  if (parsed > maxValue)
  {
    parsed = maxValue;
  }

  return parsed;
}

void resetEffect()
{
  effectStart = millis();
}

String buildRootPage()
{
  String html;

  html += "<html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<style>";
  html += "body{margin:0;font-family:Arial,'Microsoft YaHei',sans-serif;background:#0f172a;color:#e2e8f0;}";
  html += ".wrap{max-width:920px;margin:0 auto;padding:20px;}";
  html += ".card{background:#111827;border:1px solid #1f2937;border-radius:16px;padding:18px;margin-bottom:16px;box-shadow:0 10px 30px rgba(0,0,0,.18);}";
  html += ".title{font-size:24px;font-weight:700;margin-bottom:10px;color:#f8fafc;}";
  html += ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:12px;}";
  html += ".item{background:#0b1220;border:1px solid #1f2937;border-radius:12px;padding:12px;line-height:1.7;}";
  html += ".label{color:#94a3b8;font-size:13px;}";
  html += ".value{color:#f8fafc;font-size:18px;font-weight:600;word-break:break-all;}";
  html += ".muted{color:#94a3b8;font-size:13px;}";
  html += ".row{display:flex;gap:12px;flex-wrap:wrap;}";
  html += "form{display:grid;gap:12px;}";
  html += "input{width:100%;box-sizing:border-box;padding:12px 14px;border-radius:10px;border:1px solid #334155;background:#0b1220;color:#e2e8f0;font-size:16px;}";
  html += "button{border:0;border-radius:10px;padding:12px 16px;background:linear-gradient(90deg,#2563eb,#06b6d4);color:#fff;font-size:16px;font-weight:700;cursor:pointer;}";
  html += ".btnline{display:flex;gap:10px;flex-wrap:wrap;}";
  html += ".hint{font-size:12px;color:#94a3b8;}";
  html += "</style></head><body><div class='wrap'>";

  html += "<div class='card'><div class='title'>流水灯参数</div>";
  html += "<div class='grid'>";

  html += "<div class='item'><div class='label'>流水时长</div><div class='value'>";

  html += "<div class='item'><div class='label'>渐亮时长</div><div class='value'>";
  html += String(fadeInTimeMs);
  html += " ms</div></div>";

  html += "<div class='item'><div class='label'>保持时长</div><div class='value'>";
  html += String(holdTimeMs);
  html += " ms</div><div class='muted'>流水结束后进入全亮保持的时间</div></div>";

  html += "</div></div>";

  html += "<div class='card'><div class='title'>调整流水灯</div>";
  html += "<form action='/set' method='get'>";
  html += "<div><div class='label'>流水时长（毫秒）</div>";
  html += "<input type='number' min='1000' max='60000' name='flowDuration' value='" + String(flowDurationMs) + "'></div>";
  html += "<div><div class='label'>保持时长（毫秒）</div>";
  html += "<input type='number' min='0' max='60000' name='holdTime' value='" + String(holdTimeMs) + "'></div>";
  html += "<div class='btnline'><button type='submit'>保存并应用</button></div>";
  html += "<div class='hint'>说明：流水时长表示完整跑一遍流水灯的时间，保持时长表示全亮停留的时间。</div>";
  html += "</form></div>";

  html += "</div></body></html>";

  return html;
}

void handleRoot()
{
  server.send(200, "text/html", buildRootPage());
}

void handleSet()
{
  flowDurationMs = readArgUL("flowDuration", flowDurationMs, 1000, 60000);
  holdTimeMs = readArgUL("holdTime", holdTimeMs, 0, 60000);

  resetEffect();

  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "ok");
}


//==============================
// 初始化
//==============================

void setup()
{
  Serial.begin(115200);

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  resetEffect();

  //==============================
  // WiFi非阻塞连接
  //==============================

  WiFi.begin(ssid, password);
  wifiStart = millis();

  //网页
  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.begin();
}


//==============================
// WiFi后台任务
//==============================

void wifiTask()
{
  if (wifiOK)
  {
    return;
  }

  //最多等待8秒
  if (WiFi.status() == WL_CONNECTED)
  {
    wifiOK = true;
    Serial.println("WiFi OK");
    Serial.println(WiFi.localIP());
  }
  else if (millis() - wifiStart > 8000)
  {
    //放弃连接
    WiFi.disconnect();
    Serial.println("WiFi timeout");
  }
}


//==============================
// 蓝色
//==============================

CRGB blueColor()
{
  return CRGB(BLUE_R, BLUE_G, BLUE_B);
}


//==============================
// 流水
//==============================

void flowingWater()
{
  ledState = "流水灯";

  unsigned long elapsed = millis() - effectStart;
  float travel = (float)(LED_PER_STRIP + TRAIL_LENGTH - 1);
  float position = (float)elapsed / (float)flowDurationMs * travel;

  while (position >= travel)
  {
    position -= travel;
  }

  for (int s = 0; s < STRIP_COUNT; s++)
  {
    int offset = s * LED_PER_STRIP;

    for (int i = 0; i < LED_PER_STRIP; i++)
    {
      float pos = position + s * 0.8f;
      float distance = pos - i;

      if (distance >= 0 && distance < TRAIL_LENGTH)
      {
        float fade = 1.0f - distance / (float)TRAIL_LENGTH;
        fade *= fade;

        CRGB c = blueColor();
        c.nscale8(255 * fade);
        leds[offset + i] = c;
      }
      else
      {
        leds[offset + i].fadeToBlackBy(60);
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
  ledState = "全灯渐亮";

  float p = (float)(millis() - effectStart - flowDurationMs) / (float)fadeInTimeMs;

  if (p > 1)
  {
    p = 1;
  }

  for (int i = 0; i < NUM_LEDS; i++)
  {
    CRGB c = blueColor();
    c.nscale8(255 * p);
    leds[i] = c;
  }

  FastLED.show();
}


//==============================
//保持
//==============================

void holdAll()
{
  ledState = "全亮保持";

  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = blueColor();
  }

  FastLED.show();
}


//==============================
// LED任务
//==============================

void ledTask()
{
  unsigned long t = millis() - effectStart;

  if (t < flowDurationMs)
  {
    flowingWater();
  }
  else if (t < flowDurationMs + fadeInTimeMs)
  {
    fadeInAll();
  }
  else if (t < flowDurationMs + fadeInTimeMs + holdTimeMs)
  {
    holdAll();
  }
  else
  {
    resetEffect();
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
  if (wifiOK)
  {
    server.handleClient();
  }

  delay(10);
}
