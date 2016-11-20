#include <Wire.h>
#include <ACROBOTIC_SSD1306.h>
#include <TimeLib.h>
#include <NtpClientLib.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>

#define YOUR_WIFI_SSID "*"
#define YOUR_WIFI_PASSWD "*"
#define YOUR_PV_KEY =  "*"
#define YOUR_PV_SID =  "*"



int _hasTime = 0;

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

String formatDigits(int digits) {
    String digStr = "";

  if (digits < 10)
    digStr += '0';
  digStr += String(digits);

  return digStr;
}

String getFormattedDate(time_t moment) {
  if ((timeStatus() != timeNotSet) || (moment != 0)) {
    String timeStr = "";

    timeStr += String(year(moment));
    timeStr += formatDigits(month(moment));
    timeStr += formatDigits(day(moment));
    
    return timeStr;
  }
  else return "Date not set";
}

String getFormattedTime(time_t moment) {
  if ((timeStatus() != timeNotSet) || (moment != 0)) {
    String timeStr = "";
    timeStr += formatDigits(hour(moment));
    timeStr += ":";
    timeStr += formatDigits(minute(moment));

    return timeStr;
  }
  else return "Time not set";
}


void connectWifi(char* ssid, char* key) {
  oled.clearDisplay();              // Clear screen
  oled.setTextXY(0,0);              // Set cursor position, start of line 0
  oled.putString("Wifi connecting");
  Serial.println("Wifi connecting");
  oled.setTextXY(1,0);              // Set cursor position, start of line 0
  oled.putString("SSID: ");
  oled.putString(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, key);
  uint progress = 0;
  while (WiFi.status() != WL_CONNECTED) {
    
    setProgress(progress);
    progress++;
    if(progress > 14) {
      progress = 0;
    }
    delay(500);
  }
  oled.setTextXY(0,0);              // Set cursor position, start of line 0
  oled.putString("Wifi connected ");
  Serial.println("Wifi connected");
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

double getVoltageValue() {
  HTTPClient http;
  http.begin("http://10.1.1.14/pvi?rName=GridVoltageAndCurrent"); //HTTP
  int httpCode = http.GET();

  // httpCode will be negative on error
  if(httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);
      Serial.println(HTTP_CODE_OK);

      // file found at server
      if(httpCode == HTTP_CODE_OK) {
          String payload = http.getString();
          Serial.println(payload);
          //{"iGridL1":13.7493382,"iGridL2":0.0000000,"iGridL3":0.0000000,"uGridL1":257.4725342,"uGridL2":0.0000000,"uGridL3":0.0000000,"fGrid":50.0315437}
          StaticJsonBuffer<200> jsonBuffer;
          JsonObject& root = jsonBuffer.parseObject(payload);

  
          // Test if parsing succeeds.
          if (!root.success()) {
            Serial.println("There was an error parsing JSON");
            return -1;
          }
          else {
            double uGridL1 = root["uGridL1"];
            return uGridL1;
          }          
      }
  } 
  else {
      return -1;
  }
  http.end();
}


double getUsageValue() {
  HTTPClient http;
  http.begin("http://10.1.1.14/pvi?rName=AcPower"); //HTTP
  int httpCode = http.GET();

  // httpCode will be negative on error
  if(httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);
      Serial.println(HTTP_CODE_OK);

      // file found at server
      if(httpCode == HTTP_CODE_OK) {
          String payload = http.getString();
          Serial.println(payload);
          //{"powerL1":0.0089695,"powerL2":0.0000000,"powerL3":0.0000000,"target":0,"status":1}
          StaticJsonBuffer<200> jsonBuffer;
          JsonObject& root = jsonBuffer.parseObject(payload);

  
          // Test if parsing succeeds.
          if (!root.success()) {
            Serial.println("There was an error parsing JSON");
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


void setup()
{
    Serial.begin(115200);
    Serial.print("Starting... ");
    Wire.begin();  
    oled.init();                      // Initialze SSD1306 OLED display
    oled.clearDisplay();              // Clear screen
    oled.setTextXY(0,0);              // Set cursor position, start of line 0
    oled.putString("Starting");
    delay(1000);    
    connectWifi(YOUR_WIFI_SSID, YOUR_WIFI_PASSWD);
    delay(1000);
    oled.setTextXY(1,0);              // Set cursor position, start of line 0
    oled.putString("Setting clock");
    setProgress(1);
    
    NTP.onNTPSyncEvent([](NTPSyncEvent_t error) {
        if (error) {
            Serial.print("Time Sync error: ");
            if (error == noResponse)
                Serial.println("NTP server not reachable");
            else if (error == invalidAddress)
                Serial.println("Invalid NTP server address");
        }
        else {
            Serial.print("Got NTP time: ");
            _hasTime = 1;
            oled.setTextXY(2,0);              // Set cursor position, start of line 0
            oled.putString("Clock is good");
            delay(1000);
            Serial.println(NTP.getTimeDateString(NTP.getLastNTPSync()));
        }
    });
    NTP.begin("pool.ntp.org", 8, false);
    NTP.setInterval(1800);
}

int pushToServer(double usage, double voltage, String timeString, String dateString, String sid, String systemId)
{
  HTTPClient http;
  Serial.print("http://pvoutput.org/service/r2/addstatus.jsp?d="+dateString+"&t="+timeString+"&v2="+String(usage)+"&key="+sid+"&sid="+systemId); 
  http.begin("http://pvoutput.org/service/r2/addstatus.jsp?d="+dateString+"&t="+timeString+"&v2="+String(usage)+"&key="+sid+"&sid="+systemId); 
  //
  int resultCode = http.GET();
  if(resultCode == HTTP_CODE_OK) {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] GET... code: %d\n", resultCode);
    return 0;
    
  }
  else {
    Serial.printf("[HTTP] PUSH TO PVOUTPUT failed, error: %s\n", http.errorToString(resultCode).c_str());
    return -1;
  }
}

void loop()
{
    static int i = 0;
    static int lastScreenUpdate = 0;
    static int lastPush = 0;
    int current = millis();
    if ((current - lastScreenUpdate) > 10000) {
      lastScreenUpdate = current;
      Serial.print(NTP.getTimeDateString()); Serial.print(". ");
      Serial.print("WiFi is ");
      Serial.print(WiFi.isConnected() ? "connected" : "not connected"); Serial.print(". ");
      Serial.print("Uptime: ");
      Serial.print(NTP.getUptimeString()); Serial.print(" since ");
      Serial.println(NTP.getTimeDateString(NTP.getFirstSync()).c_str());

      if(_hasTime == 1) {
        Serial.println("I have the time");  
        // Update the screen
        double usage = getUsageValue();
        double voltage = 0;//getVoltageValue();
        time_t currentTime = now();
        String timeStr = getFormattedTime(currentTime);
        String dateStr = getFormattedDate(currentTime);
        UpdateScreen(usage, getFormattedDate(currentTime) + " " +getFormattedTime(currentTime), YOUR_WIFI_SSID, current - lastPush/1000/60);

        Serial.println("Should I push? ");  
        Serial.println(current);  
        Serial.println(lastPush);  
        Serial.println(current - lastPush);  
        // Push every 15 minutes
        if(usage> 0) {
         if(lastPush == 0 || (current - lastPush) > 12*60*1000) {
          Serial.println("Push");  
          int result = pushToServer(usage, voltage, timeStr, dateStr, YOUR_PV_KEY, YOUR_PV_SID);
          if(result >= 0) {
            lastPush = current;
          }
        } 
      }
    }
  }
  
  delay(0);
}
