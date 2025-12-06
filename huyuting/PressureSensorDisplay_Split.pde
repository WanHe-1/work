import processing.serial.*;

Serial myPort;
int sensor1Value = 0;
int sensor2Value = 0;

// 历史数据
ArrayList<Integer> sensor1History = new ArrayList<Integer>();
ArrayList<Integer> sensor2History = new ArrayList<Integer>();
int maxHistoryLength = 150;

// 颜色主题
color leftBg = #000000;        // 左侧黑色背景
color leftText = #ffffff;      // 左侧白色文字
color leftAccent = #ffffff;    // 左侧强调色（白色）

color rightBg = #ffffff;       // 右侧白色背景
color rightText = #000000;     // 右侧黑色文字
color rightAccent = #000000;   // 右侧强调色（黑色）

// 动画效果
float pulseEffect1 = 0;
float pulseEffect2 = 0;
float time = 0;

void setup() {
  size(1600, 900);  // 可移动窗口
  //fullScreen();  // 全屏模式
  
  println("可用串口:");
  printArray(Serial.list());
  
  // 选择串口（修改索引以匹配您的Arduino端口）
  String portName = "COM16";
  myPort = new Serial(this, portName, 9600);
  myPort.bufferUntil('\n');
  
  smooth();
  frameRate(60);
}

void draw() {
  // 绘制左侧面板（传感器1 - 黑底白字）
  drawLeftPanel();
  
  // 绘制右侧面板（传感器2 - 白底黑字）
  drawRightPanel();
  
  // 绘制中央分隔线
  drawDivider();
  
  // 数据显示在中间
  // 左侧数据（白色）
  fill(255);
  textAlign(CENTER);
  textSize(72);
  text(sensor1Value, width / 4, height / 2);
  
  // 右侧数据（黑色）
  fill(0);
  textAlign(CENTER);
  textSize(72);
  text(sensor2Value, width * 0.75, height / 2);
  
  // 更新动画
  pulseEffect1 = lerp(pulseEffect1, map(sensor1Value, 0, 1023, 0, 1), 0.15);
  pulseEffect2 = lerp(pulseEffect2, map(sensor2Value, 0, 1023, 0, 1), 0.15);
  time += 0.02;
}

void drawLeftPanel() {
  // 左侧背景
  noStroke();
  fill(leftBg);
  rect(0, 0, width / 2, height);
  
  // 装饰网格
  stroke(40, 40, 40);
  strokeWeight(1);
  for (int x = 0; x < width / 2; x += 80) {
    line(x, 0, x, height);
  }
  for (int y = 0; y < height; y += 80) {
    line(0, y, width / 2, y);
  }
  
  // 标题区域
  fill(leftAccent);
  noStroke();
  rect(0, 0, width / 2, height * 0.12);
  
  // 标题文字
  fill(leftBg);
  textAlign(CENTER);
  textSize(48);
  text("压力传感器 1", width / 4, height * 0.07);
  
  // 主视觉显示区域
  float centerY = height * 0.35;
  
  // 大型进度环
  drawCircularProgress(width / 4, height * 0.5, 200, sensor1Value, leftAccent, leftBg, pulseEffect1);
}

void drawRightPanel() {
  // 右侧背景
  noStroke();
  fill(rightBg);
  rect(width / 2, 0, width / 2, height);
  
  // 装饰网格
  stroke(230, 230, 230);
  strokeWeight(1);
  for (int x = width / 2; x < width; x += 80) {
    line(x, 0, x, height);
  }
  for (int y = 0; y < height; y += 80) {
    line(width / 2, y, width, y);
  }
  
  // 标题区域
  fill(rightAccent);
  noStroke();
  rect(width / 2, 0, width / 2, height * 0.12);
  
  // 标题文字
  fill(rightBg);
  textAlign(CENTER);
  textSize(48);
  text("压力传感器 2", width * 0.75, height * 0.07);
  
  // 主视觉显示区域
  float centerY = height * 0.35;
  
  // 大型进度环
  drawCircularProgress(width * 0.75, height * 0.5, 200, sensor2Value, rightAccent, rightBg, pulseEffect2);
}
void drawCircularProgress(float x, float y, float radius, int value, color accentCol, color bgCol, float pulse) {
  // 外圈装饰
  noFill();
  strokeWeight(2);
  stroke(accentCol, 50);
  ellipse(x, y, radius * 2.3, radius * 2.3);
  
  // 背景圆
  strokeWeight(12);
  if (bgCol == #000000) {
    stroke(40, 40, 40);
  } else {
    stroke(220, 220, 220);
  }
  ellipse(x, y, radius * 2, radius * 2);
  
  // 进度圆弧
  strokeWeight(12);
  stroke(accentCol);
  float angle = map(value, 0, 1023, 0, TWO_PI);
  noFill();
  arc(x, y, radius * 2, radius * 2, -HALF_PI, -HALF_PI + angle);
  
  /*
  // 注释掉中心脉冲圆和中心点的绘制代码
  // 中心脉冲圆
  noStroke();
  fill(accentCol, 100 + pulse * 155);
  ellipse(x, y, radius * 0.6 + pulse * 15, radius * 0.6 + pulse * 15);
  
  // 中心点
  fill(accentCol);
  ellipse(x, y, radius * 0.3, radius * 0.3);
  */
}
void drawDivider() {
  // 中央分隔线
  strokeWeight(4);
  
  // 渐变分隔线效果
  for (int i = 0; i < height; i++) {
    float alpha = 150 + sin(time + i * 0.01) * 50;
    stroke(100, 100, 100, alpha);
    point(width / 2, i);
  }
  
  // 顶部装饰
  noStroke();
  fill(255);
  triangle(width / 2 - 15, 0, width / 2 + 15, 0, width / 2, 30);
  
  fill(0);
  triangle(width / 2 - 15, height, width / 2 + 15, height, width / 2, height - 30);
  
  // 移除中央圆圈和VS文字
}

void serialEvent(Serial port) {
  String inString = port.readStringUntil('\n');
  
  if (inString != null) {
    inString = trim(inString);
    
    // 打印接收到的数据以便调试
    println("接收: " + inString);
    
    if (inString.contains("Left:") && inString.contains("Right:")) {
      try {
        // 解析格式: "Left: 0 | Right: 0 | Pos: 90 (0°)"
        String[] parts = split(inString, '|');
        
        println("分割后的部分数: " + parts.length);
        
        if (parts.length >= 2) {
          // 解析传感器1 (Left)
          String leftPart = trim(parts[0].replace("Left:", ""));
          println("Left部分: '" + leftPart + "'");
          sensor1Value = int(leftPart);
          
          // 解析传感器2 (Right)
          String rightPart = trim(parts[1].replace("Right:", ""));
          println("Right部分: '" + rightPart + "'");
          sensor2Value = int(rightPart);
          
          println("✓ 解析成功 - Left: " + sensor1Value + ", Right: " + sensor2Value);
          
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
        } else {
          println("✗ 错误: 分割后的部分数不足");
        }
        
      } catch (Exception e) {
        println("✗ 解析错误: " + e.getMessage());
        e.printStackTrace();
      }
    } else {
      println("✗ 数据格式不匹配 (没有Left:或Right:)");
    }
  }
}

void keyPressed() {
  if (key == 'q' || key == 'Q' || key == ESC) {
    exit();
  }
}
