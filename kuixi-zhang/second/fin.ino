
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <MAX30105.h>
#include <FastLED.h>

// ===== 引脚与灯带参数 =====
#define SDA_PIN       4       // D2
#define SCL_PIN       5       // D1
#define LED_PIN       14      // D5
#define LED_COUNT     160
#define LED_TYPE      WS2812B
#define COLOR_ORDER   GRB

// ===== IR 触发参数 =====
#define IR_THRESHOLD  10000UL

// ===== 灯带亮度与渐变 =====
#define MAX_BRIGHTNESS 160
#define FADE_STEP      2
#define FADE_INTERVAL  12

// ===== Wi-Fi 热点 =====
const char* AP_SSID = "IR-Monitor";
const char* AP_PASS = "12345678";

// ===== 对象 =====
MAX30105 particleSensor;
ESP8266WebServer server(80);
CRGB leds[LED_COUNT];

// ===== 实时数据 =====
uint32_t irValue = 0;
uint8_t currentBrightness = 0;
uint8_t targetBrightness = 0;

unsigned long lastFadeTime = 0;
unsigned long lastSampleTime = 0;

const CRGB WARM_WHITE = CRGB(255, 180, 100);

// ===== 网页：仅显示 IR 数值 =====
const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<style>
html,body{
  margin:0;
  width:100%;
  height:100%;
  background:#fff;
  color:#111;
  font-family:Arial,sans-serif;
}
body{
  display:flex;
  align-items:center;
  justify-content:center;
}
#ir{
  font-size:clamp(48px,15vw,100px);
  font-weight:400;
  font-variant-numeric:tabular-nums;
}
</style>
</head>
<body>
<div id="ir">0</div>
<script>
async function updateIR(){
  try{
    const response = await fetch('/ir',{cache:'no-store'});
    const value = await response.text();
    document.getElementById('ir').textContent = value;
  }catch(e){}
}
updateIR();
setInterval(updateIR,200);
</script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send_P(200, "text/html; charset=utf-8", webpage);
}

void handleIR() {
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "text/plain; charset=utf-8",
              String(irValue));
}

void setup() {
  Serial.begin(115200);

  // 初始化 I2C
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);

  // 初始化灯带
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(
    leds, LED_COUNT
  );
  FastLED.setBrightness(0);
  FastLED.clear(true);

  // 初始化 MAX30102
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    // 不输出额外串口信息，避免干扰绘图仪
    while (true) {
      delay(1000);
    }
  }

  // 采集原始 IR
  byte ledBrightness = 60;
  byte sampleAverage = 4;
  byte ledMode = 2;
  int sampleRate = 100;
  int pulseWidth = 411;
  int adcRange = 16384;

  particleSensor.setup(
    ledBrightness,
    sampleAverage,
    ledMode,
    sampleRate,
    pulseWidth,
    adcRange
  );

  // 创建 Wi-Fi 热点
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);

  // 注册网页
  server.on("/", HTTP_GET, handleRoot);
  server.on("/ir", HTTP_GET, handleIR);
  server.begin();
}

void loop() {
  server.handleClient();

  // 读取最新 IR 数据
  if (particleSensor.available()) {
    irValue = particleSensor.getIR();
    particleSensor.nextSample();
  }

  // 串口绘图仪：仅输出 IR 数值
  unsigned long now = millis();
  if (now - lastSampleTime >= 20) {
    lastSampleTime = now;
    Serial.println(irValue);
  }

  // 根据阈值设置目标亮度
  targetBrightness =
    (irValue >= IR_THRESHOLD) ? MAX_BRIGHTNESS : 0;

  // 缓慢渐亮 / 渐灭
  if (now - lastFadeTime >= FADE_INTERVAL) {
    lastFadeTime = now;

    if (currentBrightness < targetBrightness) {
      currentBrightness =
        min((int)currentBrightness + FADE_STEP,
            (int)targetBrightness);
    } else if (currentBrightness > targetBrightness) {
      currentBrightness =
        max((int)currentBrightness - FADE_STEP,
            (int)targetBrightness);
    }

    // 只在亮度变化时更新灯带
    static uint8_t previousBrightness = 255;

    if (currentBrightness != previousBrightness) {
      previousBrightness = currentBrightness;

      fill_solid(leds, LED_COUNT, WARM_WHITE);
      FastLED.setBrightness(currentBrightness);
      FastLED.show();
    }
  }

  yield();
}
