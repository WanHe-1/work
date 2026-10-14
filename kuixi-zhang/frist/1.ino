#include <WiFi.h>
#include <WebServer.h>
#include <esp_wifi.h>

const char* AP_SSID = "ESP";
const char* AP_PASSWORD = "12345678";

unsigned long lastStatusPrint = 0;

// ESP32-C3 SuperMini 上尽量使用 ADC 可用且相对安全的引脚
const int PRESSURE_PIN = 3;
const int EMG_PIN = 2;

WebServer server(80);

int pressureRawValue = 0;
float pressureGrams = 0.0;
int emgValue = 0;

const int PRESSURE_RAW_NO_LOAD = 3800;
const int PRESSURE_RAW_FULL_SCALE = 1200;
const float PRESSURE_MAX_GRAMS = 1000.0;

int readAverage(int pin, int samples = 10) {
  long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(pin);
    delay(5);
  }
  return sum / samples;
}

float rawToGrams(int raw) {
  int rawMin = min(PRESSURE_RAW_NO_LOAD, PRESSURE_RAW_FULL_SCALE);
  int rawMax = max(PRESSURE_RAW_NO_LOAD, PRESSURE_RAW_FULL_SCALE);
  int clamped = constrain(raw, rawMin, rawMax);

  if (PRESSURE_RAW_NO_LOAD == PRESSURE_RAW_FULL_SCALE) {
    return 0.0;
  }

  float ratio = 0.0;
  if (PRESSURE_RAW_NO_LOAD > PRESSURE_RAW_FULL_SCALE) {
    ratio = (float)(PRESSURE_RAW_NO_LOAD - clamped) / (float)(PRESSURE_RAW_NO_LOAD - PRESSURE_RAW_FULL_SCALE);
  } else {
    ratio = (float)(clamped - PRESSURE_RAW_NO_LOAD) / (float)(PRESSURE_RAW_FULL_SCALE - PRESSURE_RAW_NO_LOAD);
  }

  ratio = constrain(ratio, 0.0f, 1.0f);
  return ratio * PRESSURE_MAX_GRAMS;
}

String buildPage(int pRaw, float grams, int emg) {
  String page = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset='utf-8'>
  <meta name='viewport' content='width=device-width, initial-scale=1'>
  <meta http-equiv='refresh' content='1'>
  <title>ESP Sensor Monitor</title>
  <style>
    body {
      margin: 0;
      font-family: Arial, Helvetica, sans-serif;
      background: linear-gradient(135deg, #0f172a, #1e293b);
      color: #e2e8f0;
    }
    .wrap {
      max-width: 760px;
      margin: 0 auto;
      padding: 24px;
    }
    .card {
      background: rgba(15, 23, 42, 0.85);
      border: 1px solid rgba(148, 163, 184, 0.25);
      border-radius: 20px;
      padding: 24px;
      box-shadow: 0 16px 40px rgba(0, 0, 0, 0.25);
      backdrop-filter: blur(10px);
    }
    h1 {
      margin: 0 0 12px;
      font-size: 28px;
    }
    p {
      margin: 8px 0;
      color: #cbd5e1;
      line-height: 1.6;
    }
    .grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
      gap: 16px;
      margin-top: 20px;
    }
    .box {
      background: rgba(30, 41, 59, 0.9);
      border-radius: 16px;
      padding: 18px;
      border: 1px solid rgba(148, 163, 184, 0.18);
    }
    .label {
      font-size: 14px;
      color: #94a3b8;
      margin-bottom: 10px;
    }
    .value {
      font-size: 38px;
      font-weight: 700;
      color: #38bdf8;
      word-break: break-all;
    }
    .unit {
      font-size: 18px;
      color: #7dd3fc;
      margin-left: 6px;
      font-weight: 600;
    }
    .small {
      margin-top: 14px;
      font-size: 13px;
      color: #94a3b8;
    }
    .status {
      display: inline-block;
      margin-top: 12px;
      padding: 8px 12px;
      border-radius: 999px;
      background: rgba(14, 165, 233, 0.16);
      color: #7dd3fc;
      font-size: 13px;
    }
  </style>
</head>
<body>
  <div class='wrap'>
    <div class='card'>
      <h1>ESP 传感器数据监测</h1>
      <p>请先连接热点 <b>ESP</b>，然后在浏览器打开 <b>192.168.4.1</b> 查看数据。</p>
      <div class='status'>热点已启动</div>
      <div class='grid'>
        <div class='box'>
          <div class='label'>当前重量</div>
          <div class='value'>{{GRAMS}}<span class='unit'>g</span></div>
        </div>
        <div class='box'>
          <div class='label'>压力传感器原始值</div>
          <div class='value'>{{PRAW}}</div>
        </div>
        <div class='box'>
          <div class='label'>肌电传感器原始值</div>
          <div class='value'>{{EMG}}</div>
        </div>
      </div>
      <div class='small'>页面每 1 秒自动刷新一次。</div>
    </div>
  </div>
</body>
</html>
)rawliteral";
  page.replace("{{GRAMS}}", String(grams, 1));
  page.replace("{{PRAW}}", String(pRaw));
  page.replace("{{EMG}}", String(emg));
  return page;
}

void handleRoot() {
  server.send(200, "text/html; charset=utf-8", buildPage(pressureRawValue, pressureGrams, emgValue));
}


void setup() {
  Serial.begin(115200);
  delay(2500);

  analogReadResolution(12);
  analogSetPinAttenuation(PRESSURE_PIN, ADC_11db);
  analogSetPinAttenuation(EMG_PIN, ADC_11db);

  WiFi.mode(WIFI_OFF);
  delay(1000);

  WiFi.mode(WIFI_AP);
  delay(1000);

  esp_wifi_set_max_tx_power(78);
  delay(500);

  bool ok = WiFi.softAP(AP_SSID, AP_PASSWORD);
  delay(200);

  WiFi.setSleep(false);
  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_set_max_tx_power(40);
  delay(200);

  Serial.println();
  Serial.println("ESP32 AP started");
  Serial.print("softAP result: ");
  Serial.println(ok ? "OK" : "FAIL");
  Serial.print("SSID: ");
  Serial.println(AP_SSID);
  Serial.print("Password: ");
  Serial.println(AP_PASSWORD);
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());
  Serial.print("MAC: ");
  Serial.println(WiFi.softAPmacAddress());

  server.on("/", handleRoot);
  server.onNotFound(handleRoot);
  server.begin();
}

void loop() {
  pressureRawValue = readAverage(PRESSURE_PIN);
  pressureGrams = rawToGrams(pressureRawValue);
  emgValue = readAverage(EMG_PIN);

  Serial.print("pressure = ");
  Serial.print(pressureGrams, 1);
  Serial.print(" g");
  Serial.print("\traw = ");
  Serial.print(pressureRawValue);
  Serial.print("\temg = ");
  Serial.print(emgValue);

  Serial.print("\tAP clients: ");
  Serial.println(WiFi.softAPgetStationNum());

  server.handleClient();
  delay(100);

  if (millis() - lastStatusPrint >= 5000) {
    lastStatusPrint = millis();
    Serial.print("[STATUS] uptime = ");
    Serial.print(millis() / 1000);
    Serial.print(" s | mode = ");
    Serial.print((int)WiFi.getMode());
    Serial.print(" | AP clients = ");
    Serial.println(WiFi.softAPgetStationNum());
  }
}
