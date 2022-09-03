#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <DHT.h>
#include <BlynkSimpleEsp8266.h>
#include <TimeLib.h>
#include <WidgetRTC.h>
#include <EEPROM.h> 
#include "secrets.h"


#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space
#define DHTPIN 4 //pin gpio 3 in sensor
#define DHTTYPE DHT22   // DHT 22 Change this if you have a DHT11
DHT dht(DHTPIN, DHTTYPE);

SimpleTimer timer;
int relay1 = 13;
int buttonPin = 1;
boolean buttonState = HIGH;
int temperatureVPin = 10;
int humidityVPin = 11;

WidgetTerminal terminal(V12);
WidgetRTC rtc;

//V0 Time Read 1
//V1 Time Read 2
//V2 Time Read 3
//V3 Time Read 4
//V4 Clear Screen
//V6 Virtual Pin Relay 0
//V7 Virtual Pin Relay 1
//V8 Virtual Pin Relay 2
//V9 Virtual Pin Relay 3
//V10 Temperature
//V11 Humidity
//V13 Dump Eprom

// GP5 -> D1 Moisture digital
// GP12 -> D6 Relay 3
// GP16 -> D0 Relay 1 - Pompa
// GP13 -> D7 Relay 4 - Valvola
// GP14 -> D5 Relay 2
// GP4 -> D2 DHT11
// GP2 -> D4 Led

#define RELAY_1 16
#define RELAY_2 14
#define RELAY_3 12
#define RELAY_4 13

int relay_vpin[] = {V6, V7, V8, V9};
int relay_pin[] = {RELAY_1, RELAY_2, RELAY_3, RELAY_4};
byte WEEK_DAYS[] = {0x1, 0x2, 0x4, 0x8,0x10, 0x20, 0x40};
String WEEK_DAYS_STRING[] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};

#define LED 4 

boolean relay_status[4];

char Date[16];
char Time[16];


int wifisignal;
String displaycurrenttimepluswifi;

//V1: Time Input

const char* ssid = STASSID;
const char* password = STAPSK;

void sendWifi() {
  wifisignal = map(WiFi.RSSI(), -105, -40, 0, 100);
}

void clockvalue() // Digital clock display of the time
{

 int gmthour = hour();
  if (gmthour == 24){
     gmthour = 0;
  }
  String displayhour =   String(gmthour, DEC);
  int hourdigits = displayhour.length();
  if(hourdigits == 1){
    displayhour = "0" + displayhour;
  }
  String displayminute = String(minute(), DEC);
  int minutedigits = displayminute.length();  
  if(minutedigits == 1){
    displayminute = "0" + displayminute;
  }  

  displaycurrenttimepluswifi = "                                          Clock:  " + displayhour + ":" + displayminute + "               Signal:  " + wifisignal +" %";
  Blynk.setProperty(V12, "label", displaycurrenttimepluswifi);
  
}


void sendDHT()
{
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
  Blynk.virtualWrite(temperatureVPin, t); // virtual pin 
  Blynk.virtualWrite(humidityVPin, h); // virtual pin 
}


void yourSetup(){
  delay(500);

  pinMode(RELAY_1,OUTPUT);
  pinMode(RELAY_2,OUTPUT);
  pinMode(RELAY_3,OUTPUT);
  pinMode(RELAY_4,OUTPUT);
  pinMode(LED,OUTPUT);
  digitalWrite(RELAY_1, HIGH);  
  digitalWrite(RELAY_2, HIGH);  
  digitalWrite(RELAY_3, HIGH);  
  digitalWrite(RELAY_4, HIGH);  
  digitalWrite(LED, HIGH);

  for (int i=0;i<4;i++) {
    relay_status[i]=1;
  }
  
  Blynk.config(BLYNK_AUTHKEY);

  timer.setInterval(5000, sendDHT);
  timer.setInterval(5000L, clockvalue);  // check value for time
  timer.setInterval(5000L, sendWifi);    // Wi-Fi singal
  timer.setInterval(1000L, checkRelay);
  sendDHT();
  EEPROM.begin(512);  //Initialize EEPROM
  rtc.begin();
  dht.begin();


}

