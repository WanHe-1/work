#include <Wire.h>
#include <MAX30105.h>

MAX30105 particleSensor;

const int GSR = A0;

int gsrValue;
long irValue;

void setup() {
    Serial.begin(115200);

    Serial.println("Initializing MAX30105...");

    if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
        Serial.println("MAX30105 err！");
        while (1);
    }

    particleSensor.setup(
        0x1F,   // LED
        8,     
        2,      // Red + IR
        400,   
        411,  
        4096    // ADC
    );

    Serial.println("Start...");
}

void loop() {
    // GSR
    gsrValue = analogRead(GSR);

    irValue = particleSensor.getIR();
    // GSR,IR
    Serial.print(gsrValue);
    Serial.print(",");
    Serial.println(irValue);

    delay(50);
}