#include <SFE_BMP180.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <Wire.h>

#define ALT_ABOVE_SEA_LEVEL 200
#define RESPONSE_TIME 10000 //ten seconds
#define RESPONSE_TIME_QUICK 1000
#define RETRIES 2
#define RETRIES_QUICK 1

int sendCmdAndWait(const char *cmd, const char *resp, SoftwareSerial object, unsigned int response_time);
void wait_for_bmp_data(void);
void wait_for_gps_data(void);
void fetch_air_quality_data(void);
void emptyBuffer(char *arg); //clear arrays
void showBuffer(char *arg);  //show whatever is in the array
int checkBuffer(char *arg);

String apn = "web.gprs.mtnnigeria.net";
String apn_u = "ppp";                                              //APN-Username
String apn_p = "ppp";                                              //APN-Password
String url = "http://www.synergiesintpolis.com/school/insert.php"; //URL for HTTP-POST-REQUEST
String web_address = ",";
String port = "";

float lattitude, longitude; //returned from gps serial
double _temperature, pressure, pressure0, a; //bmp variables
char status;

String all_data_to_send = "";
char DATA_in[100];

int warning_light = 9;

// (TX, RX)
SoftwareSerial gpsSerial(4, 3);   // gps module
SoftwareSerial gsmSerial(10, 11); // gsm module
TinyGPSPlus gps;
SFE_BMP180 bmp;

void setup()
{
  pinMode(A0, INPUT);
  pinMode(warning_light, OUTPUT);

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
    //Turn on warning_light
    while (1);
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
      Serial.print(_temperature, 2);
      status = bmp.startPressure(3);
      if (status != 0)
      {
        delay(status);
        status = bmp.getPressure(pressure, _temperature);
        if (status != 0)
        {
          // Print out the measurement:
          Serial.println(" absolute pressure: ");
          Serial.print(pressure, 2);
          pressure0 = bmp.sealevel(pressure, ALT_ABOVE_SEA_LEVEL); // set your current altitude above sea level
          Serial.print(" relative (sea-level) pressure: ");
          Serial.print(pressure0, 2);
          Serial.print(" mb, ");
        }
        else
          Serial.println("error retrieving pressure measurement\n");
      }
      else
        Serial.println("error starting pressure measurement\n");
    }
    else
      Serial.println("error retrieving temperature measurement\n");
  }
  else
    Serial.println("error starting temperature measurement\n");
}

void wait_for_gps_data(void)
{
  while (gpsSerial.available() > 0)
  {
    gps.encode(gpsSerial.read());
    if (gps.location.isUpdated())
    {
      lattitude = gps.location.lat();
      longitude = gps.location.lng();
      break;
    }
  }
}

int sendCmdAndWait(const char *cmd, const char *resp, SoftwareSerial object, unsigned int response_time, unsigned int trial)
{
  unsigned long time_now = millis();
  int len = strlen(resp);
  String command(cmd);  //convert C array to C++ string object
  object.print(command);
  while(trial--)
  { 
    while(millis() - time_now < response_time)
    {
      while (object.available())
      {
        int i, index_ = 0;
        char c = object.read();
        DATA_in[index_++] = c; //keeping our response just in case
        i = (c == resp[i]) ? i++ : 0;
        if (i == len)
        {
          return 1; //success
        }
      }
    }
  }
  return 0; //fail
}

int checkBuffer(char *unknown, char *known)
{
  int count = 0;
  for(int i = 0; i < strlen(unknown); i++)
  {
    count = (known[count] == unknown[count]) ? count++ : 0;
    if (count == strlen(known))
    {
      return 1;   //success
    }
  }
  return 0;   //fail
}

int setup_network(void)
{
  if (sendCmdAndWait("AT\n", "OK", gsmSerial, RESPONSE_TIME, RETRIES))
  {
    showBuffer(DATA_in);
    if (sendCmdAndWait("AT+CREG?\n", "OK", gsmSerial, RESPONSE_TIME, RETRIES))
    {
      showBuffer(DATA_in);
      if (sendCmdAndWait("AT+CGATT?\n", "OK", gsmSerial, RESPONSE_TIME, RETRIES))
      {
        showBuffer(DATA_in);
        String next_command = "AT+CSTT=";
        if(sendCmdAndWait((next_command + apn).c_str(),"OK", gsmSerial, RESPONSE_TIME, RETRIES))
        {
          if (sendCmdAndWait("AT+CIICR\n", "OK", gsmSerial, RESPONSE_TIME, RETRIES))
          {
            String next_command = String("AT+CIPSTART=") + String("TCP,") + web_address + port;
            if (sendCmdAndWait(next_command.c_str(),"CONNECT OK", gsmSerial, RESPONSE_TIME, RETRIES))
            {
              showBuffer(DATA_in);
              if(sendCmdAndWait("AT+CIPSEND\n", ">", gsmSerial, RESPONSE_TIME_QUICK, RETRIES_QUICK))
              {
                showBuffer(DATA_in);
                if(sendCmdAndWait(all_data_to_send.c_str(), "SEND OK", gsmSerial, RESPONSE_TIME_QUICK, RETRIES_QUICK))
                {
                  if (sendCmdAndWait("AT+CIPCLOSE=1\n", "CLOSE OK", gsmSerial, RESPONSE_TIME_QUICK, RETRIES_QUICK))
                  {
                    return 1;
                  }
                  else
                  {
                    showBuffer(DATA_in);
                  }       
                }
                else
                {
                  showBuffer(DATA_in);
                }
              }
              else
              {
                showBuffer(DATA_in);
                return 0;
              }  
            }
            else
            {
              showBuffer(DATA_in);
              return 0;
            }          
          }
          else
          {
            showBuffer(DATA_in);
            return 0;
          }
        }
        else
        {
          showBuffer(DATA_in);
          return 0;
        }
      }
      else
      {
        showBuffer(DATA_in);
        return 0;
      }   
    }
    else
    {
      showBuffer(DATA_in);
      return 0;
    }   
  }
  else
  {
    return 0;
  }
}

void emptyBuffer(char *arg)
{
  unsigned int i;
  for (i = 0; i < strlen(arg); i++)
  {
    arg[i] = '\0';
  }
}

void showBuffer(char *arg)
{
  String array_(arg);
  Serial.println(array_);
}
