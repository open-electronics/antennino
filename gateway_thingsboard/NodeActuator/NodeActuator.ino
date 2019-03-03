/*###########################################################################################*/
/*                                                                                           */
/*                                                                                           */
/*                              ThingsBoard Antennino NodeActuator                           */
/*                                                                                           */
/*                                                                                           */
/*                                  Futura Elettronica                                       */
/*                                                                                           */
/*###########################################################################################*/


// https://arduinojson.org/v5/example/
// https://github.com/bblanchon/ArduinoJson/issues/38



#include <RFM69.h>              
#include <RFM69_ATC.h>         
#include <RFM69_OTA.h>          
#include <SPIFlash.h>           
#include <SPI.h>                
#include <OneWire.h>            
#include <DallasTemperature.h>  
#include <PinChangeInt.h>                
#include <EEPROM.h>
#include "MyTypes.h"
#include <ArduinoJson.h>



//*********************************************************************************************
#define DEBUG_MODE true 
   

#define SERIAL_EN TRUE
#define SERIAL_BAUD 115200

#define SENSORS_POWER PD4
#define TRIGGER_INPUT PD5

// OUTPUT PIN
#define LED 9 



// ANALOGIC INPUT 
#define BATT_MONITOR0 A0 
#define LDR_INPUT A1

#define ONE_WIRE_BUS PD6 




//****************************************************************************************************************
//                                         PARAMETRI FIRMWARE                                                   
//
// FAMILY:          definisce la famiglia del dispositivo
// FIRMWAREVERSION: definisce la versione corrente del firmware 
//****************************************************************************************************************

// Da settare a true solo al primo caricamento dello schetch oppure quando si vuole scrivere forzatamente i dati in EEPROM
bool firstLoad = true;



byte FAMILY =1;
byte FIRMWAREVERSION  = 1;
//****************************************************************************************************************




//****************************************************************************************************************
//     IMPORTANT RADIO SETTINGS - YOU MUST CHANGE/CONFIGURE TO MATCH YOUR HARDWARE TRANSCEIVER CONFIGURATION!
//****************************************************************************************************************

// Deve essere univoco per ogni nodo nella rete
// range:  1-252   ( 255 è riservato per Broadcast, 253  è il Gateway Dati  254 è il Gateway OTA)
byte NODEID  =  1;    
 
 
// Deve essere lo stesso valore per tutti i nodi che devono comunicare
// range: 1-254
byte NETWORKID = 1;  

// Definisco quale dispositivo è il Gateway DATA
byte DATA_GATEWAY_ID = 253;

// Definisco quale dispositivo è il Gateway OTA
byte OTA_GATEWAY_ID = 254;


// Definisce la frequenza del modulo
#define FREQUENCY RF69_433MHZ

// Definisce la password di criptazione.  
// Deve essere la stessa per tutti i nodi della rete
#define ENCRYPTKEY  "sampleEncryptKey" 

// Specifica di usare il meccanismo di autonegoziazione dell potenza di trasmissione ATC
#define ENABLE_ATC 

// SE ATC abilitato definisce il livello minimo di RSSI
#define ATC_RSSI  -80

// Definisce l'intervallo di Acknoledge
#define ACK_TIME   300

//Se setteto a true abilita la modalità Sniffing
bool promiscuousMode = false; 


bool CS_Attributes_Sent = false;
bool packet_sent_correctly = false;
int n_packet_sent=0;



//*********************************************************************************************

int senderID;



#ifdef ENABLE_ATC
  RFM69_ATC radio;
#else
  RFM69 radio;
#endif



// Definisco i parametri per la memoria Flash
#define FLASH_SS 8 


//////////////////////////////////////////
// flash(SPI_CS, MANUFACTURER_ID)
// SPI_CS          - CS pin attached to SPI flash chip (8 in case of Moteino)
// MANUFACTURER_ID - OPTIONAL, 0x1F44 for adesto(ex atmel) 4mbit flash
//                             0xEF30 for windbond 4mbit flash
//                             0x1F65 for AT25F512 ATMEL 512K (65,536 x 8) 
#define MANUFACTURER_ID   0x1F65

// INIZIALIZZO Il Modulo Chip Memory Flash
SPIFlash flash(FLASH_SS, MANUFACTURER_ID); 




boolean requestACK = false;
volatile boolean interrupt_received;
volatile uint16_t Trigger_Interrupt_Count = 0;
boolean TriggerDetected=false;
byte TriggerStatus = 0;


int Pin;




