#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

#define L298N_ENA 6   // PWM
#define L298N_ENB 3   // PWM
#define L298N_IN1 5
#define L298N_IN2 7
#define L298N_IN3 4
#define L298N_IN4 2

// --- Global objects ---
SoftwareSerial dfSerial(12, 13); // RX, TX
DFRobotDFPlayerMini dfPlayer;

// --- State variables ---
int currentBrightness = 0;    // Current brightness level
int targetBrightness = 0;     // Target brightness level
unsigned long lastUpdateTime = 0;  // Last time brightness was updated

const int SMOOTH_DELAY = 20;       // Brightness update interval (ms)
const int BRIGHTNESS_STEP = 5;     // Brightness change per step

// Audio playback control flags
bool audio1Playing = false;
bool audio2Playing = false;
bool audio3Playing = false;
bool audio4Playing = false;

unsigned long audio1StartTime = 0;
unsigned long audio2StartTime = 0;
unsigned long audio3StartTime = 0;
unsigned long audio4StartTime = 0;

const unsigned long AUDIO_DURATION = 1000; // Audio playback duration: 1 second

void setup() {
  // Initialize hardware serial (for receiving data from TinyF)
  Serial.begin(115200);
  Serial.println("System started. Waiting for TinyF data...");

  // Initialize L298N motor driver pins
  pinMode(L298N_ENA, OUTPUT);
  pinMode(L298N_ENB, OUTPUT);
  pinMode(L298N_IN1, OUTPUT);
  pinMode(L298N_IN2, OUTPUT);
  pinMode(L298N_IN3, OUTPUT);
  pinMode(L298N_IN4, OUTPUT);

  // Set motor direction (for lighting: IN1/IN3 = HIGH, IN2/IN4 = LOW)
  digitalWrite(L298N_IN1, HIGH);
  digitalWrite(L298N_IN2, LOW);
  digitalWrite(L298N_IN3, HIGH);
  digitalWrite(L298N_IN4, LOW);

  // Initialize DFPlayer Mini
  dfSerial.begin(9600);
  Serial.println("Initializing DFPlayer Mini...");
  if (!dfPlayer.begin(dfSerial)) {
    Serial.println("❌ DFPlayer initialization failed!");
    Serial.println("Check wiring, SD card, and power supply.");
    while (true) delay(1000); // Halt with error indication
  }
  dfPlayer.volume(30);      // Volume: 0 (mute) to 30 (max)
  dfPlayer.loop(false);     // Do not loop playback
  Serial.println("✅ DFPlayer Mini ready.");
}

void loop() {
  // Smoothly update brightness
  updateSmoothBrightness();

  // Check if any audio clip has exceeded its playback duration
  checkAudioTiming();

  // Process incoming serial data
  if (Serial.available() > 0) {
    String data = Serial.readStringUntil('\n');
    parseDistanceData(data);
  }
}

void parseDistanceData(String &data) {
  // Clean up the input string
  data.trim();

  // Locate comma separator
  int commaPos = data.indexOf(',');
  if (commaPos == -1) return;

  // Extract distance string
  String distStr = data.substring(0, commaPos);
  distStr.trim();

  // Extract confidence string
  String confStr = data.substring(commaPos + 1);
  confStr.trim();

  // Convert to numeric values
  long distance_mm = distStr.toInt();
  long confidence = confStr.toInt();

  // Validate data integrity
  if ((distance_mm <= 20 || distance_mm >= 4000) && distStr != "0") return;
  if (confidence == 0 && confStr != "0") return;

  // Convert to centimeters
  float distance_cm = distance_mm / 10.0;

  // ====== Smooth brightness control based on distance ======
  if (distance_cm >= 0 && distance_cm <= 200) {
    if (distance_cm <= 40) {
      // 0–40 cm: Linear increase to near-max brightness
      targetBrightness = map(distance_cm, 0, 40, 255, 200);
    } else {
      // 40–200 cm: Exponential decay
      float ratio = (distance_cm - 40) / 160.0; // Normalized to [0,1]
      targetBrightness = 200 * exp(-ratio * 3.0);
      if (targetBrightness < 10) targetBrightness = 0; // Turn off if too dim
    }
  } else {
    targetBrightness = 0; // Turn off beyond 200 cm
  }

  // Trigger audio clips based on distance zones (only if not already playing)
  if (distance_cm >= 30 && distance_cm < 40 && !audio1Playing) {
    dfPlayer.play(1);
    audio1Playing = true;
    audio1StartTime = millis();
    Serial.println("🔊 Starting Audio 1 (30–40 cm) – 1s duration");
  } else if (distance_cm >= 20 && distance_cm < 30 && !audio2Playing) {
    dfPlayer.play(2);
    audio2Playing = true;
    audio2StartTime = millis();
    Serial.println("🔊 Starting Audio 2 (20–30 cm) – 1s duration");
  } else if (distance_cm >= 10 && distance_cm < 20 && !audio3Playing) {
    dfPlayer.play(3);
    audio3Playing = true;
    audio3StartTime = millis();
    Serial.println("🔊 Starting Audio 3 (10–20 cm) – 1s duration");
  } else if (distance_cm >= 0 && distance_cm < 10 && !audio4Playing) {
    dfPlayer.play(4);
    audio4Playing = true;
    audio4StartTime = millis();
    Serial.println("🔊 Starting Audio 4 (0–10 cm) – 1s duration");
  }

  // Debug output
  Serial.print("📏 Distance: ");
  Serial.println(distance_cm, 1); // Print with 1 decimal place
}

// Smoothly interpolate brightness toward target
void updateSmoothBrightness() {
  unsigned long currentTime = millis();

  if (currentTime - lastUpdateTime >= SMOOTH_DELAY) {
    if (currentBrightness != targetBrightness) {
      if (currentBrightness < targetBrightness) {
        currentBrightness += BRIGHTNESS_STEP;
        if (currentBrightness > targetBrightness) {
          currentBrightness = targetBrightness;
        }
      } else {
        currentBrightness -= BRIGHTNESS_STEP;
        if (currentBrightness < targetBrightness) {
          currentBrightness = targetBrightness;
        }
      }

      // Apply brightness to PWM outputs
      analogWrite(L298N_ENA, currentBrightness);
      analogWrite(L298N_ENB, currentBrightness);
    }

    lastUpdateTime = currentTime;
  }
}

// Stop audio clips after 1 second of playback
void checkAudioTiming() {
  unsigned long currentTime = millis();

  if (audio1Playing && (currentTime - audio1StartTime >= AUDIO_DURATION)) {
    dfPlayer.stop();
    audio1Playing = false;
    Serial.println("🔇 Audio 1 stopped after 1s");
  }

  if (audio2Playing && (currentTime - audio2StartTime >= AUDIO_DURATION)) {
    dfPlayer.stop();
    audio2Playing = false;
    Serial.println("🔇 Audio 2 stopped after 1s");
  }

  if (audio3Playing && (currentTime - audio3StartTime >= AUDIO_DURATION)) {
    dfPlayer.stop();
    audio3Playing = false;
    Serial.println("🔇 Audio 3 stopped after 1s");
  }

  if (audio4Playing && (currentTime - audio4StartTime >= AUDIO_DURATION)) {
    dfPlayer.stop();
    audio4Playing = false;
    Serial.println("🔇 Audio 4 stopped after 1s");
  }
}