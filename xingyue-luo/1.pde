import processing.serial.*;

final String PORT_NAME = "COM3";
final int BAUD_RATE = 9600;

Serial myPort;
float targetWeight = 0;
float displayWeight = 0;

void setup() {
  size(600, 360);
  smooth(8);
  surface.setTitle("HX711 电子秤");
  textAlign(CENTER, CENTER);
  connectSerial();
}

void draw() {
  background(242, 245, 248);
  displayWeight = lerp(displayWeight, targetWeight, 0.15);

  fill(25, 105, 210);
  textSize(64);
  text(nf(displayWeight, 1, 3), width / 2, height / 2);
}

void connectSerial() {
  try {
    myPort = new Serial(this, PORT_NAME, BAUD_RATE);
    myPort.clear();
    myPort.bufferUntil('\n');
  }
  catch (Exception e) {
  }
}

void serialEvent(Serial port) {
  String data = port.readStringUntil('\n');
  if (data == null) return;

  data = trim(data);
  int separator = data.indexOf(':');
  if (separator >= 0) data = trim(data.substring(separator + 1));

  try {
    float value = Float.parseFloat(data);
    if (!Float.isNaN(value) && !Float.isInfinite(value)) {
      targetWeight = value;
    }
  }
  catch (NumberFormatException e) {
  }
}
