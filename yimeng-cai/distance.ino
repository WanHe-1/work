
#include <Wire.h>
#include <VL53L0X.h>
#include <ESP8266WiFi.h>

// WiFi 设置
const char* ssid = "home";
const char* password = "123456";
const char* serverIP = "192.168.10.1"; // IP
const int serverPort = 8888;

VL53L0X sensor;
WiFiUDP udp;

int deviceID = 1;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  sensor.init();
  sensor.setTimeout(500);
  sensor.startContinuous();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  udp.begin(WiFi.localIP(), 0); 
}

int getZone(float distance) {
  if (distance < 4000) return 3;     // <4m
  else if (distance < 6000) return 2; // 4～6m
  else if (distance <= 8000) return 1; // 6～8m
  else return 0; 
}

void loop() {
  float distance = sensor.readRangeContinuousMillimeters();
  int zone = getZone(distance);

  String msg = String(deviceID) + "," + String(zone);
  udp.beginPacket(serverIP, serverPort);
  udp.print(msg);
  udp.endPacket();

  Serial.println("Sent: " + msg + " (dist=" + String(distance) + "mm)");

  delay(400); 
}