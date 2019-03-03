
#include <WiFi.h>
#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <TimeZonedB.h>


//WIFI
#define WIFI_AP "FRITZBOX_2.4GHz"
#define WIFI_PASSWORD "arduinogalileo"
int status = WL_IDLE_STATUS;


WiFiClient wf_client;

TimeZonedB timezone("TimezonedbKey", "Europe/Rome", wf_client , 10000, true);

#define debug true





void InitWiFi(){
  Serial.println("Connecting to AP ...");

  WiFi.begin(WIFI_AP, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("Connected to AP");
}



void reconnect() {
  // Loop until we're reconnected
    if ( WiFi.status() != WL_CONNECTED) {
      WiFi.begin(WIFI_AP, WIFI_PASSWORD);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println("Connected to AP");
    }
}




void gettime(){
  
  
  
  if (timezone.getTimezoneInfo()) {
    Serial.println("####################################");
    
    Serial.print("UNIX TS: [");
    Serial.print(timezone.get_UnixTimestampForThingsBoard());
    Serial.println("]");
    
    Serial.print("Formatted: [");
    Serial.print(timezone.get_FormattedTime());
    Serial.println("]");
    
    Serial.println("####################################");
    Serial.println();
  }

  
}



void setup(){
  Serial.begin(115200);
  delay(10);
  InitWiFi();  
}





void loop(){

   if (Serial.available() > 0){
        char input = Serial.read();    
        if (input == 'g')  { 
          gettime();
          Serial.println("g");      
        }                 
   }

   //reconnect();
   
}
