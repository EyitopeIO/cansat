#include <SFE_BMP180.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <dht11.h>
#include <Wire.h>

#define ALT_ABOVE_SEA_LEVEL 200
#define RESPONSE_TIME 10000 //ten seconds
#define RESPONSE_TIME_QUICK 1000
#define RETRIES 2
#define RETRIES_QUICK 1

byte sendCmdAndWait(String cmd, String *resp, SoftwareSerial object, unsigned int response_time);
void wait_for_bmp_data(void);
void wait_for_gps_data(void);
void fetch_air_quality_data(void);
byte setup_network_and_send(void);
void fetch_solar_radiation_data(void);
void fetch_air_quality_data(void);
void emptyBuffer(char *arg); //clear arrays
void showBuffer(char *arg);  //show whatever is in the array
byte checkBuffer(char *arg);
void parse_data_to_send(void);

const String apn = "web.gprs.mtnnigeria.net";
const String apn_u = "ppp";                                              //APN-Username
const String apn_p = "ppp";                                              //APN-Password
const String url = "http://www.synergiesintpolis.com/school/insert.php"; //URL for HTTP-POST-REQUEST
const String web_address = "";
const String port = "";

float lattitude, longitude; //returned from gps serial
double temperature_, pressure, pressure0, pressure_alt, altitude_; //bmp variables
float humidity_, dewPoint_;   //dht variables
char status;

String all_data_to_send = "";
unsigned char DATA_in[100];

byte problem_light = 9;
byte dht_pin = 8;

bool ready_ = false;

// (TX, RX)
SoftwareSerial gpsSerial(4, 3);   // gps module
SoftwareSerial gsmSerial(10, 11); // gsm module
TinyGPSPlus gps;
SFE_BMP180 bmp;
dht11 dht;

void setup()
{
  pinMode(A0, INPUT); //for air quality sensor
  pinMode(A1, INPUT); //for solar radiation
  pinMode(problem_light, OUTPUT);
  pinMode(dht_pin, INPUT);

  Serial.begin(9600);
  while (!Serial);
  gpsSerial.begin(9600);
  gsmSerial.begin(9600);
  if (bmp.begin())
  {
    Serial.println("BMP180 init success"); 
    wait_for_bmp_data();
    pressure_alt = bmp.getPressure(pressure, temperature_); //needed for altitude calculations
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
  fetch_humidity_and_dewpoint_data();
  fetch_solar_radiation_data();
  fetch_air_quality_data();
  if (ready_ == true)
  {
    parse_data_to_send();
    setup_network_and_send();
  }
  delay(10000);
}

void parse_data_to_send(void)
{
  all_data_to_send = String(longitude) + "," + String(altitude_) +
  "," + String(lattitude) + "," + String(pressure) + 
   "," + String(humidity_) + "," + String(dewPoint_);
}

void wait_for_bmp_data(void)
{

  //Must first get a temperature measurement to perform a pressure reading.
  status = bmp.startTemperature();
  if (status != 0)
  {
    delay(status);
    status = bmp.getTemperature(temperature_);
    if (status != 0)
    {
      delay(status);
      Serial.println("temperature: ");
      Serial.print(temperature_, 2);
      status = bmp.startPressure(3);
      if (status != 0)
      {
        delay(status);
        status = bmp.getPressure(pressure, temperature_);
        if (status != 0)
        {
          delay(status);
          Serial.println(" absolute pressure: ");
          Serial.print(pressure, 2);
          pressure0 = bmp.sealevel(pressure, ALT_ABOVE_SEA_LEVEL); // set your current altitude above sea level
          Serial.print("relative (sea-level) pressure: ");
          Serial.print(pressure0, 2);
          Serial.print(" mb, ");
          if (status != 0)
          {
            delay(status);
            altitude_=bmp.altitude(pressure, pressure_alt);
            ready_ = true;  //successfully gotten all bmp data
          }
          
        }
        else
        {
          ready_ = false;
          Serial.println("error retrieving pressure measurement\n");
        }
      }
      else
      {
        ready_ = false;
        Serial.println("error starting pressure measurement\n");
      }
    }
    else
    {
      ready_ = false;
      Serial.println("error retrieving temperature measurement\n");
    }
  }
  else
  {
    ready_ = false;
    Serial.println("error starting temperature measurement\n");
  }
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
      ready_ = true;
      break;
    }
    else
    {
      ready_ = false;
    }
    
  }
}

byte sendCmdAndWait(String cmd, String resp, SoftwareSerial object, unsigned int response_time, unsigned int trial)
{
  unsigned long time_now = millis();
  int len = resp.length();
  object.print(cmd);
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

byte checkBuffer(char *unknown, char *known)
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

byte setup_network_and_send(void)
{
  if (sendCmdAndWait(F("AT\n"), F("OK"), gsmSerial, RESPONSE_TIME, RETRIES))
  {
    if (sendCmdAndWait(F("AT+CREG?\n"), F("OK"), gsmSerial, RESPONSE_TIME, RETRIES))
    {
      if (sendCmdAndWait(F("AT+CGATT?\n"), F("OK"), gsmSerial, RESPONSE_TIME, RETRIES))
      {
        String next_command = "AT+CSTT=";
        if(sendCmdAndWait(next_command + apn, F("OK"), gsmSerial, RESPONSE_TIME, RETRIES))
        {
          if (sendCmdAndWait(F("AT+CIICR\n"), F("OK"), gsmSerial, RESPONSE_TIME, RETRIES))
          {
            String next_command = String("AT+CIPSTART=") + String("\"TCP,\"") + web_address + "," + port;
            if(sendCmdAndWait(next_command, F("CONNECT OK"), gsmSerial, RESPONSE_TIME, RETRIES))
            {
              if(sendCmdAndWait(F("AT+CIPSEND\n"), F(">"), gsmSerial, RESPONSE_TIME_QUICK, RETRIES_QUICK))
              {
                if(sendCmdAndWait(all_data_to_send, F("SEND OK"), gsmSerial, RESPONSE_TIME_QUICK, RETRIES_QUICK))
                {
                  if (sendCmdAndWait(F("AT+CIPCLOSE=1\n"), F("CLOSE OK"), gsmSerial, RESPONSE_TIME_QUICK, RETRIES_QUICK))
                  {
                    return 1;
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
  emptyBuffer(arg);
}

void fetch_solar_radiation_data(void)
{
}

void fetch_humidity_and_dewpoint_data(void)
{
  int chk = dht.read(dht_pin);
  humidity_ = dht.humidity;
  dewPoint_ == dht.dewPoint();
  Serial.println((float)dht.humidity, 2);
  Serial.println(dht.dewPoint(), 2);
}

void fetch_air_quality_data(void)
{
  
}
