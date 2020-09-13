#include <bp35a1.h>

const char * BPWD = "YOUR_B_ROUTE_PWD";
const char * BID = "YOUR_B_ROUTE_ID";
BP35A1 wisun;
#define RXD2 36
#define TXD2 0

void setup() 
{
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  Serial.println("Hello");
  wisun.begin(&Serial2);
  while(!wisun.connect(BID, BPWD))
  {
    Serial.println("Retrying");
  }
  Serial.println("Connected");
}

void loop() 
{
  wisun.get_power();
  delay(10000);
}
