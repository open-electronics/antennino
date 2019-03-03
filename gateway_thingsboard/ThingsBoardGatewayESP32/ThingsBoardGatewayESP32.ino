// https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/examples/WiFiUDPClient/WiFiUDPClient.ino
// https://github.com/espressif/arduino-esp32/issues/20
// http://www.lucadentella.it/2017/05/11/esp32-17-sntp/
// https://randomnerdtutorials.com/esp32-pinout-reference-gpios/
// http://worldclockapi.com/
// https://timezonedb.com/api   https://github.com/JChristensen/Timezone
// https://ezcmd.com/time-server
// https://desire.giesecke.tk/index.php/2018/01/30/change-partition-size/
// https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/partition-tables.html
// https://www.youtube.com/watch?v=Qu-1RK4Fk7g
// http://www.lucadentella.it/2017/09/30/esp32-22-spiffs/
// https://desire.giesecke.tk/index.php/2018/01/30/change-partition-size/
// http://www.lucadentella.it/2017/10/30/esp32-25-display-oled-con-u8g2/



/*###########################################################################################*/
/*                                                                                           */
/*                                                                                           */
/*                              ThingsBoard Antennino Gateway                                */
/*                                                                                           */
/*                                                                                           */
/*                                  Futura Elettronica                                       */
/*                                                                                           */
/*###########################################################################################*/


#include "User_config.h"

//this needs to be first, or it all crashes and burns...
#include <FS.h>   
                
#include "SPIFFS.h"
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>  
#include <Wire.h> 
#include <RTClib.h>
#include <RFM69.h>
#include <RFM69_ATC.h>
#include <SPI.h>              
#include <stdlib.h>
#include <ArduinoJson.h>
#include <TimeZonedB.h>
#include <Adafruit_GFX.h>
//#include <Adafruit_SSD1306.h>
#include <Adafruit_SH1106.h>


//Indica se siamo collegati al server MQTT almeno una volta
boolean connectedOnce = false; 

WiFiClient eClient;

TimeZonedB timezone("Europe/Rome", eClient, 10000, false);

PubSubClient client(eClient);

RTC_DS3231 rtc;

IPAddress timeServerIP; 

unsigned long prevNTP = 0;
unsigned long lastReconnectAttempt = 0;

IPAddress mqtt_server_ip;
char mqtt_server_ip_str(20); 

int n_mqtt_packet_sent=0;
bool rtc_syncronized = false;
int switchValue;
long port;

byte devices[253];

unsigned long currentMillis;
bool first_sync = false;





// N.B. Verificare il tipo di display che si possiede 
// e prestare massima attenzione alla disposizione dei pin di alimentazione:
// in alcuni casi sono invertiti !!!!!

//Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Adafruit_SH1106 display(OLED_SDA, OLED_SCL);






void setup(){

    Serial.begin(SERIAL_BAUD);
    
    delay(200);
    
    pinMode(WIFI_RESET, INPUT_PULLUP);
    pinMode(GPS_POWER,OUTPUT);
    pinMode(LED_VERDE,OUTPUT);

    ConfigureOLED();
    
    ShowSpashScreen();
    
    delay(2000);


    setup_wifimanager();

    
    setupDS3231(); 

    
    port = strtol(mqtt_port,NULL,10);

    if (mqtt_domain[0] != '\0' ){
        
        WiFi.hostByName(mqtt_domain, mqtt_server_ip);
        
        client.setServer(mqtt_server_ip, port);  

        #if(DEBUG_MODE) 
          Serial.println();  
          Serial.println("##################################");  
          Serial.println("Connecting to MQTT with hostname: ");
          Serial.print(mqtt_domain);
          Serial.print(" --> ");
          Serial.println(mqtt_server_ip.toString());
          Serial.println("##################################");
          Serial.println();  
        #endif   
             
    }else {
        client.setServer(mqtt_server, port);              
        #if(DEBUG_MODE) 
          Serial.println();  
          Serial.println("##################################"); 
          Serial.print("Connecting to MQTT by IP address: ");
          Serial.println(mqtt_server);
          Serial.println("##################################");
          Serial.println();  
        #endif            
    }


    // N.B.  specifica quale funzione deve trattare i dati di callback MQTT
    client.setCallback(MQTTtoRFM69);

    
    delay(1500);
    
    lastReconnectAttempt = 0;
    
    setupRFM69();  

    digitalWrite(LED_VERDE, HIGH);
  
}






