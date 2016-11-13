/*
  06/01/2016
  Author: Makerbro
  Platforms: ESP8266
  Language: C++
  File: HelloOLED.ino
  ------------------------------------------------------------------------
  Description: 
  Demo for OLED display showcasing writing text to the screen.
  ------------------------------------------------------------------------
  Please consider buying products from ACROBOTIC to help fund future
  Open-Source projects like this! We'll always put our best effort in every
  project, and release all our design files and code for you to use. 
  https://acrobotic.com/
  ------------------------------------------------------------------------
  License:
  Released under the MIT license. Please check LICENSE.txt for more
  information.  All text above must be included in any redistribution. 
*/
#include <Wire.h>
#include <ACROBOTIC_SSD1306.h>
#include <NTPClient.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>
#define USE_SERIAL Serial

ESP8266WiFiMulti WiFiMulti;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);


char* _ssid = "test";
uint _dummyUsage = 3300;

void setProgress(uint progress)
{
  uint maxLength = 14;
  String lengthString = String(progress);
  for(uint8_t t = 0; t <= maxLength; t++) {
    oled.setTextXY(5,t);
      if( t < progress ) {
        oled.putString(".");
      }
      else {
        oled.putString(" ");
      }
  }
}

void connectWifi(char* ssid, char* key) {
  oled.clearDisplay();              // Clear screen
  oled.setTextXY(0,0);              // Set cursor position, start of line 0
  oled.putString("Wifi connecting");
  oled.setTextXY(1,0);              // Set cursor position, start of line 0
  oled.putString("SSID: ");
  oled.putString(ssid);
  WiFiMulti.addAP(ssid, key);
  uint progress = 0;
  while((WiFiMulti.run() != WL_CONNECTED)) {
    setProgress(progress);
    progress++;
    if(progress > 14) {
      progress = 0;
    }
    delay(500);
  }
  oled.setTextXY(0,0);              // Set cursor position, start of line 0
  oled.putString("Wifi connected ");
}

void setupClock() {
  oled.setTextXY(1,0);              // Set cursor position, start of line 0
  oled.putString("Setting clock");
  //todo: get the time here
  setProgress(1);
  timeClient.begin();
  timeClient.update();
  delay(1000);
  oled.setTextXY(2,0);              // Set cursor position, start of line 0
  oled.putString("Clock is good");
  delay(1000);
}

void UpdateScreen(double usage, String timestamp, char* ssid, uint lastPushMins) {
  oled.setTextXY(0,0);
  oled.putString("Running");
  oled.setTextXY(2,0);
  oled.putString("      SSID:     ");
  oled.setTextXY(2,12);
  oled.putString(ssid);
  oled.setTextXY(3,0);              
  oled.putString("     Usage:     ");
  oled.setTextXY(3,12);
  oled.putString(String(usage));
  oled.setTextXY(4,0);              
  oled.putString(" Last Push:     ");
  oled.setTextXY(4,12);
  oled.putString(String(lastPushMins));
  oled.putString("m");
  oled.setTextXY(6,1);
  oled.putString(timestamp);
}

void setup()
{
  USE_SERIAL.begin(115200);
  Wire.begin();  
  oled.init();                      // Initialze SSD1306 OLED display
  oled.clearDisplay();              // Clear screen
  oled.setTextXY(0,0);              // Set cursor position, start of line 0
  oled.putString("Starting");
  delay(1000);
  connectWifi("test", "VaranusIsland2");
  delay(1000);
  setupClock();
  oled.clearDisplay();              // Clear screen
}

double getUsageValue() {
  //_dummyUsage++;
  //return _dummyUsage;

  HTTPClient http;
  http.begin("http://10.1.1.14/pvi?rName=AcPower"); //HTTP
  int httpCode = http.GET();

  // httpCode will be negative on error
  if(httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);
      USE_SERIAL.println(HTTP_CODE_OK);

      // file found at server
      if(httpCode == HTTP_CODE_OK) {
          String payload = http.getString();
          USE_SERIAL.println(payload);
          //{"powerL1":0.0089695,"powerL2":0.0000000,"powerL3":0.0000000,"target":0,"status":1}
          StaticJsonBuffer<200> jsonBuffer;
          JsonObject& root = jsonBuffer.parseObject(payload);

  
          // Test if parsing succeeds.
          if (!root.success()) {
            USE_SERIAL.println("There was an error parsing JSON");
            return -1;
          }
          else {
            double powerL1 = root["powerL1"];
            return powerL1;
          }          
      }
  } 
  else {
      return -1;
  }

  http.end();
}



void loop()
{
  //todo: only do the processing if the wireless is still good, else reconnect
  double usage = getUsageValue();
  String timestamp = timeClient.getFormattedTime();
  UpdateScreen(usage, timestamp, _ssid, 5);
  delay(5*1000);
}
