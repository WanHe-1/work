#include <WiFi.h>
#include <WebServer.h>

// WiFi 热点
const char* ssid = "ESP32S3-Water";
const char* password = "water123";

// 网页服务器
WebServer server(80);

// 六个传感器的 AO 引脚
const int sensorPins[6] = {0, 4, 16, 5, 18, 19};

// 检测阈值
const int threshold = 100;

// 检测结果
int waterState[6] = {0, 0, 0, 0, 0, 0};

// 简洁网页：仅显示数字
const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<style>
body {
  margin: 0;
  background: #ffffff;
  font-family: Arial, sans-serif;
}
.grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 8px;
  padding: 8px;
}
.num {
  display: flex;
  justify-content: center;
  align-items: center;
  height: 100px;
  font-size: 48px;
  color: #000000;
  background: #f2f2f2;
  border-radius: 8px;
}
</style>
</head>
<body>
<div class="grid">
  <div class="num" id="s0">0</div>
  <div class="num" id="s1">0</div>
  <div class="num" id="s2">0</div>
  <div class="num" id="s3">0</div>
  <div class="num" id="s4">0</div>
  <div class="num" id="s5">0</div>
</div>

<script>
async function update() {
  try {
    const response = await fetch('/data?t=' + Date.now());
    const data = await response.json();

    for (let i = 0; i < 6; i++) {
      document.getElementById('s' + i).textContent = data[i];
    }
  } catch (e) {}
}

update();
setInterval(update, 500);
</script>
</body>
</html>
)rawliteral";

// 读取六个传感器
void readSensors() {
  for (int i = 0; i < 6; i++) {
    int value = analogRead(sensorPins[i]);
    waterState[i] = (value > threshold) ? 1 : 0;
  }
}

// 网页
void handleRoot() {
  server.send_P(200, "text/html; charset=utf-8", webpage);
}

// 返回六个数字
void handleData() {
  readSensors();

  String json = "[";
  for (int i = 0; i < 6; i++) {
    if (i > 0) json += ",";
    json += String(waterState[i]);
  }
  json += "]";

  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);

  analogReadResolution(12);

  for (int i = 0; i < 6; i++) {
    pinMode(sensorPins[i], INPUT);
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleData);

  server.begin();

  Serial.println("192.168.4.1");
}

void loop() {
  server.handleClient();
}