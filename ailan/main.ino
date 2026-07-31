/*
 * ============================================================
 * ESP8266 WiFi 电机控制 + JSN-SR04T(UART) + TS-300B
 * ============================================================
 *
 * 硬件接线（直连，无需电阻）：
 *
 *   JSN-SR04T(UART)
 *     VCC ────→ ESP8266 3.3V  (实测 3.3V 可工作)
 *     GND ────→ ESP8266 GND
 *     RX  ────→ ESP8266 D2 (GPIO4)  ← SoftwareSerial TX
 *     TX  ────→ ESP8266 D1 (GPIO5)  → SoftwareSerial RX
 *
 *   TS-300B
 *     VCC ────→ ESP8266 VU (5V 输出脚)
 *     OUT ────→ ESP8266 A0
 *     GND ────→ ESP8266 GND
 *
 *   电机
 *     控制信号 ──→ ESP8266 D5 (GPIO14)  通过继电器/三极管驱动
 *
 *   所有模块 GND 共地
 *
 *   ⚠ TS-300B OUT 直连 A0：TS-300B 输出 0~5V，ESP8266 A0 最高 3.3V。
 *     无分压电阻时长期使用有烧毁风险，建议后续加上 10k+20k 分压。
 *     大多数情况下直连可短时间工作，注意监控。
 * ============================================================
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <SoftwareSerial.h>

// ==================== WiFi 配置 ============================
const char* ssid     = "sia";
const char* password = "siahelloword";
// ===========================================================

// ==================== 引脚定义 ==============================
#define PIN_JSN_RX     D2    // GPIO4 → JSN-SR04T RX
#define PIN_JSN_TX     D1    // GPIO5 ← JSN-SR04T TX
#define PIN_MOTOR      D5    // GPIO14 电机控制
#define PIN_TURBID     A0    // TS-300B 模拟输出
// ===========================================================

// 状态变量
bool motorRunning = false;
unsigned long lastBroadcast = 0;
const unsigned long INTERVAL_MS = 300;  // 每 300ms 推送

// 服务器实例
ESP8266WebServer server(80);
WebSocketsServer webSocket(81);

// JSN-SR04T UART 软串口 (9600 baud)
SoftwareSerial jsnSerial(PIN_JSN_TX, PIN_JSN_RX);  // RX, TX

// 传感器缓存
volatile float cachedDistance = -1.0;
volatile float cachedTurbidity = 0.0;

// ============================================================
// JSN-SR04T UART 数据解析
// 协议：0xFF + H_DATA + L_DATA + SUM  (4字节)
// 距离(mm) = H_DATA*256 + L_DATA
// SUM = (0xFF + H_DATA + L_DATA) & 0xFF
// ============================================================
void readJSN() {
  static uint8_t buf[4];
  static uint8_t idx = 0;

  while (jsnSerial.available()) {
    uint8_t b = jsnSerial.read();

    if (idx == 0 && b != 0xFF) {
      continue;  // 等待帧头
    }

    buf[idx++] = b;

    if (idx == 4) {
      idx = 0;
      uint8_t sum = (buf[0] + buf[1] + buf[2]) & 0xFF;
      if (sum == buf[3]) {
        uint16_t mm = ((uint16_t)buf[1] << 8) | buf[2];
        cachedDistance = mm / 10.0;  // 转成 cm
      }
      // 校验失败则丢弃整帧
    }
  }
}

// ============================================================
// TS-300B 浑浊度读取
// ============================================================
void readTurbidity() {
  int raw = analogRead(PIN_TURBID);         // 0~1024
  float v = raw * (3.3 / 1024.0);           // A0 引脚电压

  // TS-300B 典型值（直连时近似）：
  //   清水 ~1.0-1.5V (经分压后) / 直连时约 2.5-3.5V
  //   浑浊水 ~0.5V (分压后) / 直连时约 1.5-2.5V
  // 简单映射：电压越低越浑浊
  float pct = (1.0 - (v / 3.3)) * 100;
  if (pct < 0)   pct = 0;
  if (pct > 100) pct = 100;
  cachedTurbidity = pct;
}

// ============================================================
// WebSocket 事件
// ============================================================
void onWS(uint8_t num, WStype_t type, uint8_t* payload, size_t len) {
  (void)len;
  switch (type) {
    case WStype_DISCONNECTED:
      break;

    case WStype_CONNECTED:
      { String m = "{\"t\":\"init\",\"m\":" + String(motorRunning ? "1" : "0") + "}";
        webSocket.sendTXT(num, m); }
      break;

    case WStype_TEXT:
      { String msg = String((char*)payload);
        if (msg.indexOf("on") > 0)      setMotor(true);
        else if (msg.indexOf("off") > 0) setMotor(false);
        else if (msg.indexOf("toggle") >= 0) setMotor(!motorRunning);
      }
      break;

    default: break;
  }
}

// ============================================================
// 电机
// ============================================================
void setMotor(bool on) {
  motorRunning = on;
  digitalWrite(PIN_MOTOR, on ? HIGH : LOW);
  char buf[32];
  snprintf(buf, sizeof(buf), "{\"t\":\"m\",\"m\":%d}", on ? 1 : 0);
  webSocket.broadcastTXT(buf);
}

// ============================================================
// 推送
// ============================================================
void broadcastData() {
  char buf[96];
  snprintf(buf, sizeof(buf),
           "{\"t\":\"s\",\"d\":%.1f,\"tb\":%.1f}",
           cachedDistance, cachedTurbidity);
  webSocket.broadcastTXT(buf);
}

// ============================================================
// HTTP 主页
// ============================================================
void handleRoot() {
  server.send_P(200, "text/html", PAGE);
}

// ============================================================
// 简易 API
// ============================================================
void handleAPI() {
  if (server.hasArg("m")) {
    setMotor(server.arg("m") == "1");
  }
  char buf[64];
  snprintf(buf, sizeof(buf),
           "{\"d\":%.1f,\"tb\":%.1f,\"m\":%d}",
           cachedDistance, cachedTurbidity, motorRunning ? 1 : 0);
  server.send(200, "application/json", buf);
}

// ============================================================
// 极简网页 (PROGMEM)
// ============================================================
static const char PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0,maximum-scale=1.0,user-scalable=no">
<title>ESP8266 控制器</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;background:#0d1117;color:#c9d1d9;display:flex;justify-content:center;align-items:center;min-height:100vh;padding:16px}
.box{max-width:360px;width:100%;text-align:center}
h1{font-size:18px;font-weight:600;color:#58a6ff;margin-bottom:20px;letter-spacing:1px}
.stat{background:#161b22;border-radius:12px;padding:16px;margin-bottom:12px;border:1px solid #21262d}
.stat-label{font-size:12px;color:#8b949e;margin-bottom:4px}
.stat-val{font-size:32px;font-weight:700}
.stat-val.dist{color:#79c0ff}
.stat-val.turb{color:#7ee787}
.stat-val .unit{font-size:14px;font-weight:400;color:#8b949e}
.stat-tag{display:inline-block;font-size:11px;padding:2px 10px;border-radius:8px;margin-top:4px}
.tag-ok{background:#1b5e20;color:#a5d6a7}
.tag-warn{background:#e65100;color:#ffcc80}
.tag-err{background:#b71c1c;color:#ef9a9a}
.tag-na{background:#21262d;color:#8b949e}
.btn{width:100%;padding:18px;font-size:18px;font-weight:700;border:none;border-radius:50px;cursor:pointer;transition:all .3s;touch-action:manipulation;outline:none;-webkit-tap-highlight-color:transparent;color:#fff}
.btn-off{background:linear-gradient(135deg,#21262d,#161b22);border:2px solid #30363d}
.btn-on{background:linear-gradient(135deg,#238636,#2ea043);border:2px solid #3fb950;box-shadow:0 0 20px rgba(46,160,67,.3)}
.btn:active{transform:scale(.96)}
.status-row{display:flex;justify-content:center;align-items:center;gap:6px;margin-top:8px;font-size:13px;color:#8b949e}
.dot{width:8px;height:8px;border-radius:50%;display:inline-block}
.dot-red{background:#da3633}
.dot-green{background:#3fb950}
#conn{font-size:11px;margin-bottom:12px;padding:6px;border-radius:6px}
.online{color:#3fb950}
.offline{color:#da3633}
</style>
</head>
<body>
<div class="box">
<h1>⚙ 传感器 & 电机</h1>
<div id="conn" class="offline">⏳ 连接中...</div>

<div class="stat">
<div class="stat-label">📏 距离</div>
<div class="stat-val dist" id="d">-- <span class="unit">cm</span></div>
<div><span class="stat-tag tag-na" id="dtag">等待数据</span></div>
</div>

<div class="stat">
<div class="stat-label">💧 浑浊度</div>
<div class="stat-val turb" id="tb">-- <span class="unit">%</span></div>
<div><span class="stat-tag tag-na" id="tbtag">等待数据</span></div>
</div>

<div class="stat" style="padding:20px">
<button class="btn btn-off" id="btn">⏻ 启动电机</button>
<div class="status-row"><span class="dot dot-red" id="dot"></span><span id="st">已停止</span></div>
</div>
</div>

<script>
var $=function(id){return document.getElementById(id)};
var conn=conn,d=d,dtag=dtag,tb=tb,tbtag=tbtag;
var btn=btn,dot=dot,st=st;
var ws=null,rt=null;

function connect(){
  var p=location.protocol==='https:'?'wss:':'ws:';
  ws=new WebSocket(p+'//'+location.hostname+':81');
  ws.onopen=function(){conn.innerHTML='✅ 已连接';conn.className='online';if(rt){clearTimeout(rt);rt=null}};
  ws.onclose=function(){conn.innerHTML='❌ 断开，3s重连';conn.className='offline';if(rt)clearTimeout(rt);rt=setTimeout(connect,3000)};
  ws.onerror=function(){};
  ws.onmessage=function(e){
    try{
      var x=JSON.parse(e.data);
      if(x.t=='s'){
        d.innerHTML=(x.d<0?'---':x.d.toFixed(1))+' <span class="unit">cm</span>';
        dtag.innerHTML=x.d<0?'<span class="stat-tag tag-err">超出范围</span>':'<span class="stat-tag tag-ok">正常</span>';
        tb.innerHTML=x.tb.toFixed(1)+' <span class="unit">%</span>';
        var tg='<span class="stat-tag tag-ok">清澈</span>';
        if(x.tb>=30)tg='<span class="stat-tag tag-warn">一般</span>';
        if(x.tb>=60)tg='<span class="stat-tag tag-err">浑浊</span>';
        tbtag.innerHTML=tg;
      }else if(x.t=='m'||x.t=='init'){
        var on=x.m==1||x.m==true;
        btn.innerHTML=on?'⏻ 停止电机':'⏻ 启动电机';
        btn.className='btn '+(on?'btn-on':'btn-off');
        dot.className='dot '+(on?'dot-green':'dot-red');
        st.textContent=on?'运行中':'已停止';
      }
    }catch(e){}
  };
}
btn.onclick=function(){if(ws&&ws.readyState==1)ws.send('{"cmd":"toggle"}')};
connect();
</script>
</body>
</html>
)rawliteral";

// ============================================================
// Setup
// ============================================================
void setup() {
  Serial.begin(115200);

  pinMode(PIN_MOTOR, OUTPUT);
  digitalWrite(PIN_MOTOR, LOW);

  // JSN-SR04T UART 9600 baud
  jsnSerial.begin(9600);

  // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  int n = 0;
  while (WiFi.status() != WL_CONNECTED && n < 40) {
    delay(500);
    n++;
  }
  if (WiFi.status() != WL_CONNECTED) {
    delay(2000);
    ESP.restart();
  }

  webSocket.begin();
  webSocket.onEvent(onWS);

  server.on("/", handleRoot);
  server.on("/api", handleAPI);
  server.begin();
}

// ============================================================
// Loop
// ============================================================
void loop() {
  server.handleClient();
  webSocket.loop();

  // 持续读取 JSN-SR04T 串口数据
  readJSN();

  // 定期读取 TS-300B 并推送
  unsigned long now = millis();
  if (now - lastBroadcast >= INTERVAL_MS) {
    lastBroadcast = now;
    readTurbidity();
    broadcastData();

    // 串口只输出数据
    Serial.print(cachedDistance, 1);
    Serial.print(",");
    Serial.print(cachedTurbidity, 1);
    Serial.print(",");
    Serial.println(motorRunning ? "1" : "0");
  }
}