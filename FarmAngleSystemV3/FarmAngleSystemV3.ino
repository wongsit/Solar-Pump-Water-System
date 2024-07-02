/* Farm Angel System
   20230113
   By Khanchai Wongsit
   Version 3
   + IOT
*/

// Template ID, Device Name and Auth Token are provided by the Blynk.Cloud
// See the Device Info tab, or Template settings

#define BLYNK_TEMPLATE_ID "TMPLWMJ2OJ4h"
#define BLYNK_DEVICE_NAME "Angel FarmCopy"
#define BLYNK_AUTH_TOKEN "e4yCJjpRsAZrREbE1kkGFnIg9q69g4DO"

// Comment this out to disable prints and save space
#define BLYNK_PRINT Serial

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

#include <Wire.h>
#include "DS3231.h"

#include "DHT.h"
RTClib RTC;
DS3231 Clock;

char auth[] = BLYNK_AUTH_TOKEN;

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "@AngleFarm"; //"@AngleFarm";
char pass[] = "0883779930";  //"0883779930";

BlynkTimer timer;

int control_mode = 1; //mode 1=auto, 0=manual
boolean control_val = true; //control true=on val1, false=on val2

#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
float humidity = 0.0;
float temperature = 0.0;
float humidity_prev = 0.0;
float temperature_prev = 0.0;

/* Sensor */
const int ult_trig = D5;  //D5
const int ult_echo = D6;  //D6

/*  Load */
const int pump = D2;   //D2
const int valve = D7;   //D3
const int valve2 = D8;   //D8

/* Water Level (cm.)*/
const int tank_high = 140;
const int water_low = 50;
const int water_high = 110;
long water_level = 0;

String pump_status;
String valve_status;
long BoxTemperature;

/* Function */
long microsecondsToCentimeters(long microseconds);
long check_water(void);
void Timer1(int Tnow, int Ton, int Toff);
void pump_on(void);
void pump_off(void);
void valve_on(void);    //on valve alternative
void valve_off(void);   //off valve alternative
void valve1_on(void);
void valve1_off(void);
void valve2_on(void);
void valve2_off(void);

//Control Valve
// This function is called every time the Virtual Pin 0 state changes
BLYNK_WRITE(V0)
{
  // Set incoming value from pin V0 to a variable
  int value = param.asInt();
  //Valve
  if (value == 1) {
    valve_on();
  } else {
    valve_off();
  } 
}

//Control Pump
// This function is called every time the Virtual Pin 0 state changes
BLYNK_WRITE(V4)
{
  // Set incoming value from pin V0 to a variable
  int value = param.asInt();
  //Pump
  if (value == 1) {
    pump_on();
  } else {
    pump_off();
  } 
}

//Control Mode Auto/Manual
BLYNK_WRITE(V9)
{
  // Set incoming value from pin V0 to a variable
  int value = param.asInt();
  // Update state
  Blynk.virtualWrite(V10, value);
  control_mode = value;
}

// This function is called every time the device is connected to the Blynk.Cloud
BLYNK_CONNECTED()
{
  // Change Web Link Button message to "Congratulations!"
  Blynk.setProperty(V3, "offImageUrl", "https://static-image.nyc3.cdn.digitaloceanspaces.com/general/fte/congratulations.png");
  Blynk.setProperty(V3, "onImageUrl",  "https://static-image.nyc3.cdn.digitaloceanspaces.com/general/fte/congratulations_pressed.png");
  Blynk.setProperty(V3, "url", "https://docs.blynk.io/en/getting-started/what-do-i-need-to-blynk/how-quickstart-device-was-made");
}

// This function sends Arduino's uptime every second to Virtual Pin 2.
void myTimerEvent()
{
  // You can send any value at any time.
  // Please don't send more that 10 values per second.
  //Blynk.virtualWrite(V2, millis() / 1000);
  ESP.wdtFeed();              //Start WDT Count
       
}



void setup () {
  Serial.begin(115200);
  Wire.begin();

  //Initial Load
  Serial.println("Initial...");

  dht.begin();

  Blynk.begin(auth, ssid, pass);
  // You can also specify server:
  //Blynk.begin(auth, ssid, pass, "blynk.cloud", 80);
  //Blynk.begin(auth, ssid, pass, IPAddress(192,168,1,100), 8080);

  // Setup a function to be called every second
  timer.setInterval(1000L, myTimerEvent);

  //INPUT
  pinMode(ult_trig, OUTPUT);
  pinMode(ult_echo, INPUT);
  //OUTPUT
  pinMode(pump, OUTPUT);
  pinMode(valve, OUTPUT);
  pinMode(valve2, OUTPUT);

  Blynk.virtualWrite(V10, 1);  //Set Auto Mode

  pump_off();
  valve1_off();
  valve2_off();

  //Watch Dog Timer HW WDT = 8 sec
  ESP.wdtDisable();           //Disable Software WDT
  //ESP.wdtFeed();              //Start WDT Count
  Serial.println("System Start... ");
}

