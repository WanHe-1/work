extern unsigned long HX711_Read(void);
extern long Get_Weight();

float Weight = 0;
int HX711_SCK = 2;
int HX711_DT = 3;
long HX711_Buffer = 0;
long Weight_Maopi = 0, Weight_Shiwu = 0;
#define GapValue 405

void setup()
{
  pinMode(HX711_SCK, OUTPUT);
  pinMode(HX711_DT, INPUT);

  Serial.begin(9600);
  delay(3000);
  Weight_Maopi = HX711_Read();
}

void loop()
{
  Weight = Get_Weight();
  Serial.print("Weight:");
  Serial.println(float(Weight / 1000), 3);
  delay(2000);
}

long Get_Weight()
{
  HX711_Buffer = HX711_Read();
  Weight_Shiwu = HX711_Buffer;
  Weight_Shiwu = Weight_Shiwu - Weight_Maopi;
  Weight_Shiwu = (long)((float)Weight_Shiwu / GapValue);
  return Weight_Shiwu;
}

unsigned long HX711_Read(void)
{
  unsigned long count;
  unsigned char i;

  digitalWrite(HX711_DT, HIGH);
  delayMicroseconds(1);
  digitalWrite(HX711_SCK, LOW);
  delayMicroseconds(1);
  count = 0;
  while (digitalRead(HX711_DT));

  for (i = 0; i < 24; i++)
  {
    digitalWrite(HX711_SCK, HIGH);
    delayMicroseconds(1);
    count = count << 1;
    digitalWrite(HX711_SCK, LOW);
    delayMicroseconds(1);
    if (digitalRead(HX711_DT))
      count++;
  }

  digitalWrite(HX711_SCK, HIGH);
  count ^= 0x800000;
  delayMicroseconds(1);
  digitalWrite(HX711_SCK, LOW);
  delayMicroseconds(1);
  return count;
}
