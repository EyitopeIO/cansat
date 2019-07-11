#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <SFE_BMP180.h>
#include <Wire.h>

#define ALT_ABOVE_SEA_LEVEL 300

int sendCmdAndWait(const char* cmd, const char* resp);
void wait_for_bmp_data(void);
void wait_for_gps_data(void);
void fetch_air_quality_data(void);
void emptyBuffer(char* arg);  //clear arrays 
void showBuffer(char* arg);   //show whatever is in the array

String apn = "web.gprs.mtnnigeria.net";                  
String apn_u = "ppp";                     //APN-Username
String apn_p = "ppp";                     //APN-Password
String url = "http://www.synergiesintpolis.com/school/insert.php";  //URL for HTTP-POST-REQUEST

float lattitude, longitude;   //returned from gps serial
char status;
double _temperature,pressure,pressure0,a;  //bmp variables

String all_data_to_send = "";
char DATA_in[100];

// (TX, RX)
SoftwareSerial gpsSerial(4,3);// gps module
SoftwareSerial gsmSerial(10,11); // gsm module
TinyGPSPlus gps;
SFE_BMP180 bmp; 


void setup()
{ 
 pinMode(A0, INPUT);
 
 Serial.begin(9600);
 while (!Serial);
 gpsSerial.begin(9600);
 gsmSerial.begin(9600);  
 if (bmp.begin())
 {
  Serial.println("BMP180 init success");
 }
 else
 {
  Serial.println("BMP180 init fail (disconnected?)\n\n");
  //Turn on some red LED here and await reset.
  while(1);
 }
}

void loop()
{
  wait_for_bmp_data();
  wait_for_gps_data();
  
}




void wait_for_bmp_data(void)
{
  // Loop here getting pressure readings every 10 seconds.
  status = bmp.startTemperature();
  if (status != 0)
  {
    // Wait for the measurement to complete:
    delay(status);
    status = bmp.getTemperature(_temperature);
    if (status != 0)
    {
      Serial.println("temperature: ");
      Serial.print(_temperature,2);
      status = bmp.startPressure(3);
      if (status != 0)
      {
        delay(status);
        status = bmp.getPressure(pressure,_temperature);
        if (status != 0)
        {
          // Print out the measurement:
          Serial.println(" absolute pressure: ");
          Serial.print(pressure,2);
          pressure0 = bmp.sealevel(pressure,ALT_ABOVE_SEA_LEVEL); // set your current altitude above sea level
          Serial.print(" relative (sea-level) pressure: ");
          Serial.print(pressure0,2);
          Serial.print(" mb, ");
        }
        else Serial.println("error retrieving pressure measurement\n");
      }
      else Serial.println("error starting pressure measurement\n");
    }
    else Serial.println("error retrieving temperature measurement\n");
  }
  else Serial.println("error starting temperature measurement\n");
}



void wait_for_gps_data(void)
{
  {
    while(gpsSerial.available() > 0)
    {
      gps.encode(gpsSerial.read());
    }
    if(gps.location.isUpdated())
    {
      lattitude = gps.location.lat();
      longitude = gps.location.lng();
      break;
    }
  }
}

int sendCmdAndWait(const char* cmd, const char* resp)
{
   softUART.write(cmd);
   while(softUART.available())
   {
    int len = strlen(resp);
    int i, data_point = 0;
    char c = softUART.read();
    DATA_in[data_point++] = c;  //keeping our response just in case
    i = (c == resp[i]) ? i++ : 0;
    if(i == len)
    {
      return 1;
    }
    else
    {
      return 0;
    }
   }
   return 0;
}

int setup_network(void)
{
  if (sendCmdAndWait("AT\n", "OK")
  {
    showBuffer(DATA_in);
  }
  else
}

void fetch_air_quality_data(void)
{
  
}


void emptyBuffer(char* arg)
{
  unsigned int i;
  for (i=0;i<strlen(arg);i++)
  {
    arg[i] = '\0';
  }
}

void showBuffer(char* arg)
{
  unsigned int i;
  for(i=0; i < strlen(arg); i++)
  {
    Serial.print(*arg[i]);
  }
}