void loop () {

  Blynk.run();
  timer.run();
  // You can inject your own code or combine it with other sketches.
  // Check other examples on how to communicate with Blynk. Remember
  // to avoid delay() function!

  //Sensor Measuring and send
  temperature = dht.readTemperature();
  Blynk.virtualWrite(V6, temperature);
  humidity = dht.readHumidity();
  Blynk.virtualWrite(V7, humidity);
  water_level = check_water();  /* Check water level */
  Blynk.virtualWrite(V8, water_level);

  DateTime now = RTC.now();  //Check Time
  // BoxTemperature = Clock.getTemperature();
   String dt = String(now.year(), DEC) + "/" + String(now.month(), DEC) + "/" + String(now.day(), DEC) + " " + String(now.hour(), DEC)+ ":" + String(now.minute(), DEC)+ ":" + String(now.second(), DEC) ;
   //Serial.print("Date Time: ");
   //Serial.println(dt);      
   Blynk.virtualWrite(V11, dt);

  //Auto Mode
  if (control_mode == 1) { //Auto
    
    if (now.hour() != 165) {            /* Check RTC Operating if 165 = error */      
           
      if ((now.second() % 10) == 0) {
        Timer1(now.hour(), 8, 13);          /* call Timer1 Control Operation Hour Start, Stop*/

        if (digitalRead(pump) == HIGH) {
          Blynk.virtualWrite(V5, 1);
        } else {
          Blynk.virtualWrite(V5, 0);
        }
        //Status Valve
        if (digitalRead(valve) == HIGH) {
          Blynk.virtualWrite(V1, 1);
        } else {
          Blynk.virtualWrite(V1, 0);
        }     
        //Status Valve2
        if (digitalRead(valve2) == LOW) {
          Blynk.virtualWrite(V12, 1);
        } else {
          Blynk.virtualWrite(V12, 0);
        }      
      } else {
        Serial.println();
        
      }
    } else { /* RTC Not found */
      Serial.println("RTC not found ! ,Off Pump and Valve. please check RTC device");
      pump_off();
      valve_off();

      control_mode = 0; //Manual
      // Update state
      Blynk.virtualWrite(V10, control_mode);
      Blynk.virtualWrite(V11, "RTC not found !");
    }
  }else{
  /* Control pump */
    if (water_level >= water_high) {
      Serial.println(", Pump Off");
      pump_off();
    } 
  }

 // ESP.wdtFeed();              //Re-start HW WDT Count 
}

long microsecondsToCentimeters(long microseconds)
{
  // The speed of sound is 340 m/s or 29 microseconds per centimeter.
  // The ping travels out and back, so to find the distance of the
  // object we take half of the distance travelled.
  return microseconds / 29 / 2;
}


long check_water(void) {
  long duration, cm, water_level;

  digitalWrite(ult_trig, LOW);
  delayMicroseconds(2);
  digitalWrite(ult_trig, HIGH);
  delayMicroseconds(5);
  digitalWrite(ult_trig, LOW);
  pinMode(ult_echo, INPUT);
  duration = pulseIn(ult_echo, HIGH);

  cm =  microsecondsToCentimeters(duration);

  if (cm > 4 && cm < tank_high) {
    water_level = tank_high - cm;
  } else {
    water_level = 0;
  }

  //Serial.print(", Water High: ");
  //Serial.print(water_level);
  //Serial.print(" cm");
  //Serial.println();
  //delay(500);
  return water_level;
}

/* ontrol Pump and Valve */
void Timer1(int Tnow, int Ton, int Toff) {
  if ((Tnow >= Ton) && (Tnow <= Toff)) {
    //Serial.print(", Timer1 On");
    //water_level = check_water();  /* Check water level */
    /* Control Val */
    if ((water_level > (water_high-5))) {
      //Serial.print(", Valve On");
      //valve_on();
      if(Tnow<10){
        valve2_on();
      }else{
        valve1_on();
      }
    }
    if ((water_level < water_low)) {
      //Serial.print(", Valve Off");
      valve_off();
      delay(5000);
    }
    /* Control pump */
    if (water_level >= water_high) {
      //Serial.println(", Pump Off");
      pump_off();
      delay(5000);
    } else {
      //Serial.println(", Pump On");
      pump_on();
    }
  } else {
    //Serial.print(", Timer1 Off");
    water_level = check_water();  /* Check water level */
    if (water_level >= water_low) {
      //Serial.print(", Valve On");
      valve_on();
    } else {
      //Serial.print(", Valve Off");
      valve_off();
      delay(5000);
    }
    //Serial.println(", Pump Off");
    pump_off();
  }
}

void pump_on(void) {
  digitalWrite(pump, HIGH);
  Blynk.virtualWrite(V5, 1);      // Update state
}

void pump_off(void) {
  digitalWrite(pump, LOW);
  Blynk.virtualWrite(V5, 0);      // Update state
}

//on valve alternative
void valve_on(void) {
  if(control_val==true){
    digitalWrite(valve, HIGH); //On Valve    
    Blynk.virtualWrite(V1, 1);      // Update state
  }else{
    digitalWrite(valve2, LOW); //On Valve2    
    Blynk.virtualWrite(V12, 1);           // Update state
  }
}

//off valve alternative
void valve_off(void) {
  if(control_val==true){
    digitalWrite(valve, LOW);        //Off Valve
    control_val=false;              //change valve
    Blynk.virtualWrite(V1, 0);      // Update state
  }else{
    digitalWrite(valve2, HIGH);     //Off Valve2
    control_val=true;               //change valve
    Blynk.virtualWrite(V12, 0);     // Update state
  }
}


void valve1_on(void) {  
    digitalWrite(valve, HIGH); //On Valve    
    Blynk.virtualWrite(V1, 1);      // Update state  
}

void valve1_off(void) {  
    digitalWrite(valve, LOW);        //Off Valve
    Blynk.virtualWrite(V1, 0);      // Update state
}


void valve2_on(void) {  
    digitalWrite(valve2, LOW);        //On Valve2    
    Blynk.virtualWrite(V12, 1);       // Update state
}

void valve2_off(void) {
    digitalWrite(valve2, HIGH);     //Off Valve2
    Blynk.virtualWrite(V12, 0);     // Update state
}