//long now;
void yourLoop(){
  //put the code that needs to run continuously
  Blynk.run();
  timer.run();
}


void setTimer(int address, TimeInputParam param) {
  
  sprintf(Date, "%02d/%02d/%04d",  day(), month(), year());
  sprintf(Time, "%02d:%02d:%02d", hour(), minute(), second());

  TimeInputParam t(param);

  if (t.hasStartTime()) // Process start time
  {

    //In blynk Monday is 2
    //In Arduino Monday is 1
    int weekdays = 0x0;
    for (int i = 1; i <= 7; i++) {
      if (t.isWeekdaySelected(i))  {
        weekdays = weekdays | WEEK_DAYS[i-1];
        terminal.println(String("Day ")+ String(i));
      }
    }
    terminal.println("Storing byte:"+String(weekdays, HEX));
    EEPROM.put(32+address/8,weekdays);

    
    terminal.println(String("Start: ") + t.getStartHour() + ":" + t.getStartMinute() + ":"+ t.getStartSecond());
    long startSeconds = (t.getStartHour() * 3600) + (t.getStartMinute() * 60) + t.getStartSecond();
    EEPROM.put(address,startSeconds);
    terminal.flush();
  } else {
    EEPROM.put(address,-1);  
  }
  if (t.hasStopTime()) // Process stop time
  {
    terminal.println(String("Stop : ") + t.getStopHour() + ":" + t.getStopMinute() + ":"+ t.getStopSecond());
    long stopSeconds = (t.getStopHour() * 3600) + (t.getStopMinute() * 60) + t.getStopSecond();
    EEPROM.put(address + 4,stopSeconds);
    terminal.flush();
  } else {
    EEPROM.put(address + 4,-1);  
  }
  
  EEPROM.commit(); 

  terminal.flush();
}

BLYNK_WRITE(V0)
{  
  setTimer(0, param);
}

BLYNK_WRITE(V1)
{
  setTimer(8, param);
}

BLYNK_WRITE(V2)
{  
  setTimer(16, param);
}

BLYNK_WRITE(V3)
{  
  setTimer(24, param);
}

BLYNK_WRITE(V4)
{  
  terminal.clear();
  terminal.flush();
}

boolean simulate = false;
BLYNK_WRITE(V5)
{  
  terminal.println("Simulate called "+String(param.asInt()));
  simulate = param.asInt() == 1;
  terminal.flush();
}

BLYNK_WRITE(V6)
{  
  terminal.println("V6 Write called "+String(param.asInt()));
  terminal.flush();
  if (!simulate)
    digitalWrite(relay_pin[0],param.asInt());
}

BLYNK_WRITE(V7)
{  
  terminal.println("V7 Write called "+String(param.asInt()));
  terminal.flush();
  if (!simulate)
    digitalWrite(relay_pin[1],param.asInt());
}

BLYNK_WRITE(V8)
{  
  terminal.println("V8 Write called "+String(param.asInt()));
  terminal.flush();
  if (!simulate)
    digitalWrite(relay_pin[2],param.asInt());
}

BLYNK_WRITE(V9)
{  
  terminal.println("V9 Write called "+String(param.asInt()));
  terminal.flush();
  if (!simulate)
    digitalWrite(relay_pin[3],param.asInt());
}

long read_start_time(int relay) {
  long startSeconds = 0l;
  EEPROM.get(relay*8,startSeconds);
  return startSeconds;
} 

long read_stop_time(int relay) {
  long startSeconds = 0l;
  EEPROM.get(relay*8+4,startSeconds);
  return startSeconds;
} 

