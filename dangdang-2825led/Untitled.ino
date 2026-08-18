#include <ESP8266WiFi.h>
    #include <ESP8266WebServer.h>
    #include <FastLED.h>
    
    
    // =====================================================
    // WiFi 设置
    // =====================================================
    
    const char* ssid = "WanHe'phone";
    const char* password = "000000qq";
    
    ESP8266WebServer server(80);
    
    unsigned long wifiStart;
    bool apStarted = false;
    
    const char* apSsid = "ESP8266-LED";
    const char* apPassword = "dangdang";
    
    
    // =====================================================
    // LED 设置
    // =====================================================
    
    #define LED_PIN_4 4               // D2 = GPIO4
    #define LED_PIN_5 5               // D1 = GPIO5
    #define LED_PER_STRIP 20          // 每条20颗
    #define STRIP_COUNT 6             // 6条
    #define NUM_LEDS (LED_PER_STRIP * STRIP_COUNT)
    
    CRGB leds[NUM_LEDS];
    
    
    // =====================================================
    // 淡蓝色
    // =====================================================
    
    #define BLUE_R 80
    #define BLUE_G 180
    #define BLUE_B 255
    
    
    // =====================================================
    // PWM亮度
    // =====================================================
    
    // 流水灯最大PWM
    // Web端可修改：1 ~ 255
    uint8_t progressBrightness = 127;
    
    // 最终全亮PWM
    const uint8_t HOLD_BRIGHTNESS = 255;
    
    
    // =====================================================
    // 时间参数
    // =====================================================
    
    // 流水灯时间
    unsigned long progressTimeMs = 4000;
    
    // PWM最大值 → 255渐亮时间
    // Web端可修改：1 ~ 5000ms
    unsigned long brightnessFadeTimeMs = 200;
    
    // PWM255常亮时间
    unsigned long holdTimeMs = 700;

    // PWM255 → 0呼吸时间
    unsigned long breatheTimeMs = 300;
    
    // 全部熄灭后等待时间
    unsigned long offTimeMs = 200;
    
    
    // =====================================================
    // 状态
    // =====================================================
    
    enum EffectState
    {
      PROGRESS,       // 流水灯
      BRIGHTNESS_UP,  // 最大PWM -> 255
      BREATHE_DOWN,   // PWM255 -> 0呼吸
      OFF             // 全部熄灭
    };
    
    EffectState effectState = PROGRESS;
    
    
    // =====================================================
    // 当前阶段开始时间
    // =====================================================
    
    unsigned long effectStart;
    
    
    // =====================================================
    // 蓝色
    // =====================================================
    
    CRGB blueColor()
    {
      return CRGB(
        BLUE_R,
        BLUE_G,
        BLUE_B
      );
    }
    
    
    // =====================================================
    // 重置效果
    // =====================================================
    
    void resetEffect()
    {
      effectStart = millis();
    
      effectState = PROGRESS;
    
      FastLED.clear();
      FastLED.show();
    }
    
    
    // =====================================================
    // 网页参数读取
    // =====================================================
    
    unsigned long readArgUL(
      const String& name,
      unsigned long current,
      unsigned long minValue,
      unsigned long maxValue
    )
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
    
    
    // =====================================================
    // Web页面
    // =====================================================
    
    String buildRootPage()
    {
      String html;
    
      html.reserve(7000);
    
    
      // ===================================================
      // HTML
      // ===================================================
    
      html += "<!DOCTYPE html>";
      html += "<html>";
    
      html += "<head>";
    
      html += "<meta charset='utf-8'>";
    
      html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
    
      html += "<title>ESP8266 流水灯</title>";
    
    
      // ===================================================
      // CSS
      // ===================================================
    
      html += "<style>";
    
      html += "body{";
      html += "margin:0;";
      html += "font-family:Arial,'Microsoft YaHei',sans-serif;";
      html += "background:#0f172a;";
      html += "color:#e2e8f0;";
      html += "}";
    
    
      html += ".wrap{";
      html += "max-width:920px;";
      html += "margin:0 auto;";
      html += "padding:20px;";
      html += "}";
    
    
      html += ".card{";
      html += "background:#111827;";
      html += "border:1px solid #1f2937;";
      html += "border-radius:16px;";
      html += "padding:18px;";
      html += "margin-bottom:16px;";
      html += "box-shadow:0 10px 30px rgba(0,0,0,.18);";
      html += "}";
    
    
      html += ".title{";
      html += "font-size:24px;";
      html += "font-weight:700;";
      html += "margin-bottom:15px;";
      html += "color:#f8fafc;";
      html += "}";
    
    
      html += ".grid{";
      html += "display:grid;";
      html += "grid-template-columns:repeat(auto-fit,minmax(220px,1fr));";
      html += "gap:12px;";
      html += "}";
    
    
      html += ".item{";
      html += "background:#0b1220;";
      html += "border:1px solid #1f2937;";
      html += "border-radius:12px;";
      html += "padding:12px;";
      html += "line-height:1.7;";
      html += "}";
    
    
      html += ".label{";
      html += "color:#94a3b8;";
      html += "font-size:13px;";
      html += "}";
    
    
      html += ".value{";
      html += "color:#f8fafc;";
      html += "font-size:18px;";
      html += "font-weight:600;";
      html += "}";
    
    
      html += ".muted{";
      html += "color:#94a3b8;";
      html += "font-size:13px;";
      html += "}";
    
    
      html += "form{";
      html += "display:grid;";
      html += "gap:12px;";
      html += "}";
    
    
      html += "input{";
      html += "width:100%;";
      html += "box-sizing:border-box;";
      html += "padding:12px 14px;";
      html += "border-radius:10px;";
      html += "border:1px solid #334155;";
      html += "background:#0b1220;";
      html += "color:#e2e8f0;";
      html += "font-size:16px;";
      html += "}";
    
    
      html += "button{";
      html += "border:0;";
      html += "border-radius:10px;";
      html += "padding:12px 16px;";
      html += "background:linear-gradient(90deg,#2563eb,#06b6d4);";
      html += "color:#fff;";
      html += "font-size:16px;";
      html += "font-weight:700;";
      html += "cursor:pointer;";
      html += "}";
    
    
      html += "</style>";
    
      html += "</head>";
    
    
      // ===================================================
      // BODY
      // ===================================================
    
      html += "<body>";
    
      html += "<div class='wrap'>";
    
    
      // ===================================================
      // 流水灯参数
      // ===================================================
    
      html += "<div class='card'>";
    
      html += "<div class='title'>Value</div>";
    
      html += "<div class='grid'>";
    
    
      // ---------------------------------------------------
      // 流水灯时间
      // ---------------------------------------------------
    
      html += "<div class='item'>";
    
      html += "<div class='label'>Led Time</div>";
    
      html += "<div class='value'>";
    
      html += String(progressTimeMs);
    
      html += " ms</div>";
    
      html += "</div>";
    
    
      // ---------------------------------------------------
      // 流水灯最大PWM
      // ---------------------------------------------------
    
      html += "<div class='item'>";
    
      html += "<div class='label'>Max PWM</div>";
    
      html += "<div class='value'>";
    
      html += String(progressBrightness);
    
      html += "</div>";
    
      html += "</div>";
    
    
      // ---------------------------------------------------
      // 渐亮时间
      // ---------------------------------------------------
    
      html += "<div class='item'>";
    
      html += "<div class='label'>";
    
      html += String(progressBrightness);
    
      html += " → 255(time ms)</div>";
    
      html += "<div class='value'>";
    
      html += String(brightnessFadeTimeMs);
    
      html += " ms</div>";
    
      html += "</div>";
    
    
      // ---------------------------------------------------
      // PWM255常亮
      // ---------------------------------------------------

      html += "<div class='item'>";

      html += "<div class='label'>Hold Time</div>";

      html += "<div class='value'>";

      html += String(holdTimeMs);

      html += " ms</div>";

      html += "</div>";


      // ---------------------------------------------------
      // PWM255 → 0呼吸
      // ---------------------------------------------------

      html += "<div class='item'>";

      html += "<div class='label'>Breathe Time</div>";

      html += "<div class='value'>";

      html += String(breatheTimeMs);

      html += " ms</div>";

      html += "</div>";


      // ---------------------------------------------------
      // 熄灭等待
      // ---------------------------------------------------

      html += "<div class='item'>";

      html += "<div class='label'>OFF Time</div>";

      html += "<div class='value'>";

      html += String(offTimeMs);

      html += " ms</div>";

      html += "</div>";
    
    
      html += "</div>";
    
      html += "</div>";
    
    
      // ===================================================
      // 调整流水灯
      // ===================================================
    
      html += "<div class='card'>";
    
      html += "<div class='title'>Revise</div>";
    
      html += "<form action='/set' method='get'>";
    
    
      // ---------------------------------------------------
      // 流水灯时间
      // ---------------------------------------------------
    
      html += "<div>";
    
      html += "<div class='label'>Time(ms)</div>";
    
      html += "<input type='number' ";
    
      html += "min='1000' ";
    
      html += "max='60000' ";
    
      html += "name='progressTime' ";
    
      html += "value='";
    
      html += String(progressTimeMs);
    
      html += "'>";
    
      html += "</div>";
    
    
      // ---------------------------------------------------
      // 进度条最大PWM
      // ---------------------------------------------------
    
      html += "<div>";
    
      html += "<div class='label'>PWM MAX（1～255）</div>";
    
      html += "<input type='number' ";
    
      html += "min='1' ";
    
      html += "max='255' ";
    
      html += "name='progressBrightness' ";
    
      html += "value='";
    
      html += String(progressBrightness);
    
      html += "'>";
    
      html += "</div>";
    
    
      // ---------------------------------------------------
      // PWM最大值 → 255渐亮时间
      // ---------------------------------------------------
    
      html += "<div>";
    
      html += "<div class='label'>";
    
      html += String(progressBrightness);
    
      html += " → 255(time ms)</div>";
    
      html += "<input type='number' ";
    
      html += "min='1' ";
    
      html += "max='5000' ";
    
      html += "name='brightnessFadeTime' ";
    
      html += "value='";
    
      html += String(brightnessFadeTimeMs);
    
      html += "'>";
    
      html += "</div>";
    
    
      // ---------------------------------------------------
      // PWM255 → 0呼吸时间
      // ---------------------------------------------------

      html += "<div>";

      html += "<div class='label'>Breathe time</div>";

      html += "<input type='number' ";

      html += "min='1' ";

      html += "max='60000' ";

      html += "name='breatheTime' ";

      html += "value='";

      html += String(breatheTimeMs);

      html += "'>";

      html += "</div>";


      // ---------------------------------------------------
      // 熄灭等待时间
      // ---------------------------------------------------

      html += "<div>";

      html += "<div class='label'>OFF time</div>";

      html += "<input type='number' ";

      html += "min='0' ";

      html += "max='60000' ";

      html += "name='offTime' ";

      html += "value='";

      html += String(offTimeMs);

      html += "'>";

      html += "</div>";
    
    
      // ---------------------------------------------------
      // 保存
      // ---------------------------------------------------
    
      html += "<button type='submit'>upload</button>";
    
    
      html += "<div class='muted'>";
    
      html += "</div>";
    
    
      html += "</form>";
    
      html += "</div>";
    
    
      // ===================================================
      // 页面结束
      // ===================================================
    
      html += "</div>";
    
      html += "</body>";
    
      html += "</html>";
    
    
      return html;
    }
    
    
    // =====================================================
    // 首页
    // =====================================================
    
    void handleRoot()
    {
      server.send(
        200,
        "text/html; charset=utf-8",
        buildRootPage()
      );
    }
    
    
    // =====================================================
    // 修改参数
    // =====================================================
    
    void handleSet()
    {
      // ---------------------------------------------------
      // 进度条时间
      // ---------------------------------------------------
    
      progressTimeMs =
        readArgUL(
          "progressTime",
          progressTimeMs,
          1000,
          60000
        );
    
    
      // ---------------------------------------------------
      // 进度条最大PWM
      // ---------------------------------------------------
    
      progressBrightness =
        (uint8_t)readArgUL(
          "progressBrightness",
          progressBrightness,
          1,
          255
        );
    
    
      // ---------------------------------------------------
      // 最大PWM → 255渐亮时间
      // ---------------------------------------------------
    
      brightnessFadeTimeMs =
        readArgUL(
          "brightnessFadeTime",
          brightnessFadeTimeMs,
          1,
          5000
        );
    
    
      // ---------------------------------------------------
      // PWM255 → 0呼吸时间
      // ---------------------------------------------------

      breatheTimeMs =
        readArgUL(
          "breatheTime",
          breatheTimeMs,
          1,
          60000
        );


      // ---------------------------------------------------
      // 熄灭等待时间
      // ---------------------------------------------------

      offTimeMs =
        readArgUL(
          "offTime",
          offTimeMs,
          0,
          60000
        );
    
    
      // ---------------------------------------------------
      // 应用参数后重新开始
      // ---------------------------------------------------
    
      resetEffect();
    
    
      // ---------------------------------------------------
      // 返回首页
      // ---------------------------------------------------
    
      server.sendHeader(
        "Location",
        "/"
      );
    
    
      server.send(
        302,
        "text/plain",
        "OK"
      );
    }
    
    
    // =====================================================
    // 启动热点
    // =====================================================
    
    void startAPMode()
    {
      if (apStarted)
      {
        return;
      }
    
      WiFi.disconnect();
      WiFi.mode(WIFI_AP_STA);
    
      bool ok = WiFi.softAP(apSsid, apPassword);
    
      apStarted = ok;
    
      Serial.println();
      Serial.println("==============================");
      Serial.println("WiFi connect timeout");
    
      if (ok)
      {
        Serial.println("AP started successfully.");
        Serial.print("AP SSID: ");
        Serial.println(apSsid);
        Serial.print("AP Password: ");
        Serial.println(apPassword);
        Serial.print("AP IP: ");
        Serial.println(WiFi.softAPIP());
      }
      else
      {
        Serial.println("AP start failed.");
      }
    
      Serial.println("LED continues running.");
      Serial.println("==============================");
    }
    
    
    // =====================================================
    // WiFi后台连接
    // =====================================================
    
    void wifiTask()
    {
      if (WiFi.status() == WL_CONNECTED)
      {
        return;
      }
    
      if (!apStarted && millis() - wifiStart >= 5000)
      {
        startAPMode();
      }
    }
    
    
    // =====================================================
    // 6条灯带同步进度条
    // =====================================================
    
    void progressFill()
    {
      unsigned long elapsed =
        millis() - effectStart;
    
    
      // ===================================================
      // 计算流水进度
      // ===================================================
    
      float progress =
        (float)elapsed /
        (float)progressTimeMs;
    
    
      if (progress < 0.0f)
      {
        progress = 0.0f;
      }
    
    
      if (progress > 1.0f)
      {
        progress = 1.0f;
      }
    
    
      // ===================================================
      // 计算当前灯珠位置
      // ===================================================
    
      float exactPosition =
        progress *
        (float)LED_PER_STRIP;
    
    
      int fullCount =
        (int)exactPosition;
    
    
      // ===================================================
      // 六条灯带完全同步
      // ===================================================
    
      for (int s = 0; s < STRIP_COUNT; s++)
      {
        int offset =
          s * LED_PER_STRIP;
    
    
        // =================================================
        // 已经经过的灯珠
        // PWM = 当前设定最大PWM
        // =================================================
    
        for (int i = 0; i < LED_PER_STRIP; i++)
        {
          if (i < fullCount)
          {
            CRGB c =
              blueColor();
    
    
            c.nscale8(
              progressBrightness
            );
    
    
            leds[offset + i] =
              c;
          }
          else
          {
            leds[offset + i] =
              CRGB::Black;
          }
        }
    
    
        // =================================================
        // 当前正在经过的灯珠
        // PWM 0 → 最大PWM
        // =================================================
    
        if (fullCount < LED_PER_STRIP)
        {
          float partial =
            exactPosition -
            (float)fullCount;
    
    
          if (partial > 0.0f)
          {
            CRGB c =
              blueColor();
    
    
            uint8_t brightness =
              (uint8_t)(
                partial *
                progressBrightness
              );
    
    
            c.nscale8(
              brightness
            );
    
    
            leds[
              offset + fullCount
            ] = c;
          }
        }
      }
    
    
      FastLED.show();
    }
    
    
    // =====================================================
    // 全灯 当前最大PWM → PWM255
    // =====================================================
    
    void brightnessFadeUp()
    {
      unsigned long elapsed =
        millis() - effectStart;
    
    
      // ===================================================
      // 计算渐变进度
      // ===================================================
    
      float progress =
        (float)elapsed /
        (float)brightnessFadeTimeMs;
    
    
      if (progress < 0.0f)
      {
        progress = 0.0f;
      }
    
    
      if (progress > 1.0f)
      {
        progress = 1.0f;
      }
    
    
      // ===================================================
      // 当前最大PWM → PWM255
      // ===================================================
    
      float brightness =
        (float)progressBrightness +
        (
          (float)HOLD_BRIGHTNESS -
          (float)progressBrightness
        ) *
        progress;
    
    
      uint8_t pwm =
        (uint8_t)brightness;
    
    
      // ===================================================
      // 所有灯同步渐亮
      // ===================================================
    
      for (int i = 0; i < NUM_LEDS; i++)
      {
        CRGB c =
          blueColor();
    
    
        c.nscale8(
          pwm
        );
    
    
        leds[i] =
          c;
      }
    
    
      FastLED.show();
    }
    
    
    // =====================================================
    // PWM255 → 0呼吸渐灭
    // =====================================================

    void breatheDown()
    {
      unsigned long elapsed =
        millis() - effectStart;

      float progress =
        (float)elapsed /
        (float)breatheTimeMs;

      if (progress < 0.0f)
      {
        progress = 0.0f;
      }

      if (progress > 1.0f)
      {
        progress = 1.0f;
      }

      float brightness =
        (float)HOLD_BRIGHTNESS -
        (float)HOLD_BRIGHTNESS *
        progress;

      uint8_t pwm =
        (uint8_t)brightness;

      for (int i = 0; i < NUM_LEDS; i++)
      {
        CRGB c =
          blueColor();

        c.nscale8(
          pwm
        );

        leds[i] =
          c;
      }

      FastLED.show();
    }
    
    
    // =====================================================
    // 全部熄灭
    // =====================================================
    
    void turnOff()
    {
      FastLED.clear();
    
      FastLED.show();
    }
    
    
    // =====================================================
    // LED任务
    // =====================================================
    
    void ledTask()
    {
      unsigned long t =
        millis() - effectStart;
    
    
      switch (effectState)
      {
    
        // =================================================
        // 流水灯
        // =================================================
    
        case PROGRESS:
    
          progressFill();
    
    
          if (t >= progressTimeMs)
          {
            // 进入 最大PWM → 255渐亮
            effectState =
              BRIGHTNESS_UP;
    
    
            effectStart =
              millis();
          }
    
          break;
    
    
        // =================================================
        // 最大PWM → PWM255
        // =================================================
    
        case BRIGHTNESS_UP:
    
          brightnessFadeUp();
    
    
          if (t >= brightnessFadeTimeMs)
          {
            // 确保最终达到PWM255
    
            for (int i = 0; i < NUM_LEDS; i++)
            {
              CRGB c =
                blueColor();
    
    
              c.nscale8(
                HOLD_BRIGHTNESS
              );
    
    
              leds[i] =
                c;
            }
    
    
            FastLED.show();
    
    
            // 进入PWM255 -> 0呼吸
            effectState =
              BREATHE_DOWN;
    
    
            effectStart =
              millis();
          }
    
          break;
    
    
        // =================================================
        // PWM255 -> 0呼吸
        // =================================================
    
        case BREATHE_DOWN:
    
          breatheDown();
    
    
          if (t >= breatheTimeMs)
          {
            effectState =
              OFF;
    
    
            effectStart =
              millis();
          }
    
          break;
    
    
        // =================================================
        // 全部熄灭
        // =================================================
    
        case OFF:
    
          turnOff();
    
    
          if (t >= offTimeMs)
          {
            effectState =
              PROGRESS;
    
    
            effectStart =
              millis();
    
    
            FastLED.clear();
    
            FastLED.show();
          }
    
          break;
      }
    }
    
    
    // =====================================================
    // setup
    // =====================================================
    
    void setup()
    {
      Serial.begin(115200);
    
    
      delay(100);
    
    
      // ===================================================
      // FastLED初始化
      // ===================================================
    
      FastLED.addLeds<
        WS2812B,
        LED_PIN_4,
        GRB
      >(
        leds,
        NUM_LEDS
      );

      FastLED.addLeds<
        WS2812B,
        LED_PIN_5,
        GRB
      >(
        leds,
        NUM_LEDS
      );
    
    
      FastLED.clear();
    
      FastLED.show();
    
    
      // ===================================================
      // 启动LED效果
      // ===================================================
    
      resetEffect();
    
    
      // ===================================================
      // WiFi
      // ===================================================
    
      WiFi.mode(
        WIFI_STA
      );
    
    
      WiFi.begin(
        ssid,
        password
      );
    
    
      WiFi.setAutoReconnect(true);
    
    
      wifiStart =
        millis();
    
    
      // ===================================================
      // Web服务器
      // ===================================================
    
      server.on(
        "/",
        handleRoot
      );
    
    
      server.on(
        "/set",
        handleSet
      );
    
    
      server.begin();
    
    
      // ===================================================
      // 串口信息
      // ===================================================
    
      Serial.println();
    
      Serial.println("==============================");
    
      Serial.println("ESP8266 LED Controller");
    
      Serial.println("------------------------------");
    
      Serial.println("LED Pin: D2 / GPIO4, D1 / GPIO5");
    
      Serial.println("Strip Count: 6");
    
      Serial.println("LED Per Strip: 20");
    
      Serial.println("Total LED: 120");
    
      Serial.println("Mode: Synchronized");
      Serial.println("AP fallback: 5s timeout");
      Serial.print("AP SSID: ");
      Serial.println(apSsid);
    
      Serial.print("Progress PWM: ");
      Serial.println(progressBrightness);
    
      Serial.print("Fade PWM: ");
      Serial.print(progressBrightness);
      Serial.println(" -> 255");
    
      Serial.print("Fade Time: ");
      Serial.print(brightnessFadeTimeMs);
      Serial.println(" ms");
    
      Serial.println("Breathe PWM: 255 -> 0");

      Serial.print("Breathe Time: ");
      Serial.print(breatheTimeMs);
      Serial.println(" ms");

      Serial.print("Off Time: ");
      Serial.print(offTimeMs);
      Serial.println(" ms");
    
      Serial.print("Progress Time: ");
      Serial.print(progressTimeMs);
      Serial.println(" ms");
    
      Serial.println("==============================");
    }
    
    
    // =====================================================
    // loop
    // =====================================================
    
    void loop()
    {
      // LED最高优先级
      ledTask();
    
    
      // WiFi后台
      wifiTask();
    
    
      // Web服务器
      server.handleClient();
    
    
      // 短暂让出CPU
      delay(5);
    }