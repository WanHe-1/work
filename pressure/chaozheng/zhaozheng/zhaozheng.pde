import processing.serial.*;

// 串口通信
Serial myPort;
String portName;

// 数据存储
float[][] sensorData = new float[4][1000];  // 4个传感器，每个存储1000个数据点
int[] dataIndex = new int[4];  // 每个传感器的数据索引
float[] totalData = new float[1000];  // 总和数据
int totalIndex = 0;

// 传感器配置
int numSensors = 4;
String[] sensorNames = {"传感器1", "传感器2", "传感器3", "传感器4"};
color[] sensorColors = {#FF6B6B, #4ECDC4, #45B7D1, #96CEB4};

// 显示设置
int graphWidth = 1800;
int graphHeight = 750;
int margin = 50;
int legendHeight = 30;

// 数据统计
float[] currentValues = new float[4];
float currentTotal = 0;
float[] minValues = new float[4];
float[] maxValues = new float[4];
boolean[] initialized = new boolean[4];

// 显示模式
int displayMode = 0;  // 0: 总和, 1: 单独显示, 2: 对比显示
boolean showLegend = true;
boolean showStats = true;

void setup() {
  size(2000, 900);  // 增大宽度到2000
  background(240);
  
  // 初始化数据
  for (int i = 0; i < numSensors; i++) {
    dataIndex[i] = 0;
    minValues[i] = 90000.0;
    maxValues[i] = 0.0;
    initialized[i] = false;
  }
  totalIndex = 0;
  
  // 串口初始化
  try {
    if (Serial.list().length> 0) {
      // 优先查找COM3，如果找不到则使用第一个可用串口
      portName = null;
      for (String port : Serial.list()) {
        if (port.contains("COM3")) {
          portName = port;
          break;
        }
      }
      
      // 如果没找到COM3，使用第一个串口
      if (portName == null) {
        portName = Serial.list()[0];
      }
      
      myPort = new Serial(this, portName, 9600);
      myPort.bufferUntil('\n');
      println("已连接到串口: " + portName);
      println("可用串口列表:");
      for (String port : Serial.list()) {
        println("  " + port);
      }
    } else {
      println("未找到可用串口！");
    }
  } catch (Exception e) {
    println("串口初始化失败: " + e.getMessage());
  }
  
  // 设置字体
  textAlign(LEFT, CENTER);
  textSize(12);
}

void draw() {
  background(240);
  
  // 绘制连接状态（移到右上角）
  if (myPort != null && myPort.available() > 0) {
    fill(0, 150, 0);
    textAlign(RIGHT, CENTER);
    textSize(12);
    text("串口已连接: " + portName, width - 20, 20);
  } else {
    fill(150, 0, 0);
    textAlign(RIGHT, CENTER);
    textSize(12);
    text("串口未连接或无数据", width - 20, 20);
  }
  
  // 绘制主图表
  drawMainGraph();
  
  // 绘制图例
  if (showLegend) {
    drawLegend();
  }
  
  // 绘制控制信息
  drawControls();
  
  // 绘制调试信息
  drawDebugInfo();
}

void drawMainGraph() {
  // 设置绘图区域
  int graphX = margin + 50;  // 为Y轴标签留出更多空间
  int graphY = 20;  // 从顶部开始
  int graphW = graphWidth - 2 * margin - 50;  // 相应减少宽度
  int graphH = graphHeight - margin;  // 减少底部边距
  
  // 绘制背景网格
  stroke(200);
  strokeWeight(1);
  for (int i = 0; i <= 10; i++) {
    float x = graphX + (graphW * i / 10);
    line(x, graphY, x, graphY + graphH);
  }
  for (int i = 0; i <= 6; i++) {
    float y = graphY + (graphH * i / 6);
    line(graphX, y, graphX + graphW, y);
  }
  
  // 绘制坐标轴
  stroke(100);
  strokeWeight(2);
  line(graphX, graphY + graphH, graphX + graphW, graphY + graphH);  // X轴
  line(graphX, graphY, graphX, graphY + graphH);  // Y轴
  
  // 绘制数据曲线
  if (displayMode == 0) {
    // 显示总和
    drawTotalGraph(graphX, graphY, graphW, graphH);
  } else if (displayMode == 1) {
    // 显示所有传感器
    for (int i = 0; i < numSensors; i++) {
      drawSensorGraph(i, graphX, graphY, graphW, graphH);
    }
  } else {
    // 对比显示
    drawComparisonGraph(graphX, graphY, graphW, graphH);
  }
  
  // 绘制Y轴标签
  fill(100);
  textAlign(RIGHT, CENTER);
  textSize(16);  // 进一步增大字体
  for (int i = 0; i <= 6; i++) {
    int value = (int)(90000 * i / 6);
    float y = graphY + graphH - (graphH * i / 6);
    text(value + "g", graphX - 20, y);  // 调整位置，确保在图表左侧
  }
}

void drawTotalGraph(int x, int y, int w, int h) {
  stroke(#FF6B6B);
  strokeWeight(2);
  noFill();
  
  beginShape();
  for (int i = 0; i < totalData.length - 1; i++) {
    if (totalData[i] >= 0 && totalData[i + 1] >= 0) {  // 改为>=0，允许显示0值
      float x1 = x + (w * i / (totalData.length - 1));
      float y1 = y + h - (h * totalData[i] / 90000);
      float x2 = x + (w * (i + 1) / (totalData.length - 1));
      float y2 = y + h - (h * totalData[i + 1] / 90000);
      line(x1, y1, x2, y2);
    }
  }
  endShape();
}

void drawSensorGraph(int sensorIndex, int x, int y, int w, int h) {
  stroke(sensorColors[sensorIndex]);
  strokeWeight(1);
  noFill();
  
  beginShape();
  for (int i = 0; i < sensorData[sensorIndex].length - 1; i++) {
    if (sensorData[sensorIndex][i] >= 0 && sensorData[sensorIndex][i + 1] >= 0) {  // 改为>=0，允许显示0值
      float x1 = x + (w * i / (sensorData[sensorIndex].length - 1));
      float y1 = y + h - (h * sensorData[sensorIndex][i] / 90000);
      float x2 = x + (w * (i + 1) / (sensorData[sensorIndex].length - 1));
      float y2 = y + h - (h * sensorData[sensorIndex][i + 1] / 90000);
      line(x1, y1, x2, y2);
    }
  }
  endShape();
}

void drawComparisonGraph(int x, int y, int w, int h) {
  // 绘制所有传感器在同一个图表中，使用不同颜色
  for (int i = 0; i < numSensors; i++) {
    stroke(sensorColors[i]);
    strokeWeight(1);
    noFill();
    
    beginShape();
    for (int j = 0; j < sensorData[i].length - 1; j++) {
      if (sensorData[i][j] >= 0 && sensorData[i][j + 1] >= 0) {  // 改为>=0，允许显示0值
        float x1 = x + (w * j / (sensorData[i].length - 1));
        float y1 = y + h - (h * sensorData[i][j] / 90000);
        float x2 = x + (w * (j + 1) / (sensorData[i].length - 1));
        float y2 = y + h - (h * sensorData[i][j + 1] / 90000);
        line(x1, y1, x2, y2);
      }
    }
    endShape();
  }
}

void drawLegend() {
  int legendY = graphHeight + 30;  // 更靠近图表
  
  fill(50);
  textAlign(LEFT, CENTER);
  textSize(17);
  text("图例:", 100, legendY);  // 调整位置，与图表对齐
  
  for (int i = 0; i < numSensors; i++) {
    // 绘制颜色方块
    fill(sensorColors[i]);
    rect(180 + i * 250, legendY - 12, 20, 20);
    
    // 绘制标签
    fill(50);
    text(sensorNames[i] + ": " + nf(currentValues[i], 0, 1) + "g", 210 + i * 250, legendY);
  }
  
  // 总和显示
  fill(#FF6B6B);
  rect(180 + numSensors * 250, legendY - 12, 20, 20);
  fill(50);
  text("总和: " + nf(currentTotal, 0, 1) + "g", 210 + numSensors * 250, legendY);
}


void drawControls() {
  int controlY = height - 60;
  
  fill(100);
  textAlign(LEFT, CENTER);
  textSize(12);
  text("控制: 1-总和模式, 2-单独模式, 3-对比模式, L-切换图例", 100, controlY);
}

void drawDebugInfo() {
  int debugY = height - 35;
  
  fill(50);
  textAlign(LEFT, CENTER);
  textSize(11);
  text("数据点数量: " + totalIndex + " | 当前值: " + nf(currentTotal, 0, 1) + "g", 100, debugY);
}

void serialEvent(Serial myPort) {
  String data = myPort.readStringUntil('\n');
  if (data != null) {
    data = trim(data);
    println("接收到数据: " + data);  // 调试信息
    
    // 检查是否是新的格式数据（S1:xxxg S2:xxxg S3:xxxg S4:xxxg | Total:xxxg）
    if (data.contains("S1:") && data.contains("Total:")) {
      // 解析各个传感器的值
      for (int i = 0; i < numSensors; i++) {
        String sensorPattern = "S" + (i + 1) + ":";
        int startIndex = data.indexOf(sensorPattern);
        if (startIndex != -1) {
          startIndex += sensorPattern.length();
          int endIndex = data.indexOf("g", startIndex);
          if (endIndex != -1) {
            try {
              String valueStr = data.substring(startIndex, endIndex);
              float value = float(valueStr);
              currentValues[i] = value;
              
              // 更新统计数据
              if (!initialized[i] || value < minValues[i]) minValues[i] = value;
              if (!initialized[i] || value > maxValues[i]) maxValues[i] = value;
              initialized[i] = true;
              
              // 存储到数据数组
              sensorData[i][dataIndex[i]] = value;
              dataIndex[i]++;
              if (dataIndex[i] >= sensorData[i].length) {
                dataIndex[i] = 0;
              }
              println("传感器" + (i + 1) + "解析成功: " + value + "g");  // 调试信息
            } catch (NumberFormatException e) {
              println("解析传感器 " + (i + 1) + " 数据失败: " + data);
            }
          }
        }
      }
      
      // 解析总压力值
      int totalStartIndex = data.indexOf("Total:");
      if (totalStartIndex != -1) {
        totalStartIndex += 6; // "Total:"的长度
        int totalEndIndex = data.indexOf("g", totalStartIndex);
        if (totalEndIndex != -1) {
          try {
            String totalStr = data.substring(totalStartIndex, totalEndIndex);
            currentTotal = float(totalStr);
            totalData[totalIndex] = currentTotal;
            totalIndex++;
            if (totalIndex >= totalData.length) {
              totalIndex = 0;
            }
            println("解析成功 - 总压力: " + currentTotal + "g");  // 调试信息
          } catch (NumberFormatException e) {
            println("解析总压力数据失败: " + data);
          }
        }
      }
    } else if (data.contains(",")) {
      // 兼容旧CSV格式数据（传感器1,传感器2,传感器3,传感器4,总和）
      String[] values = split(data, ",");
      if (values.length >= 5) {
        // 解析各个传感器的值
        for (int i = 0; i < numSensors; i++) {
          try {
            float value = float(values[i]);
            currentValues[i] = value;
            
            // 更新统计数据
            if (!initialized[i] || value < minValues[i]) minValues[i] = value;
            if (!initialized[i] || value > maxValues[i]) maxValues[i] = value;
            initialized[i] = true;
            
            // 存储到数据数组
            sensorData[i][dataIndex[i]] = value;
            dataIndex[i]++;
            if (dataIndex[i] >= sensorData[i].length) {
              dataIndex[i] = 0;
            }
          } catch (NumberFormatException e) {
            println("解析传感器 " + i + " 数据失败: " + values[i]);
          }
        }
        
        // 解析总和值
        try {
          currentTotal = float(values[numSensors]);
          totalData[totalIndex] = currentTotal;
          totalIndex++;
          if (totalIndex >= totalData.length) {
            totalIndex = 0;
          }
        } catch (NumberFormatException e) {
          println("解析总和数据失败: " + values[numSensors]);
        }
      }
    } else {
      // 兼容旧格式（只有总和）
      try {
        float sensorValue = float(data);
        currentTotal = sensorValue;
        totalData[totalIndex] = sensorValue;
        totalIndex++;
        if (totalIndex >= totalData.length) {
          totalIndex = 0;
        }
      } catch (NumberFormatException e) {
        println("解析数据失败: " + data);
      }
    }
  }
}

void keyPressed() {
  switch (key) {
    case '1':
      displayMode = 0;
      println("切换到总和显示模式");
      break;
    case '2':
      displayMode = 1;
      println("切换到单独显示模式");
      break;
    case '3':
      displayMode = 2;
      println("切换到对比显示模式");
      break;
    case 'l':
    case 'L':
      showLegend = !showLegend;
      println("图例显示: " + (showLegend ? "开启" : "关闭"));
      break;
  }
}