void loop(){
 
    currentMillis = millis();
    switchValue = digitalRead(WIFI_RESET);

    if (switchValue == 0) {   
        
      Serial.println("#############################");   
      Serial.println("###### RESET CONFIG #########"); 
      Serial.println("#############################"); 

      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("#####################");
      display.setCursor(0, 10);
      display.println("#                   #");
      display.setCursor(0, 20);
      display.println("#    RESET CONFIG   #");
      display.setCursor(0, 30);
      display.println("#                   #");
      display.setCursor(0, 40); 
      display.println("#####################");
      display.display();

      
      SPIFFS.format();
      WiFi.disconnect(); 
      WiFi.begin("0","0");      
    }



      //Utile per Il Debugg, per inviare eventuali comandi  poi disabilitare in produzione
      if (Serial.available() > 0){
        char input = Serial.read();   
        if (input == 'r')  { 
          client.disconnect();        
        } 
           
        if (input == 'p')  { 
           print_device();        
        }  
         
        if (input == 'c')  { 
           clear_device();       
        } 
         
        if (input == 's')  { 
           Sync_RTC_With_NTP();         
        } 
                      
      }
     

 
    // Non sono In modalità CONFIGURAZIONE
    if (switchValue == 1) { 
    
            // Sincronizzo il RTC con il servizio  NTP  ogni intervalNTP  millisecond           
            if ((currentMillis - prevNTP > intervalNTP) || ( !first_sync) ) {          
                  prevNTP = currentMillis;
                  Sync_RTC_With_NTP();                                  
            }

            rtc_syncronized= true;
             
            if (rtc_syncronized ) {     
                
                  //MQTT client connexion management    
                  if (!client.connected()) {   
                    
                    reconnect();
                    
                    if (currentMillis - lastReconnectAttempt > time_before_retry) {     
                      lastReconnectAttempt = currentMillis;
                      if (reconnect()) {
                        lastReconnectAttempt = 0;
                      }
                    }       
                           
                  } else {        
                       
                     connectedOnce = true;    
                        
                     client.loop();  
                     
                     if(RFM69toMQTT()){      
                        #if(DEBUG_MODE) 
                          Serial.println();  
                          Serial.println("##################################"); 
                          Serial.print("RFM69toMQTT OK");
                          Serial.println("##################################");
                          Serial.println();  
                        #endif        
                     }

  
                    // mostra i parametri dell ESP8266
                    //stateMeasures();  
                   
                  }
                  
            }
            
  }

  
}






