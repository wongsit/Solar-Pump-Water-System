/* Farm Angel System
 * 20210127
 * By Khanchai Wongsit
 * Version 2.0
 * + IOT
 */

#include <Wire.h>
#include "DS3231.h"

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"


const char* ssid = "@AngleFarm";
const char* password = "0883779930";
const char* mqtt_server = "broker.netpie.io";
const int mqtt_port = 1883;
const char* mqtt_client = "dfa4167f-54ec-4c43-8383-5b3214c2f527";
const char* mqtt_username = "JzfmVm93R4ZJrArRZMXPdFXUkh61iwbq";
const char* mqtt_password = "phSoX!*wFjV18~wqxw$qDeeyS1S01k4X";

WiFiClient espClient;
PubSubClient client(espClient);

char msg[100];

int period = 2000;
unsigned long countTime = 0;

#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
float humidity = 0.0;
float temperature = 0.0;
float humidity_prev = 0.0;
float temperature_prev = 0.0;


RTClib RTC;
DS3231 Clock;

/* Sensor */
const int ult_trig = D5;  //D5
const int ult_echo = D6;  //D6

/*  Load */
const int pump = D2;   //D2
const int valve = D7;   //D3

/* Water Level (cm.)*/
const int tank_high = 140;
const int water_low = 50;
const int water_high = 110;
long water_level = 0;

/* Function */
long microsecondsToCentimeters(long microseconds);
long check_water(void);
void Timer1(int Tnow,int Ton,int Toff);
void pump_on(void);
void pump_off(void);
void valve_on(void);
void valve_off(void);

void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    String message;
    for (int i = 0; i < length; i++) {
        message = message + (char)payload[i];
    }
    Serial.println(message);
    if(String(topic) == "@msg/pump") {
        if(message == "on"){
            pump_on();
            client.publish("@shadow/data/update", "{\"data\" : {\"PumpStatus\" : \"on\"}}");
            Serial.println("Pump on");
        }
        else if (message == "off"){
            pump_off();
            client.publish("@shadow/data/update", "{\"data\" : {\"PumpStatus\" : \"off\"}}");
            Serial.println("Pump off");
        }
    }
}


void reconnect() {
    int count=1;
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection…");
        if (client.connect(mqtt_client, mqtt_username, mqtt_password)) {
            Serial.println("connected");
            client.subscribe("@msg/pump");
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println("try again in 5 seconds");
            delay(5000);
            count++;
        }
        if(count>3) break;
    }
}



void setup () {
  Serial.begin(115200);
  Wire.begin();

  Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");

    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
    dht.begin();

    //INPUT
  pinMode(ult_trig, OUTPUT);
  pinMode(ult_echo, INPUT);  
  //OUTPUT
  pinMode(pump, OUTPUT);
  pinMode(valve, OUTPUT);  
}

void loop () {
   String pump_status;
   String valve_status;
   long BoxTemperature;
    
    
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
      BoxTemperature = Clock.getTemperature();
      //BoxTemperature = 0.0;
      //Serial.println();
      //check_water();
  
      if((now.second()%10) == 0){      
        Timer1(now.hour(),8,16);  /* call Timer1 Control Operation */

        if(digitalRead(pump) == HIGH){
           pump_status = "on";
        }else{
          pump_status = "off";
        }

        if(digitalRead(valve) == HIGH){
           valve_status = "on";
        }else{
          valve_status = "off";
        }
        
        if (!client.connected()) {
          //Initial Load
          Serial.println("Reconnect wifi..");
          reconnect();
        }
    
        /* NETPIE */
        humidity = dht.readHumidity();
        temperature = dht.readTemperature();      
        //String data = "{\"data\": {\"Humidity\":" + String(humidity) + ", \"Temperature\":" + String(temperature) + ", \"PumpStatus\":\"" + pump_status + "\", \"WaterLevel\":" + water_level + ", \"BoxTemperature\":" + BoxTemperature + "}}";
        String data = "{\"data\": {\"Humidity\":" + String(humidity) + ", \"Temperature\":" + String(temperature) + ", \"PumpStatus\":\"" + pump_status+ "\", \"ValveStatus\":\"" + valve_status + "\", \"WaterLevel\":" + water_level + "}}";
        //Send                
        Serial.println(data);
        data.toCharArray(msg, (data.length() + 1));
        client.publish("@shadow/data/update", msg);               
      }else{        
        Serial.println();        
      }
    }else{  /* RTC Not found */
      Serial.println("RTC not found ! ,Off Pump and Valve. please check RTC device");
      pump_off();
      valve_off();

      if(digitalRead(pump) == LOW){
           pump_status = "on";
        }else{
          pump_status = "off";
        }

        if(digitalRead(valve) == HIGH){
           valve_status = "on";
        }else{
          valve_status = "off";
        }
        
        if (!client.connected()) {
          //Initial Load
          Serial.println("Reconnect wifi..");
          reconnect();
        }
    

      /* NETPIE */
        humidity = dht.readHumidity();
        temperature = dht.readTemperature();      
        //String data = "{\"data\": {\"Humidity\":" + String(humidity) + ", \"Temperature\":" + String(temperature) + ", \"PumpStatus\":\"" + pump_status + "\", \"WaterLevel\":" + String(water_level) + ", \"BoxTemperature\":" + String(BoxTemperature) + "}}";
        String data = "{\"data\": {\"Humidity\":" + String(humidity) + ", \"Temperature\":" + String(temperature) + ", \"PumpStatus\":\"" + pump_status+ "\", \"ValveStatus\":\"" + valve_status + "\", \"WaterLevel\":" + water_level + "}}";
        //Send                
        Serial.println(data);
        data.toCharArray(msg, (data.length() + 1));
        client.publish("@shadow/data/update", msg);    
    }
    client.loop(); 
  }//time 
  client.loop();
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
