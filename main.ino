#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <SFE_BMP180.h>
#include <Wire.h>

#define ALTITUDE 300

float lattitude, longitude;

  // (TX, RX)
SoftwareSerial gpsSerial(4,3);// gps module
SoftwareSerial gsmSerial(10,11); // gsm module
TinyGPSPlus gps;
SFE_BMP180 pressure;

String apn = "web.gprs.mtnnigeria.net";                       //APN
String apn_u = "ppp";                     //APN-Username
String apn_p = "ppp";                     //APN-Password
String url = "http://www.synergiesintpolis.com/school/insert.php";  //URL for HTTP-POST-REQUEST
//String data1;   //String for the first Paramter (e.g. Sensor1)
//String data2;   //String for the second Paramter (e.g. Sensor2)
String lattitudes,longitudes,height,temperature, pre, airquality,humidity;

String P_string;

void bmp_loop();

char status;
double T,P,p0,a;

String all_data_to_send = "";

void setup()
{ 
 Serial.begin(9600);
 while (!Serial);
 gpsSerial.begin(9600);
 gsmSerial.begin(9600);  
 pinMode(A0, INPUT);
 if (pressure.begin())
 {
  Serial.println("BMP180 init success");
 }
 else
 {
  Serial.println("BMP180 init fail (disconnected?)\n\n");
  while(1);
 }
}

void loop()
{
  //get BMP data
  bmp_loop();
  airQuality_loop();

  Serial.println("GPS LOOP");
  gpsSerial.listen();
  gps_loop();

  
  Serial.println("GSM LOOP ");
  gsmSerial.listen();
  gsm_sendhttp();
}

void airQuality_loop()
{
  Serial.println("\nAnalog: ");
  Serial.print(analogRead(A0));
  Serial.println();
}

void bmp_loop()
{
  // Loop here getting pressure readings every 10 seconds.
  
  status = pressure.startTemperature();
  if (status != 0)
  {
    // Wait for the measurement to complete:
    delay(status);
    status = pressure.getTemperature(T);
    if (status != 0)
    {
      Serial.println("temperature: ");
      Serial.print(T,2);
      status = pressure.startPressure(3);
      if (status != 0)
      {
        delay(status);
        status = pressure.getPressure(P,T);
        if (status != 0)
        {
          // Print out the measurement:
          Serial.println(" absolute pressure: ");
          Serial.print(P,2);
          
          p0 = pressure.sealevel(P,ALTITUDE); // set your current altitude above sea level
          Serial.print(" relative (sea-level) pressure: ");
          Serial.print(p0,2);
          Serial.print(" mb, ");
        }
        else Serial.println("error retrieving pressure measurement\n");
      }
      else Serial.println("error starting pressure measurement\n");
    }
    else Serial.println("error retrieving temperature measurement\n");
  }
  else Serial.println("error starting temperature measurement\n");
  delay(5000);  // Pause for 5 seconds. 
}

void gps_loop() {
  while(1)
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
  Serial.print(" Latitude = ");
  Serial.print(lattitude,6);
  Serial.print("° N");
  Serial.print(" Longitude = ");
  Serial.print(longitude,6);
  Serial.println("° E");
  delay(1000);
}

void gsm_sendhttp()
{
  gsmSerial.println("AT");
  runsl();//Print GSM Status an the Serial Output;
  delay(1000); 
  gsmSerial.println("AT+CBC");
  runsl(); 
  gsmSerial.println(F("AT+CGREG?"));
  runsl();
  delay(10000);
  gsmSerial.println(F("AT+CGREG?"));
  runsl();
  delay(200); 
  gsmSerial.println(F("AT+CSCLK?"));
  runsl();
  delay(50);
  gsmSerial.println(F("AT+SAPBR=3,1,Contype,GPRS"));
  runsl();
  delay(100);
  gsmSerial.println("AT+SAPBR=3,1,APN," + apn);
  runsl();
  delay(100);
  gsmSerial.println(F("AT+SAPBR =1,1"));
  runsl();
  delay(100);
  gsmSerial.println(F("AT+SAPBR=2,1"));
  runsl();
  delay(2000);
  gsmSerial.println(F("AT+HTTPINIT"));
  runsl();
  delay(100);
  gsmSerial.println(F("AT+HTTPPARA=CID,1"));
  runsl();
  delay(100);
  gsmSerial.println("AT+HTTPPARA=URL," + url);
  runsl();
  delay(500);
  gsmSerial.println(F("AT+HTTPPARA=CONTENT,application/x-www-form-urlencoded"));
  runsl();
  delay(100);
  gsmSerial.println(F("AT+HTTPDATA=192,10000"));
  runsl();
  delay(100);
  
  gsmSerial.println(
  all_data_to_send="la=" + lattitudes +  "," + "lo=" + longitudes +
  "," + "te=" + temperature + "," + "hu=" + humidity + ","
  + "pre=" + pre + "," + "airQ=" + airquality); 
  
  //gsmSerial.println("la=" + lattitudes +  "," + "lo=" + longitudes + "," + "te=" + temperature + "," + "hu=" + humidity); 
  runsl();
  delay(10000);
  gsmSerial.println(F("AT+HTTPACTION=1"));
  runsl();
  delay(5000);
  gsmSerial.println(F("AT+HTTPREAD"));
  runsl();
  delay(100);
  gsmSerial.println(F("AT+HTTPTERM"));
  runsl(); 
  delay(100);
}

//Print GSM Status
void runsl() {
  while (gsmSerial.available())
  {
    Serial.println(gsmSerial.read());
  }
}