void ConfigureOLED(){ 
  
  // N.B. Verificare il tipo di display che si possiede 
  // e prestare massima attenzione alla disposizione dei pin di alimentazione:
  // in alcuni casi sono invertiti !!!!!
  
  //display.begin(SSD1306_SWITCHCAPVCC, 0x3D); // Address 0x3D for 128x64
  
  display.begin(SH1106_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  
  //display.display();
  //delay(2000);
   
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.display();
}




void ShowSpashScreen() {
  display.setCursor(10, 10);
  display.println("Antennino Gateway");
  display.setCursor(23, 25);
  display.println("ThingsBoard");
  display.setCursor(10, 45);
  display.println("Futura Elettronica");
  display.display(); 
}





void Sync_RTC_With_NTP(){
  
  if (timezone.getTimezoneInfo(timezonedb_key)) {
    
    unsigned long epoch;
    
    Serial.println("####################################");
    Serial.print("UNIX TS: [");
    epoch = timezone.get_UnixTimestamp();
    Serial.print(epoch);  
    Serial.println("]");
    
    Serial.print("Formatted: [");
    Serial.print(timezone.get_FormattedTime());
    Serial.println("]"); 
    Serial.println("####################################");
    Serial.println();

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Sync RTC"); 
    display.setCursor(0, 20);
    display.println(timezone.get_FormattedTime());
    display.display(); 
  
    rtc.adjust(epoch); 
      
    rtc_syncronized = true;   

    if (!first_sync){
      delay(2000);
    }
    
    first_sync = true;
       
  }
}




void print_device(){
   for (int i = 0; i < 3; i++){
       Serial.print("VALORE:");
       Serial.print(i);
       Serial.print("=");
       Serial.println(devices[i]);   
    }
}


void clear_device(){
   for (int i = 0; i < 252; i++){
       devices[i]=0;   
   }
}





//callback notifying us of the need to save config
void saveConfigCallback () {
  
  shouldSaveConfig = true;

  display.clearDisplay();
  display.setCursor(10, 0);
  display.println("Configuration");
  display.setCursor(10, 10);
  display.println("Saved");
  
}




boolean reconnect() {
 // https://pubsubclient.knolleary.net/api.html#connect2
  
  int failure_number = 0;

  // Loop until we're reconnected
  while (!client.connected()) { 

      display.clearDisplay();
      display.setCursor(10, 0);
      display.println("Connected to MQTT");
      display.setCursor(10, 10);
      display.println(mqtt_server);   


      // Create a random client ID
      String clientId;
      clientId += String(random(0xffff), HEX);
      if ( client.connect(clientId.c_str(), mqtt_token, NULL) ) { 
        
          #if(DEBUG_MODE)  
                Serial.println();
                Serial.println("############################");  
                Serial.println("CONNECTED TO MQTT BROKER"); 
                Serial.println("############################"); 
                Serial.println(); 
          #endif
   
          display.setCursor(10, 20);
          display.println("Success");
          display.display();


          failure_number = 0;      

         
          if (client.subscribe(subject_rpc)) {                  
             #if(DEBUG_MODE)
                Serial.println();   
                Serial.println("############################");
                Serial.print("SUBSCRIBET TO SUBJECT: "); 
                Serial.println(subject_rpc); 
                Serial.println("############################");
                Serial.println();  
             #endif            
          }   
    
          if (client.subscribe(subject_rpc_request)){
             #if(DEBUG_MODE) 
                Serial.println();  
                Serial.println("############################"); 
                Serial.print("SUBSCRIBET TO SUBJECT: "); 
                Serial.println(subject_rpc_request); 
                Serial.println("############################");
                Serial.println();  
             #endif       
          } 
 
      } else { 
        
          failure_number ++; 

          #if(DEBUG_MODE)
                Serial.println();   
                Serial.println("####################################");  
                Serial.print("CONNESSIONE MQTT FALLITA CODICE ERRORE: "); 
                Serial.println(client.state()); 
                Serial.print("NUMERO TENTATIVI: "); 
                Serial.println(failure_number); 
                Serial.print("NUOVO TENTATIVO RICONNESSIONE TRA: "); 
                Serial.println(time_before_retry);
                Serial.println("####################################"); 
                Serial.println(); 
          #endif 


           display.setCursor(10, 20);
           display.println("MQTT Conn Failed");
           display.setCursor(10, 30);
           display.println("Riconnet in ");
           display.setCursor(10, 40);
           display.println(time_before_retry);
           display.display();

         
           delay(time_before_retry);

          
          if (failure_number > maxMQTTretry){                                
               // if we didn't connected once to mqtt we reset and start in AP mode again to have a chance to change the parameters
               if (!connectedOnce) { 
                   setup_wifimanager();          
               }                      
          }  
          
      }
    
  }

  return client.connected();
  
}





void setup_wifimanager(){

    switchValue = digitalRead(WIFI_RESET);

    
    // Se ho premuto il pulsante di reset Wi-Fi ricofiguro la connessione
    if (switchValue == 0){  
      
      Serial.println("####################################");   
      Serial.println("########## RESET CONFIG ############"); 
      Serial.println("####################################"); 
      delay(2000);


      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("#####################");
      display.setCursor(0, 10);
      display.println("#                   #");
      display.setCursor(0, 20);
      display.println("#    RESET CONFIG   #");
      display.setCursor(0, 30);
      display.println("#                   #");
      display.setCursor(0, 40); 
      display.println("#####################");
      display.display();

   
      SPIFFS.format();
      WiFi.disconnect(); 
      WiFi.begin("0","0");        
    }

   
    //read configuration from FS json
    if (SPIFFS.begin()) {
      
      if (SPIFFS.exists(config_file_path)) {
        
        File configFile = SPIFFS.open(config_file_path, "r");
        
        if (configFile) {
          
          size_t size = configFile.size();
    
          // Allocate a buffer to store contents of the file.
          std::unique_ptr<char[]> buf(new char[size]);
          configFile.readBytes(buf.get(), size);
          DynamicJsonBuffer jsonBuffer;
          JsonObject& json = jsonBuffer.parseObject(buf.get());
          json.printTo(Serial);

          // Trasferisco nelle rispettive variabili il contenuto del file JSON di configurazione
          if (json.success()) {
        
            strcpy(mqtt_server, json["mqtt_server"]);
            strcpy(mqtt_domain, json["mqtt_domain"]);
            strcpy(mqtt_port, json["mqtt_port"]);
            strcpy(mqtt_user, json["mqtt_user"]);
            strcpy(mqtt_pass, json["mqtt_pass"]);
            strcpy(mqtt_token, json["mqtt_token"]); 
            strcpy(timezonedb_key, json["timezonedb_key"]); 
                  
          } else {
               
            #if(DEBUG_MODE)
                Serial.println();   
                Serial.println("####################################");  
                Serial.print("failed to load json config");    
                Serial.println("####################################"); 
                Serial.println(); 
            #endif  
                 
          }
          
        }
        
      }     
  
    } else {
      #if(DEBUG_MODE)
           Serial.println();   
           Serial.println("####################################");  
           Serial.print("failed to mount FS");    
           Serial.println("####################################"); 
           Serial.println(); 
      #endif 
    }

  
    WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 16);
    WiFiManagerParameter custom_mqtt_domain("mqtt_domain", "mqtt domain", mqtt_domain, 40);
    
    WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
    WiFiManagerParameter custom_mqtt_token("token", "TB token", mqtt_token, 40); 
    WiFiManagerParameter custom_mqtt_user("user", "mqtt user", mqtt_user, 40);
    WiFiManagerParameter custom_mqtt_pass("pass", "mqtt pass", mqtt_pass, 40);

    WiFiManagerParameter custom_timezonedb_key("timezonedb_key", "timezonedb_key", timezonedb_key, 40);


    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    wifiManager.setAPCallback(configModeCallback);
    
    //wifiManager.setBreakAfterConfig(true);

  
    //set config save notify callback
    wifiManager.setSaveConfigCallback(saveConfigCallback);

 
    //add all your parameters here
    wifiManager.addParameter(&custom_mqtt_server);
    wifiManager.addParameter(&custom_mqtt_domain);
    wifiManager.addParameter(&custom_mqtt_token);
    wifiManager.addParameter(&custom_mqtt_port);   
    wifiManager.addParameter(&custom_mqtt_user);
    wifiManager.addParameter(&custom_mqtt_pass);
    wifiManager.addParameter(&custom_timezonedb_key);
    


    if (switchValue == 0){
      wifiManager.resetSettings();
    }
    

    //set minimum quality of signal so it ignores AP's under that quality
    wifiManager.setMinimumSignalQuality(MinimumWifiSignalQuality);

  
    //fetches ssid and pass and tries to connect
    //if it does not connect it starts an access point with the specified name
    //and goes into a blocking loop awaiting configuration
    
    if (!wifiManager.autoConnect(Gateway_Name, WifiManager_password)) {
      #if(DEBUG_MODE)
        Serial.println("Failed to connect, we should reset as see if it connects");
      #endif
      delay(3000);
      ESP.restart();
      delay(5000);
    }


    
    #if(DEBUG_MODE) 
       Serial.println(); 
       Serial.println("#################################"); 
       Serial.println("CONNECTED TO AP"); 
       Serial.print("LOCAL IP: "); 
       Serial.println(WiFi.localIP());
       Serial.println("#################################"); 
       Serial.println();
   #endif 


    //Read updated parameters
    strcpy(mqtt_server, custom_mqtt_server.getValue());
    strcpy(mqtt_domain, custom_mqtt_domain.getValue());
    strcpy(mqtt_port, custom_mqtt_port.getValue());
    strcpy(mqtt_token, custom_mqtt_token.getValue()); 
    strcpy(mqtt_user, custom_mqtt_user.getValue());
    strcpy(mqtt_pass, custom_mqtt_pass.getValue());
    strcpy(timezonedb_key, custom_timezonedb_key.getValue());
  
    //Save the custom parameters to FS
    if (shouldSaveConfig) {

      DynamicJsonBuffer jsonBuffer;
      JsonObject& json = jsonBuffer.createObject();

      json["mqtt_server"] = mqtt_server;
      json["mqtt_domain"] = mqtt_domain;
      json["mqtt_port"] = mqtt_port;
      json["mqtt_user"] = mqtt_user;
      json["mqtt_pass"] = mqtt_pass;
      json["mqtt_token"] = mqtt_token;
      json["timezonedb_key"] = timezonedb_key;
      
      File configFile = SPIFFS.open(config_file_path, "w");
      
      if (!configFile) {
        #if(DEBUG_MODE)
           Serial.println();   
           Serial.println("####################################");  
           Serial.print("Failed to open config file for writing");    
           Serial.println("####################################"); 
           Serial.println(); 
        #endif 
      }

      #if(DEBUG_MODE) 
        Serial.println(); 
        Serial.println("#################################"); 
        Serial.println("SAVING CONFIG:"); 
        json.printTo(Serial);
        Serial.println();
        Serial.println("#################################"); 
        Serial.println();
      #endif
      
      json.printTo(configFile); 
       
      configFile.close();
    }

    
}




