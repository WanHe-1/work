#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>


//================ WiFi =================

const char* ssid = "OKN";
const char* password = "OKN666123";


WebServer server(80);



//================ GPIO =================

#define TRIGGER_PIN 3



// ESP32-C3 I2C

#define SDA_PIN 8
#define SCL_PIN 9




//================ MPU6050 =================

#define REG_PWR_MGMT_1     0x6B
#define REG_SMPLRT_DIV     0x19
#define REG_CONFIG         0x1A
#define REG_GYRO_CONFIG    0x1B
#define REG_ACCEL_CONFIG   0x1C
#define REG_ACCEL_XOUT_H   0x3B
#define REG_WHO_AM_I       0x75



byte mpu_addr = 0x68;



const float GYRO_SCALE = 65.5;

const float GYRO_THRESHOLD = 120.0;



const unsigned long HOLD_TIME = 5000;

const unsigned long COOLDOWN_TIME = 800;



unsigned long pin_high_start = 0;

unsigned long last_trigger_time = 0;



bool pin_active = false;




//================ 数据 =================


int16_t accelX;
int16_t accelY;
int16_t accelZ;


int16_t gyroX;
int16_t gyroY;
int16_t gyroZ;



float nod_speed;

float shake_speed;





//================ MPU函数 =================


bool isKnownMpuId(byte id)
{

return id==0x68 ||
       id==0x69 ||
       id==0x70 ||
       id==0x71 ||
       id==0x73;

}



void writeRegister(byte reg,byte value)
{

Wire.beginTransmission(mpu_addr);

Wire.write(reg);

Wire.write(value);

Wire.endTransmission();

}




byte readRegister(byte reg)
{

Wire.beginTransmission(mpu_addr);

Wire.write(reg);

Wire.endTransmission(false);


Wire.requestFrom(mpu_addr,(byte)1);


return Wire.read();

}




int16_t readWord()
{

byte h=Wire.read();

byte l=Wire.read();


return (int16_t)((h<<8)|l);

}





bool initMPU()
{


Wire.begin(SDA_PIN,SCL_PIN);

Wire.setClock(100000);



delay(100);



Wire.beginTransmission(0x68);

if(Wire.endTransmission()==0)
{

mpu_addr=0x68;

}

else
{

Wire.beginTransmission(0x69);

if(Wire.endTransmission()==0)

mpu_addr=0x69;

else

return false;

}



byte id=readRegister(REG_WHO_AM_I);


Serial.print("WHO_AM_I:");

Serial.println(id,HEX);



if(!isKnownMpuId(id))
{

Serial.println("Unknown MPU");

}



writeRegister(REG_PWR_MGMT_1,0x00);


delay(100);


writeRegister(REG_SMPLRT_DIV,0x07);


writeRegister(REG_CONFIG,0x03);


writeRegister(REG_GYRO_CONFIG,0x08);


writeRegister(REG_ACCEL_CONFIG,0x10);



return true;

}




bool readMPU()
{


Wire.beginTransmission(mpu_addr);

Wire.write(REG_ACCEL_XOUT_H);


if(Wire.endTransmission(false)!=0)

return false;



if(Wire.requestFrom(mpu_addr,(byte)14)!=14)

return false;



accelX=readWord();

accelY=readWord();

accelZ=readWord();


readWord();


gyroX=readWord();

gyroY=readWord();

gyroZ=readWord();



return true;

}





//================ 动作检测 =================


void triggerPin(String name)
{

unsigned long now=millis();


if(now-last_trigger_time<COOLDOWN_TIME)

return;



digitalWrite(TRIGGER_PIN,HIGH);


pin_high_start=now;

last_trigger_time=now;


pin_active=true;



Serial.print(name);

Serial.println(" detected");

}







//================ 简单网页 =================


void handleRoot()
{


String html;


html+="<html>";

html+="<meta charset='utf-8'>";

html+="<meta http-equiv='refresh' content='1'>";


html+="<body>";


html+="<h2>MPU6050 Data</h2>";



html+="AX:";
html+=String(accelX);
html+="<br>";

html+="AY:";
html+=String(accelY);
html+="<br>";

html+="AZ:";
html+=String(accelZ);
html+="<br><br>";



html+="GX:";
html+=String(gyroX);
html+="<br>";

html+="GY:";
html+=String(gyroY);
html+="<br>";

html+="GZ:";
html+=String(gyroZ);
html+="<br><br>";



html+="NOD:";
html+=String(nod_speed);

html+="<br>";

html+="SHAKE:";
html+=String(shake_speed);

html+="<br><br>";



html+="PIN3:";


if(pin_active)

html+="HIGH";

else

html+="LOW";



html+="</body>";

html+="</html>";



server.send(
200,
"text/html",
html
);


}





//================ setup =================


void setup()
{


Serial.begin(115200);


pinMode(TRIGGER_PIN,OUTPUT);

digitalWrite(TRIGGER_PIN,LOW);



delay(1000);



Serial.println();

Serial.println("ESP32-C3 MPU6050");



if(!initMPU())
{

Serial.println("MPU FAIL");

while(1);

}



Serial.println("MPU OK");





//=========== WiFi ===========


WiFi.begin(ssid,password);


Serial.print("Connecting WiFi");


while(WiFi.status()!=WL_CONNECTED)
{

delay(500);

Serial.print(".");

}



Serial.println();

Serial.println("WiFi Connected");


Serial.print("IP:");

Serial.println(WiFi.localIP());




//=========== WEB ============


server.on("/",handleRoot);


server.begin();


Serial.println("WEB START");


}





//================ loop =================


void loop()
{


if(!readMPU())
{

Serial.println("MPU ERROR");

return;

}



nod_speed=abs(gyroY/GYRO_SCALE);


shake_speed=abs(gyroZ/GYRO_SCALE);





if(pin_active &&
millis()-pin_high_start>=HOLD_TIME)
{

digitalWrite(TRIGGER_PIN,LOW);


pin_active=false;


Serial.println("PIN3 LOW");

}





if(!pin_active)
{


if(nod_speed>GYRO_THRESHOLD)

triggerPin("NOD");


else if(shake_speed>GYRO_THRESHOLD)

triggerPin("SHAKE");


}






//串口输出

Serial.print("AX:");

Serial.print(accelX);


Serial.print(" AY:");

Serial.print(accelY);


Serial.print(" AZ:");

Serial.print(accelZ);


Serial.print(" GX:");

Serial.print(gyroX);


Serial.print(" GY:");

Serial.print(gyroY);


Serial.print(" GZ:");

Serial.print(gyroZ);



Serial.print(" NOD:");

Serial.print(nod_speed);



Serial.print(" SHAKE:");

Serial.print(shake_speed);



Serial.print(" PIN:");

Serial.println(pin_active?"HIGH":"LOW");





//网页

server.handleClient();



delay(50);

}