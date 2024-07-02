/* Farm Angel System
 * 20210127
 * By Khanchai Wongsit
 */

#include <Wire.h>
#include "DS3231.h"

RTClib RTC;


/* Sensor */
const int ult_trig = D5;  //D5
const int ult_echo = D6;  //D6

/*  Load */
const int pump = D2;   //D2
const int valve = D7;   //D3

/* Water Level (cm.)*/
const int tank_high = 140;
const int water_low = 50;
const int water_high = 120;

/* Function */
long microsecondsToCentimeters(long microseconds);
long check_water(void);
void Timer1(int Tnow,int Ton,int Toff);
void pump_on(void);
void pump_off(void);
void valve_on(void);
void valve_off(void);


void setup () {
    Serial.begin(57600);
    Wire.begin();

    //INPUT
  pinMode(ult_trig, OUTPUT);
  pinMode(ult_echo, INPUT);  
  //OUTPUT
  pinMode(pump, OUTPUT);
  pinMode(valve, OUTPUT);  
}

void loop () {
  //Initial Load
  Serial.println("Initial...");
  pump_off();
  valve_off();
  delay(5000);
  Serial.println("System Start... ");

    /* ==== MAIN PROGRAM ==== */
  while(true){
    /* Print Time */
    delay(1000);  
    DateTime now = RTC.now(); 
    if(now.hour()!= 165){ /* Check RTC Operating */
      Serial.print("Date Time: ");   
      Serial.print(now.year(), DEC);
      Serial.print('/');
      Serial.print(now.month(), DEC);
      Serial.print('/');
      Serial.print(now.day(), DEC);
      Serial.print(' ');
      Serial.print(now.hour(), DEC);
      Serial.print(':');
      Serial.print(now.minute(), DEC);
      Serial.print(':');
      Serial.print(now.second(), DEC);
      //Serial.println();
      //check_water();
  
      if((now.second()%10) == 0){      
        Timer1(now.hour(),10,15);  /* call Timer1 Control Operation */
      }else{        
        Serial.println();        
      }
    }else{  /* RTC Not found */
      Serial.println("RTC not found ! ,Off Pump and Valve. please check RTC device");
      pump_off();
      valve_off();
    }
  }//time 
}

long microsecondsToCentimeters(long microseconds)
{
  // The speed of sound is 340 m/s or 29 microseconds per centimeter.
  // The ping travels out and back, so to find the distance of the
  // object we take half of the distance travelled.
  return microseconds / 29 / 2;
}


long check_water(void){
 long duration, cm, water_level;   
     
  digitalWrite(ult_trig, LOW);
  delayMicroseconds(2);
  digitalWrite(ult_trig, HIGH);
  delayMicroseconds(5);
  digitalWrite(ult_trig, LOW);
  pinMode(ult_echo, INPUT);
  duration = pulseIn(ult_echo, HIGH);
 
  cm =  microsecondsToCentimeters(duration);

  if(cm > 4 && cm < tank_high){
    water_level = tank_high - cm;
  }else{
    water_level = 0;
  }

  Serial.print(", Water High: ");
  Serial.print(water_level);
  Serial.print(" cm");
  //Serial.println();  
  delay(500);
  return water_level;
}
 /* ontrol Pump and Valve */
void Timer1(int Tnow,int Ton,int Toff){
  long water_level = 0;
  
  if((Tnow >= Ton)&&(Tnow <= Toff)){
    Serial.print(", Timer1 On");  
    water_level = check_water();  /* Check water level */
    /* Control Val */
    if((water_level > water_low+50)){
      Serial.print(", Valve On");
      valve_on();
    }
    if((water_level < water_low)){
      Serial.print(", Valve Off");
      valve_off();
    }
    /* Control pump */
    if(water_level >= water_high){
      Serial.println(", Pump Off");
      pump_off();
    }else{
      Serial.println(", Pump On");
      pump_on();
    }
  }else{
    Serial.print(", Timer1 Off");  
    water_level = check_water();  /* Check water level */  
    if(water_level >= water_low){
      Serial.print(", Valve On");
      valve_on();
    }else{
      Serial.print(", Valve Off");
      valve_off();
    }
    Serial.println(", Pump Off");
    pump_off();
  }
}

void pump_on(void){
  digitalWrite(pump,HIGH);
}

void pump_off(void){
  digitalWrite(pump,LOW);
}

void valve_on(void){
  digitalWrite(valve,HIGH);
}
void valve_off(void){
  digitalWrite(valve,LOW);
}
