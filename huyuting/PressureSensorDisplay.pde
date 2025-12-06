import processing.serial.*;

Serial myPort;
int sensor1Value = 0;
int sensor2Value = 0;

// 历史数据数组用于绘制曲线
ArrayList<Integer> sensor1History = new ArrayList<Integer>();
ArrayList<Integer> sensor2History = new ArrayList<Integer>();
int maxHistoryLength = 200;

// 颜色主题
color bgColor = #0a0e27;
color sensor1Color = #00d4ff;
color sensor2Color = #ff6b9d;
color accentColor = #ffd93d;
color textColor = #ffffff;
color gridColor = #1a2332;

// 动画效果
float pulseEffect1 = 0;
float pulseEffect2 = 0;

void setup() {
  fullScreen();
  //size(1920, 1080);  // 备用固定尺寸
  
  // 列出所有可用串口
  println("Available serial ports:");
  printArray(Serial.list());
  
  // 选择正确的串口（根据实际情况修改索引）
  // Windows 通常是 "COM3", "COM4" 等
  String portName = "COM16";  // 修改索引以选择正确的串口
  myPort = new Serial(this, portName, 9600);
  myPort.bufferUntil('\n');
  
  smooth();
  frameRate(60);
}

void draw() {
  // 渐变背景
  setGradientBackground();
  
  // 绘制网格
  drawGrid();
  
  // 标题
  drawTitle();
  
  // 绘制传感器1数据
  drawSensorPanel(width * 0.08, height * 0.2, width * 0.38, height * 0.7, 
                  "压力传感器 1", sensor1Value, sensor1Color, sensor1History, pulseEffect1);
  
  // 绘制传感器2数据
  drawSensorPanel(width * 0.54, height * 0.2, width * 0.38, height * 0.7, 
                  "压力传感器 2", sensor2Value, sensor2Color, sensor2History, pulseEffect2);
  
  // 更新脉冲效果
  pulseEffect1 = lerp(pulseEffect1, map(sensor1Value, 0, 1023, 0, 1), 0.1);
  pulseEffect2 = lerp(pulseEffect2, map(sensor2Value, 0, 1023, 0, 1), 0.1);
}

void setGradientBackground() {
  for (int i = 0; i < height; i++) {
    float inter = map(i, 0, height, 0, 1);
    color c = lerpColor(#0a0e27, #1a1f3a, inter);
    stroke(c);
    line(0, i, width, i);
  }
}

void drawGrid() {
  stroke(gridColor);
  strokeWeight(1);
  
  // 垂直线
  for (int x = 0; x < width; x += 50) {
    line(x, 0, x, height);
  }
  
  // 水平线
  for (int y = 0; y < height; y += 50) {
    line(0, y, width, y);
  }
}

void drawTitle() {
  fill(textColor);
  textAlign(CENTER);
  textSize(48);
  text("压力传感器实时监控系统", width / 2, height * 0.08);
  
  textSize(20);
  fill(accentColor);
  text("Real-time Pressure Sensor Monitoring", width / 2, height * 0.12);
}

void drawSensorPanel(float x, float y, float w, float h, String title, int value, color col, ArrayList<Integer> history, float pulse) {
  // 面板背景
  noStroke();
  fill(20, 30, 50, 200);
  rect(x, y, w, h, 20);
  
  // 发光边框效果
  strokeWeight(3);
  stroke(col, 100 + pulse * 155);
  noFill();
  rect(x, y, w, h, 20);
  
  // 标题
  fill(col);
  textAlign(CENTER);
  textSize(32);
  text(title, x + w / 2, y + 40);
  
  // 数值显示
  textSize(80);
  fill(textColor);
  text(value, x + w / 2, y + h / 2 - 20);
  
  textSize(24);
  fill(col);
  text("/ 1023", x + w / 2, y + h / 2 + 20);
  
  // 百分比条
  float barWidth = w * 0.8;
  float barHeight = 40;
  float barX = x + (w - barWidth) / 2;
  float barY = y + h / 2 + 60;
  
  // 条形背景
  noStroke();
  fill(30, 40, 60);
  rect(barX, barY, barWidth, barHeight, 20);
  
  // 条形填充
  float fillWidth = map(value, 0, 1023, 0, barWidth);
  fill(col);
  rect(barX, barY, fillWidth, barHeight, 20);
  
  // 百分比文字
  textSize(20);
  fill(textColor);
  float percentage = map(value, 0, 1023, 0, 100);
  text(nf(percentage, 0, 1) + "%", x + w / 2, barY + barHeight + 30);
  
  // 圆形指示器
  drawCircularIndicator(x + w / 2, y + h - 100, 60, value, col, pulse);
}

void drawCircularIndicator(float x, float y, float radius, int value, color col, float pulse) {
  // 背景圆
  noFill();
  strokeWeight(8);
  stroke(30, 40, 60);
  ellipse(x, y, radius * 2, radius * 2);
  
  // 进度圆弧
  strokeWeight(8);
  stroke(col);
  float angle = map(value, 0, 1023, 0, TWO_PI);
  noFill();
  arc(x, y, radius * 2, radius * 2, -HALF_PI, -HALF_PI + angle);
  
  // 发光中心点
  noStroke();
  fill(col, 150 + pulse * 105);
  ellipse(x, y, radius * 0.5 + pulse * 10, radius * 0.5 + pulse * 10);
}


void serialEvent(Serial port) {
  String inString = port.readStringUntil('\n');
  
  if (inString != null) {
    inString = trim(inString);
    
    // 解析数据格式: "Sensor1: 123 | Sensor2: 456 | Pos: 90"
    if (inString.contains("Sensor1:") && inString.contains("Sensor2:")) {
      try {
        String[] parts = split(inString, '|');
        
        // 解析传感器1
        String sensor1Str = parts[0].replace("Sensor1:", "").trim();
        sensor1Value = int(sensor1Str);
        
        // 解析传感器2
        String sensor2Str = parts[1].replace("Sensor2:", "").trim();
        sensor2Value = int(sensor2Str);
        
        // 添加到历史记录
        sensor1History.add(sensor1Value);
        sensor2History.add(sensor2Value);
        
        // 限制历史记录长度
        if (sensor1History.size() > maxHistoryLength) {
          sensor1History.remove(0);
        }
        if (sensor2History.size() > maxHistoryLength) {
          sensor2History.remove(0);
        }
        
      } catch (Exception e) {
        println("Error parsing data: " + e.getMessage());
      }
    }
  }
}

void keyPressed() {
  if (key == 'q' || key == 'Q' || key == ESC) {
    exit();
  }
}