//  INIZIALIZZO DS18B20
OneWire ourWire(ONE_WIRE_BUS);
DallasTemperature sensors(&ourWire);



//Questo è il PayLoad per OTA
PayloadOTA_t dataOTA;



float triggerlevel;
float batteryVolts = 5;
float temperature;
float battery_level;
char input;
byte ackCount=0;
long counter =1;

 
unsigned long previousMillis = 0; 

//Intervallo di invio notifiche
unsigned long interval = 5000; 




void setup() {
  analogReference (DEFAULT);
  Serial.begin(SERIAL_BAUD);

  Serial.println(F("--START SETUP--"));
  Serial.println("");
  pinMode(LED, OUTPUT);
  pinMode(SENSORS_POWER, OUTPUT);


  // Se la EEPROM è stata inizializzata, ricavo i parametri dalla EEPROM altrimenti salvo quelli presenti nel codice nella EEPROM
  if ( (EEPROM.read(0) == 1) && (!firstLoad) ) {
    getparams();
  } else {
    storeparams();
    getparams();
    firstLoad = false;
  }


 #if(DEBUG_MODE)
   delay(2000);
 #endif


  radio.initialize(FREQUENCY,NODEID,NETWORKID);
  radio.encrypt(ENCRYPTKEY);
  radio.promiscuous(promiscuousMode);
  #ifdef ENABLE_ATC
    radio.enableAutoPower(ATC_RSSI);
  #endif
  
 
  //Creo un gestore di interrupt  -> Trigger_allarm
  pinMode(TRIGGER_INPUT, INPUT_PULLUP);
  attachPinChangeInterrupt(TRIGGER_INPUT, Trigger_Interrupt_Function, FALLING);


  #if (DEBUG_MODE)
    Serial.println(F("***********************************"));
    Serial.println(F("SOFTWARE SETTINGS:"));
    Serial.print(F("FAMILY: "));Serial.println (FAMILY);
    Serial.print(F("FIRMWAREVERSION: "));Serial.println (FIRMWAREVERSION);
    Serial.println(F("***********************************"));
    Serial.println("");
  #endif 


  digitalWrite(LED ,HIGH);

   // Da rimuovere in produzione
   #if(DEBUG_MODE)
     delay(1000);
   #endif
   
  Serial.println("");
  Serial.println(F("--END SETUP--"));
  Serial.println("");

  digitalWrite(LED,LOW);



  
  if (flash.initialize()){
    Serial.println(F("SPI Flash Init OK!"));
     //word my_flash_id = flash.readDeviceId();
     //Serial.print("Flash Initialize failed! Here is the ID of my flash:");
     //Serial.println(my_flash_id,HEX);  
  }else{
    Serial.println(F("SPI Flash Init FAIL!"));
  }



  Serial.println("");
  Serial.println(F("--START LOOP--"));
  Serial.println("");

  Serial.flush();
  
}