long getCurrentTimeSeconds() {
  return (hour() * 3600) + (minute() * 60) + second();
}

boolean isStoredDay(int pin, int day) {
  int weekdays;
  EEPROM.get(32+pin,weekdays);
  boolean isDaySelected = weekdays & WEEK_DAYS[day];

  return isDaySelected;
}

void checkRelay() {

  for (int i=0;i<4;i=i+1) {
    
    //relay_status=1 -> relay off
    if (read_start_time(i) != -1 && 
        read_start_time(i) < getCurrentTimeSeconds() &&
        getCurrentTimeSeconds() < read_stop_time(i) &&
        relay_status[i] == 1 &&
        isStoredDay(i,convert_arduino_array_day_index(weekday()))) {
      relay_status[i]=0;

      sprintf(Time, "%02d:%02d:%02d", hour(), minute(), second());
      terminal.println(String(Time)+" Set Relay "+String(i)+" to ON");
      Blynk.virtualWrite(relay_vpin[i], LOW);
      Blynk.syncVirtual(relay_vpin[i]);
      if (!simulate)
        digitalWrite(relay_pin[i],LOW);

    }
    if (read_stop_time(i) != -1 && 
        getCurrentTimeSeconds()> read_stop_time(i)  &&
        relay_status[i] == 0 &&
        isStoredDay(i,convert_arduino_array_day_index(weekday()))) {
          
      relay_status[i]=1;
      
      sprintf(Time, "%02d:%02d:%02d", hour(), minute(), second());
      terminal.println(String(Time)+" Set Relay "+String(i)+" to OFF");
      Blynk.virtualWrite(relay_vpin[i], HIGH);
      Blynk.syncVirtual(relay_vpin[i]);
      if (!simulate)
        digitalWrite(relay_pin[i],HIGH);
    }
  }
  terminal.flush();

}

int convert_arduino_array_day_index(int i) {
  if (i == 1) return 6;
  return i-2;
}

//dump EPROM
BLYNK_WRITE(V13)
{
  terminal.clear();
  terminal.println("Dump values:");
  for (int i=0;i<4;i=i+1) {
    terminal.println("V"+String(i)+": Start time "+String(read_start_time(i)));
    terminal.println("V"+String(i)+": Stop time "+String(read_stop_time(i)));

    //int weekdays;
    //EEPROM.get(32+i,weekdays);
    //terminal.println("V"+String(i)+": Stored days: "+String(weekdays, HEX));
    for (int day=0;day<7;day++) {
      /* boolean isDaySelected = weekdays & WEEK_DAYS[day];
      if (isDaySelected) {
        terminal.println("V"+String(i)+": Selected day "+WEEK_DAYS_STRING[day]);
      }*/
      if (isStoredDay(i,day)) {
        terminal.println("V"+String(i)+": Selected day "+WEEK_DAYS_STRING[day]);
      }
    }
  }
  
  float h = dht.readHumidity();
  terminal.print("Humidify ");
  terminal.println(int(h));
  float t = dht.readTemperature();
  terminal.print("Temperature ");
  terminal.println(int(t));
  
  sprintf(Time, "day: %02d %02d:%02d:%02d", weekday(), hour(), minute(), second());
  terminal.println("Day: "+WEEK_DAYS_STRING[convert_arduino_array_day_index(weekday())]+" Time: "+String(Time));
  terminal.print("Simulate: "+String(simulate));

  terminal.flush();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);

  int cnt = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (cnt++ >= 10) {
      WiFi.beginSmartConfig();
      while (1) {
        delay(1000);
        if (WiFi.smartConfigDone()) {
          Serial.println();
          Serial.println("SmartConfig: Success");
          break;
        }
        Serial.print("|");
      }
    }
  } 
  WiFi.printDiag(Serial);
  
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
      ESP.restart();
  }
  
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  yourSetup();
}  
void loop() {
  ArduinoOTA.handle();
  yourLoop();
}
