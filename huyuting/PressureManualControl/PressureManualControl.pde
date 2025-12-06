// 手动控制压力传感器数值显示

int sensor1Value = 0;
int sensor2Value = 0;

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

// 控制模式
boolean editingLeft = true;  // true=编辑左侧，false=编辑右侧

void setup() {
  size(1600, 900);  // 可移动窗口
  smooth();
  frameRate(60);
  
  println("=== 压力传感器手动控制 ===");
  println("键盘控制:");
  println("  Tab - 切换编辑左侧/右侧传感器");
  println("  ↑ - 增加10");
  println("  ↓ - 减少10");
  println("  → - 增加1");
  println("  ← - 减少1");
  println("  0-9 - 直接输入数值");
  println("  Enter - 确认输入");
  println("  R - 重置为0");
  println("  M - 设置为最大值(1023)");
  println("========================");
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
  textSize(120);
  text(sensor1Value, width / 4, height / 2);
  
  // 右侧数据（黑色）
  fill(0);
  textAlign(CENTER);
  textSize(120);
  text(sensor2Value, width * 0.75, height / 2);
  
  // 显示当前编辑状态
  drawEditIndicator();
  
  // 显示控制提示
  drawControlHints();
  
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
  
  // 大型进度环
  drawCircularProgress(width / 4, height * 0.7, 150, sensor1Value, leftAccent, leftBg, pulseEffect1);
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
  
  // 大型进度环
  drawCircularProgress(width * 0.75, height * 0.7, 150, sensor2Value, rightAccent, rightBg, pulseEffect2);
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
  
  // 中心脉冲圆
  noStroke();
  fill(accentCol, 100 + pulse * 155);
  ellipse(x, y, radius * 0.6 + pulse * 15, radius * 0.6 + pulse * 15);
  
  // 中心点
  fill(accentCol);
  ellipse(x, y, radius * 0.3, radius * 0.3);
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
}

void drawEditIndicator() {
  // 显示当前编辑的传感器
  if (editingLeft) {
    // 左侧编辑指示器
    fill(255, 255, 0);
    noStroke();
    triangle(width / 4 - 80, height / 2 - 100, 
             width / 4 - 80, height / 2 - 80, 
             width / 4 - 60, height / 2 - 90);
    
    fill(255);
    textAlign(LEFT);
    textSize(20);
    text("← 编辑中", width / 4 - 55, height / 2 - 85);
  } else {
    // 右侧编辑指示器
    fill(255, 255, 0);
    noStroke();
    triangle(width * 0.75 + 80, height / 2 - 100, 
             width * 0.75 + 80, height / 2 - 80, 
             width * 0.75 + 60, height / 2 - 90);
    
    fill(0);
    textAlign(RIGHT);
    textSize(20);
    text("编辑中 →", width * 0.75 + 55, height / 2 - 85);
  }
}

void drawControlHints() {
  // 底部控制提示
  fill(100);
  textAlign(CENTER);
  textSize(16);
  text("Tab: 切换 | ↑↓: ±10 | ←→: ±1 | R: 重置 | M: 最大值", width / 2, height - 20);
}

void keyPressed() {
  if (key == TAB) {
    // 切换编辑模式
    editingLeft = !editingLeft;
    println("切换到编辑: " + (editingLeft ? "左侧传感器" : "右侧传感器"));
  }
  else if (key == 'r' || key == 'R') {
    // 重置当前传感器
    if (editingLeft) {
      sensor1Value = 0;
      println("左侧传感器重置为 0");
    } else {
      sensor2Value = 0;
      println("右侧传感器重置为 0");
    }
  }
  else if (key == 'm' || key == 'M') {
    // 设置为最大值
    if (editingLeft) {
      sensor1Value = 1023;
      println("左侧传感器设置为 1023");
    } else {
      sensor2Value = 1023;
      println("右侧传感器设置为 1023");
    }
  }
  else if (keyCode == UP) {
    // 增加10
    if (editingLeft) {
      sensor1Value = constrain(sensor1Value + 10, 0, 1023);
      println("左侧: " + sensor1Value);
    } else {
      sensor2Value = constrain(sensor2Value + 10, 0, 1023);
      println("右侧: " + sensor2Value);
    }
  }
  else if (keyCode == DOWN) {
    // 减少10
    if (editingLeft) {
      sensor1Value = constrain(sensor1Value - 10, 0, 1023);
      println("左侧: " + sensor1Value);
    } else {
      sensor2Value = constrain(sensor2Value - 10, 0, 1023);
      println("右侧: " + sensor2Value);
    }
  }
  else if (keyCode == RIGHT) {
    // 增加1
    if (editingLeft) {
      sensor1Value = constrain(sensor1Value + 1, 0, 1023);
      println("左侧: " + sensor1Value);
    } else {
      sensor2Value = constrain(sensor2Value + 1, 0, 1023);
      println("右侧: " + sensor2Value);
    }
  }
  else if (keyCode == LEFT) {
    // 减少1
    if (editingLeft) {
      sensor1Value = constrain(sensor1Value - 1, 0, 1023);
      println("左侧: " + sensor1Value);
    } else {
      sensor2Value = constrain(sensor2Value - 1, 0, 1023);
      println("右侧: " + sensor2Value);
    }
  }
  else if (key >= '0' && key <= '9') {
    // 数字输入（简单版本 - 直接设置为数字*100）
    int digit = key - '0';
    int newValue = digit * 100;
    if (editingLeft) {
      sensor1Value = constrain(newValue, 0, 1023);
      println("左侧设置为: " + sensor1Value);
    } else {
      sensor2Value = constrain(newValue, 0, 1023);
      println("右侧设置为: " + sensor2Value);
    }
  }
  else if (key == 'q' || key == 'Q' || key == ESC) {
    exit();
  }
}
