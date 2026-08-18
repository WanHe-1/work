#include <WiFi.h>
#include <WebServer.h>

const char* AP_SSID = "ESP32-Sensor";
const char* AP_PASSWORD = "12345678";

// ESP32-C3 SuperMini 常用 ADC 引脚：GPIO0~GPIO4
// 如果你的板子丝印是 A0/A1，请优先按 A0、A1 接线。
const int PRESSURE_PIN = 0;
const int EMG_PIN = 1;

WebServer server(80);

int pressureRawValue = 0;
float pressureGrams = 0.0;
int emgValue = 0;

// 根据你的压力传感器分压电路进行校准：
// 1. PRESSURE_RAW_NO_LOAD = 没有压力时的原始值
// 2. PRESSURE_RAW_FULL_SCALE = 达到你设定最大重量时的原始值
// 3. PRESSURE_MAX_GRAMS = 该原始值对应的最大重量（克）
const int PRESSURE_RAW_NO_LOAD = 3800;
const int PRESSURE_RAW_FULL_SCALE = 1200;
const float PRESSURE_MAX_GRAMS = 1000.0;

int readAverage(int pin, int samples = 10) {
  long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(pin);
    delay(2);
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

String buildPage() {
  String page = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset='utf-8'>
  <meta name='viewport' content='width=device-width, initial-scale=1'>
  <title>ESP32 Sensor Monitor</title>
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
      <h1>ESP32 传感器数据监测</h1>
      
      <div class='status' id='wifiStatus'>正在连接热点</div>
      <div class='grid'>
        <div class='box'>
          <div class='label'>当前重量</div>
          <div class='value'><span id='weight'>--</span><span class='unit'>g</span></div>
        </div>
        <div class='box'>
          <div class='label'>压力传感器原始值</div>
          <div class='value' id='pressureRaw'>--</div>
        </div>
        <div class='box'>
          <div class='label'>肌电传感器原始值</div>
          <div class='value' id='emg'>--</div>
        </div>
      </div>
    </div>
  </div>
  <script>
    async function updateData() {
      try {
        const res = await fetch('/data');
        const data = await res.json();
        document.getElementById('weight').textContent = Number(data.weight_g).toFixed(1);
        document.getElementById('pressureRaw').textContent = data.pressure_raw;
        document.getElementById('emg').textContent = data.emg;
        document.getElementById('wifiStatus').textContent = '已连接';
      } catch (e) {
        document.getElementById('wifiStatus').textContent = '数据获取失败';
      }
    }
    updateData();
    setInterval(updateData, 300);
  </script>
</body>
</html>
)rawliteral";
  return page;
}

void handleRoot() {
  server.send(200, "text/html; charset=utf-8", buildPage());
}

void handleData() {
  String json = "{";
  json += "\"pressure_raw\":" + String(pressureRawValue) + ",";
  json += "\"weight_g\":" + String(pressureGrams, 1) + ",";
  json += "\"emg\":" + String(emgValue);
  json += "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(12);
  analogSetPinAttenuation(PRESSURE_PIN, ADC_11db);
  analogSetPinAttenuation(EMG_PIN, ADC_11db);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);

  Serial.println();
  Serial.println("ESP32 AP started");
  Serial.print("SSID: ");
  Serial.println(AP_SSID);
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/data", handleData);
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
  Serial.println(emgValue);

  server.handleClient();
  delay(20);
}
