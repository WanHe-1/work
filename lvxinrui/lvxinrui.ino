// Define pins
const int trigPin = 11;        // Trig pin of the ultrasonic sensor
const int echoPin = 10;        // Echo pin of the ultrasonic sensor
const int ledPin = 9;          // LED connected to a PWM pin

// Define measurement variables
long duration;                  // Duration of the ultrasonic pulse (round-trip time)
int distance;                   // Calculated distance in centimeters

// Define distance range (adjust according to your actual setup)
const int minDistance = 3;     // Minimum valid distance in centimeters
const int maxDistance = 80;    // Maximum valid distance in centimeters

void setup() {
  // Initialize serial communication for debugging
  Serial.begin(9600);
  
  // Set pin modes
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ledPin, OUTPUT);
  
  // Initialize LED (optional: turn fully on at start)
  analogWrite(ledPin, 255);
}

void loop() {
  // 1. Send a 10-microsecond ultrasonic pulse
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);        // Ensure low level for at least 2 microseconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);       // Send a 10-microsecond high pulse
  digitalWrite(trigPin, LOW);
  
  // 2. Read the duration of the high pulse on the Echo pin
  duration = pulseIn(echoPin, HIGH);
  
  // 3. Convert time to distance in centimeters
  // Speed of sound is approximately 343 m/s (34300 cm/s)
  // Distance = (duration * speed of sound) / 2 (round-trip)
  distance = duration * 0.0343 / 2;

  // Print distance to serial monitor
  Serial.print(distance);
  Serial.println(" cm");
  delay(10);

  int brightness;
  
  // 4. Determine LED brightness based on distance
  if (distance < minDistance) {
    // Object is too close, set LED to dimmest (almost off)
    brightness = 255;
  } else if (distance > maxDistance) {
    // Object is too far, set LED to brightest
    brightness = 0;
  } else {
    // Map distance to brightness value (inverted: closer = brighter)
    brightness = map(distance, minDistance, maxDistance, 255, 0);
    
    // Ensure brightness stays within valid range [0, 255]
    brightness = constrain(brightness, 0, 255);
  }
  
  // 5. Set LED brightness using PWM
  analogWrite(ledPin, brightness);
  
  // Print brightness value for debugging
  Serial.println(brightness);

  // Delay between measurements
  delay(100);
}