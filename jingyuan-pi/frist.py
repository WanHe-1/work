#include <WiFi.h>
#include <WebServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>

const char* ssid = "SIAStudio-office";
const char* password = "helloworld";

WebServer server(80);

#define ONE_WIRE_BUS 2   // DS18B20
const int pulsePin = 3;  // 心率

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

float temperature = 0;
int pulseValue = 0;

void handleRoot() {
  String html = "<html><head>";
  html += "<meta http-equiv='refresh' content='1'>";
  html += "<title>ESP32 Monitor</title>";
  html += "</head><body>";

  html += "<h1>ESP32 Health Monitor</h1>";

  html += "<h2>Heart Rate</h2>";
  html += "<p>Raw Value: " + String(pulseValue) + "</p>";

  html += "<h2>Temperature</h2>";
  html += "<p>" + String(temperature, 2) + " °C</p>";

  html += "</body></html>";

  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(9600);

  pinMode(pulsePin, INPUT);

  sensors.begin();

  WiFi.begin(ssid, password);
  Serial.print("Connecting");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());

  // Web
  server.on("/", handleRoot);
  server.begin();
}

void loop() {
  server.handleClient();

  pulseValue = analogRead(pulsePin);

  sensors.requestTemperatures();
  temperature = sensors.getTempCByIndex(0);

  Serial.print("Pulse: ");
  Serial.print(pulseValue);
  Serial.print(" | Temp: ");
  Serial.println(temperature);

  delay(1000);
}