void configModeCallback (WiFiManager *myWiFiManager) {
  
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Enter APMode");
  display.setCursor(0, 20);
  display.println("Please connect to:");
  display.setCursor(0, 30);
  display.println(myWiFiManager->getConfigPortalSSID());
  display.setCursor(0, 50);
  display.println( WiFi.softAPIP());
  display.display(); 
  
}







void setupDS3231(){ 
  
  if (! rtc.begin()) {
    #if(DEBUG_MODE)
      Serial.println();
      Serial.println("#################################");
      Serial.println("Couldn't find RTC");
      Serial.println("#################################");
      Serial.println();
    #endif
    while (1);
  }


  if (rtc.lostPower()) {
    #if(DEBUG_MODE)
      Serial.println();
      Serial.println("#################################");
      Serial.println("RTC lost power, lets set the time!");
      Serial.println("#################################");
      Serial.println();
    #endif
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  
  }


}







String getRTCDate(){ 
  
   // http://www.onlineconversion.com/unix_time.htm
   // https://www.timecalculator.net/milliseconds-to-date
   
   char buf[50]; 
   DateTime now = rtc.now(); 
   long unix_time;

    #if(DEBUG_MODE)
      Serial.println();
      Serial.println("#################################");  
      Serial.print("RTC TIME: ");   
      char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
      Serial.print(now.year(), DEC);
      Serial.print('/');
      Serial.print(now.month(), DEC);
      Serial.print('/');
      Serial.print(now.day(), DEC);
      Serial.print(" (");
      Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
      Serial.print(") ");
      Serial.print(now.hour(), DEC);
      Serial.print(':');
      Serial.print(now.minute(), DEC);
      Serial.print(':');
      Serial.print(now.second(), DEC);
      Serial.println(); 
      Serial.println("#################################");
      Serial.println();
    #endif
      
    unix_time = now.unixtime();
    
    //Serialln.print(unix_time);
  
    //ltoa((unix_time -7200) , buf ,10);
    
    ltoa((unix_time -3600) , buf ,10);
      
    //ltoa((unix_time) , buf ,10); 
    
    String UnixTime(buf);  
    
    return UnixTime + "000" ;
     
}