void loop(){

      //Utile per Il Debugg, per inviare eventuali comandi 
      #if(DEBUG_MODE)
      
      if (Serial.available() > 0){
        char input = Serial.read();
        
        if (input == 'a')  {
          sendCS_Attributes();    
        } 
          
        if (input == 'd')  {
           sendData();    
        }

        if (input == 's')  {
           storeparams();
           getparams();    
        } 
        
      }
      #endif

     unsigned long currentMillis = millis();

     if (radio.receiveDone()){ 
      
         uint8_t DATALEN = radio.DATALEN;
         senderID = radio.SENDERID;
         uint16_t RSSI = radio.RSSI;
         
         char data[RF69_MAX_DATA_LEN+1]; // For the null character
         char *token; 
         
         String key;
         String value;
         String RPC;
         int Pin_Val;
         String Request_Id;
         char buffer[50];
  
          //save packet because it may be overwritten
          memcpy(data, (void *)radio.DATA, DATALEN);
          data[DATALEN] = '\0';  // Terminate the string

          if (radio.ACKRequested()) {      
            radio.sendACK();
            Serial.println(" - ACK sent");   
          }  

          token = strtok(data, ";");

          while( token != NULL ) { 
          String mystring(token); 

          //Serial.print("token: "); 
          //Serial.println(token); 
         
          key   = getValue( mystring,':',0);
          value = getValue( mystring,':',1);

          if (key =="RPC"){
            RPC = value;
          }
          
          if (key == "PIN"){
            Pin = value.toInt();
          }

          if (key =="RI"){
             Request_Id = value;
          }

          if (RPC == "SET_PIN") {
              if (key =="VAL"){
                Pin_Val = value.toInt();
              }     
          }

               
          token = strtok(NULL, ";");   
                    
         }


          #if (DEBUG_MODE)
            Serial.print("RPC:");
            Serial.println(RPC);
            
            Serial.print("PIN:");
            Serial.println(Pin);
            
            Serial.print("VAL:");
            Serial.println(Pin_Val);
            
            Serial.print("RI:");
            Serial.println(Request_Id);
          #endif



          if (RPC == "SET_PIN"){    
            set_gpio_status(Pin, Pin_Val);                       
          }


          if ( ( (RPC == "GET_PIN") || (RPC == "SET_PIN") ) and (Pin != 0) ){
    
             String command = get_gpio_status(Pin,Request_Id); 
                             
             command.toCharArray(buffer, 50);

             Serial.println();
             Serial.println("############################");
             Serial.println("SENDING MQTT RPC RESPONSE");
             Serial.println(buffer); 
                
             if(radio.sendWithRetry(DATA_GATEWAY_ID , buffer, strlen(buffer), 1 )) { 
                Serial.println("RCP RESPONSE-OK");  
             } else {
                Serial.println("RCP RESPONSE-KO");
             }    
             
             Serial.println("############################");
             Serial.println();
               
          }

                  
           // Per ricevere aggiornamenti OTA
           CheckForWirelessHEX(radio, flash, false); 

           radio.receiveDone();
                
      }



   
     if (Trigger_Interrupt_Count >= 1){
        TriggerDetected = true;
        Trigger_Interrupt_Count=0;     
     }

    
     if (TriggerDetected) {           
          TriggerStatus = digitalRead(TRIGGER_INPUT);     
          sendData();   
          #if (DEBUG_MODE)
             Blink(LED,3);
          #endif  
     }  else {    
            if(currentMillis - previousMillis > interval) {
              
              previousMillis = currentMillis; 

              // invio i dati telemetrici
              sendData();  

              // Invio i dati degli attibuti (per una sola volta) del nodo dopo che sono stati inviati i n dati telemetrici
              if (n_packet_sent >=3 && !CS_Attributes_Sent) {
                 //sendCS_Attributes();
              }
                     
            }                              
     }

     Serial.flush();   
     
}





// Gestore Interrupt  Windows
void Trigger_Interrupt_Function() {
  Trigger_Interrupt_Count++;
}



String get_gpio_status(byte pin , String Request_id) {
  // https://www.arduino.cc/en/Reference/PortManipulation
  
  boolean pinvalue = false;
  
  // Prepare gpios JSON payload string
  StaticJsonBuffer<50> jsonBuffer;
  JsonObject& data = jsonBuffer.createObject();

  pinvalue = (*portOutputRegister(digitalPinToPort(pin)) & digitalPinToBitMask(pin));
  //Serial.print(F("PIN VALUE: "));
  //Serial.println(pinvalue);
  
  data["DT"] = "RPC_R";
  data["PIN"] = pin;
  data["VAL"] = pinvalue ? true : false; 
  data["RI"] = Request_id;
  
  char payload[50]; 
  data.printTo(payload, sizeof(payload)); 
  String strPayload = String(payload);
  return strPayload;
}


void set_gpio_status(int pin, boolean enabled) {
    // Output GPIOs state
    digitalWrite(pin, enabled ? HIGH : LOW);  
}


String getValue(String data, char separator, int index){
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;
  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}




void Blink(byte PIN, int DELAY_MS){
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN,LOW);
}



float ReadTemperature(){
  float temp;
  digitalWrite(SENSORS_POWER, false); 
  sensors.begin();
  sensors.requestTemperatures();
  temp = sensors.getTempCByIndex(0); 
  digitalWrite(SENSORS_POWER, true);
  return temp; 
}


int ReadLux(){ 
  digitalWrite(SENSORS_POWER, false);
  int v = analogRead(LDR_INPUT);
  digitalWrite(SENSORS_POWER, false);
  return v;
}


float ReadBatteryLevel() {
  int level = 0;
  float result = 0.0;
  for (byte i = 0; i<10; i++) {
    level = level + analogRead(BATT_MONITOR0);  
  }
  result = (level / 10.0) * 0.01066;
  return result;
}




void sendData() {
   // Info: https://arduinojson.org/v5/api/jsonobject/begin_end/   
   double B,T,L;
   B = ReadBatteryLevel();
   T = ReadTemperature();  
   L = ReadLux();
 
   DynamicJsonBuffer jsonBuffer_Build(100);
   JsonObject& json_buld_data = jsonBuffer_Build.createObject();
   json_buld_data["DT"] = "DATA";
   json_buld_data["BAT"] = B;
   json_buld_data["T"] = T;
   json_buld_data["L"] = L;
   char payload[100]; 
   json_buld_data.printTo(payload, sizeof(payload));
   String strPayload = String(payload);

   #if (DEBUG_MODE)
    Serial.print("Sending Data: ");
    Serial.print(payload);
   #endif
   
  if (radio.sendWithRetry(DATA_GATEWAY_ID, payload, strlen(payload), 1)){
    #if (DEBUG_MODE)
      Serial.println(" -> Reply OK!");    
     #endif
     packet_sent_correctly = true; 
     n_packet_sent++;
     
     TriggerDetected = false;      
  }else { 
     #if (DEBUG_MODE)
       Serial.println(" -> NO reply...");
       packet_sent_correctly = false;
     #endif 
  }
  
}



void sendCS_Attributes() {
   // Info: https://arduinojson.org/v5/api/jsonobject/begin_end/   
   DynamicJsonBuffer jsonBuffer_Build(100);
   JsonObject& json_buld_data = jsonBuffer_Build.createObject();
   json_buld_data["DT"] = "CS_ATTR";
   json_buld_data["id"] = NODEID;
   json_buld_data["type"] = FAMILY;
   json_buld_data["fw"] = FIRMWAREVERSION;
   //json_buld_data["JEDEC"] = FLASH_JEDEC;   // TODO: Modificare la libreria SPIFlash per leggere il parametro JEDEC  completo
   char payload[100]; 
   json_buld_data.printTo(payload, sizeof(payload));
   String strPayload = String(payload);

   #if (DEBUG_MODE)
    Serial.print("Sending CS_Attributes: ");
    Serial.print(payload);
   #endif
   
  if (radio.sendWithRetry(DATA_GATEWAY_ID, payload, strlen(payload), 1)){
    CS_Attributes_Sent = true;
    #if (DEBUG_MODE)
      Serial.println(" -> Reply OK!");    
     #endif
  }else { 
     #if (DEBUG_MODE)
       Serial.println(" -> NO reply...");
     #endif 
  }
  
}





void sendOTARequest() {
  //fill in the struct with new values
  dataOTA.family = FAMILY;
  dataOTA.fw_version = FIRMWAREVERSION;

  #if(DEBUG_MODE)
    Serial.print("Sending struct (");
    Serial.print(" PAYLOAD: [");
    Serial.print(" family=");
    Serial.print(dataOTA.family);
    Serial.print(" fw_version=");
    Serial.print(dataOTA.fw_version);
    Serial.print("]");
    Serial.print("  (");
    Serial.print(sizeof(dataOTA));
    Serial.print(" bytes) ");
  #endif

  if (radio.sendWithRetry(OTA_GATEWAY_ID, (const void*)(&dataOTA), sizeof(dataOTA))) {    
     #if (DEBUG_MODE)
      Serial.println(" -> Reply OK!");    
     #endif   
  }else {
    #if (DEBUG_MODE)
     Serial.println(" -> NO reply..."); 
	#endif
  }

}



void storeparams() {
  EEPROM.write(0,1);
  EEPROM.write(1,NODEID); 
  EEPROM.write(2,NETWORKID); 
  EEPROM.write(3,DATA_GATEWAY_ID ); 
  EEPROM.write(4,OTA_GATEWAY_ID);
  
  #if (DEBUG_MODE)
   Serial.println(F("STORE PARAMETERS IN EEPROM"));  
  #endif
}


void getparams() { 
  NODEID = EEPROM.read(1); 
  NETWORKID = EEPROM.read(2); 
  DATA_GATEWAY_ID  = EEPROM.read(3); 
  OTA_GATEWAY_ID = EEPROM.read(4); 
  
  #if (DEBUG_MODE)
    Serial.println(F("***********************************"));
    Serial.println(F("GET PARAMETERS FROM EEPROM:"));
    Serial.print(F("NODEID:"));Serial.println (NODEID);
    Serial.print(F("NETWORKID:"));Serial.println (NETWORKID);
    Serial.print(F("DATA_GATEWAY_ID:"));Serial.println (DATA_GATEWAY_ID);
    Serial.print(F("OTA_GATEWAY_ID:"));Serial.println (OTA_GATEWAY_ID);
    Serial.println(F("***********************************"));
    Serial.println("");
  #endif    
}
