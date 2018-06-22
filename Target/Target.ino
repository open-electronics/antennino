
#include <RFM69.h>         
#include <RFM69_ATC.h>   
#include "RFM69_OTA.h"    
#include <SPIFlash.h>      
#include <SPI.h>           
#include <LowPower.h> 
#include <EEPROM.h>
#include "MyTypes.h"



#define DEBUG_MODE false 



//****************************************************************************************************************
//                                         PARAMETRI FIRMWARE                                                   
//
// FAMILY:          definisce la famiglia del dispositivo
// FIRMWAREVERSION: definisce la versione corrente del firmware 
//****************************************************************************************************************
byte FAMILY =1;
byte FIRMWAREVERSION = 1;
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

// Definisco quale dispositivo è il Gateway Dati
byte DATA_GATEWAY_ID = 253;

// Definisco quale dispositivo è il Gateway OTA
byte OTA_GATEWAY_ID = 254;

// Definisce la frequenza del modulo
#define FREQUENCY RF69_433MHZ

// Definisce la password di criptazione: Deve essere la stessa per tutti i nodi della rete
#define ENCRYPTKEY  "sampleEncryptKey" 

// Specifica di usare il meccanismo di autonegoziazione della potenza di trasmissione ATC
#define ENABLE_ATC 

// SE ATC abilitato definisce il livello minimo di RSSI
#define ATC_RSSI  -80

// Definisce l'intervallo di Acknoledge
#define ACK_TIME   300

//Se setteto a true abilita la modalità Sniffing
bool promiscuousMode = false; 


#define ENABLE_ATC    //comment out this line to disable AUTO TRANSMISSION CONTROL
#define ATC_RSSI      -75

#define ACK_TIME    30

//*********************************************************************************************
//*********************************************************************************************



#define SERIAL_BAUD 115200
  
#define BLINKPERIOD 500

#define LED        9 // LEDs on D9
#define FLASH_SS   8 // FLASH SS on D8


#ifdef ENABLE_ATC
  RFM69_ATC radio;
#else
  RFM69 radio;
#endif


byte flag = 0;
char input = 0;
long lastPeriod = -1;
byte Sleep_Time = 0;



// Definisce il Payload da inviare al Gateway
Payload_t dataGateway;
PayloadOTA_t dataOTA;


SPIFlash flash(FLASH_SS, 0xEF30); //EF30 for windbond 4mbit flash






void setup() {
  
  pinMode(LED, OUTPUT);
  
  Serial.begin(SERIAL_BAUD);


  
  // Se la EEPROM è stata inizializzata, ricavo i parametri dalla EEPROM altrimenti salvo quelli presenti nel codice nella EEPROM
  if (EEPROM.read(0) == 1) {
    getparams();
  } else {
    storeparams();
    getparams();
  }


  delay(2000);


  radio.initialize(FREQUENCY,NODEID,NETWORKID);
  radio.encrypt(ENCRYPTKEY); 
  #ifdef ENABLE_ATC
    radio.enableAutoPower(ATC_RSSI);
  #endif



  Serial.print(F("Start node..."));


  #if defined(DEBUG_MODE)
    Serial.println(F("***********************************"));
    Serial.println(F("SOFTWARE SETTINGS:"));
    Serial.print(F("FIRMWAREVERSION: "));Serial.println (FIRMWAREVERSION);
    Serial.print(F("FAMILY: "));Serial.println (FAMILY);
    Serial.println(F("***********************************"));
  #endif 


  if (flash.initialize()){
     Serial.println(F("SPI Flash Init OK!"));
  }else{
     Serial.println(F("SPI Flash Init FAIL!"));
  }
	
}






void loop(){

   if (flag == 0){
     //sendOTARequest();
   }


   
  if (radio.receiveDone()){
    
    #if defined(DEBUG_MODE)
        Serial.print("Got [");
        Serial.print(radio.SENDERID);
        Serial.print(':');
        Serial.print(radio.DATALEN);
        Serial.print("] > ");
        
        for (byte i = 0; i < radio.DATALEN; i++)
          Serial.print((char)radio.DATA[i], HEX);
        Serial.println();
    #endif
    
    CheckForWirelessHEX(radio, flash, DEBUG_MODE);
    
  }
  
  
  
   if  (Sleep_Time >0) {
       for (byte i=0; i < Sleep_Time; i++) {  
        LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
       }
   }
   
  
 }




void sendOTARequest() {

  //fill in the struct with new values
  dataOTA.family = FAMILY;
  dataOTA.fw_version = FIRMWAREVERSION;

  #if defined(DEBUG_MODE)
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

  
  if (radio.sendWithRetry(OTA_GATEWAY_ID, (const void*)(&dataOTA), sizeof(dataOTA) , ACK_TIME)) {  
     Serial.println(" -> Reply OK!");   
     //flag = 1;
     //Sleep_Time = 0;	 
  }else {
     Serial.println(" -> NO reply..."); 
	   //flag = 0;
     //Sleep_Time = 1;
  }

  Blink(LED, 30);
 
}




void storeparams() {
  EEPROM.write(0,1);
  EEPROM.write(1,NODEID); 
  EEPROM.write(2,NETWORKID); 
  EEPROM.write(3,DATA_GATEWAY_ID ); 
  EEPROM.write(4,OTA_GATEWAY_ID);
  
  #if defined(DEBUG_MODE)
   Serial.println(F("STORE PARAMETERS IN EEPROM"));  
  #endif
}



void getparams() { 
  NODEID = EEPROM.read(1); 
  NETWORKID = EEPROM.read(2); 
  DATA_GATEWAY_ID  = EEPROM.read(3); 
  OTA_GATEWAY_ID = EEPROM.read(4); 
  
  #if defined(DEBUG_MODE)
    Serial.println(F("***********************************"));
    Serial.println(F("GET PARAMETERS FROM EEPROM"));
    Serial.print(F("NODEID:"));Serial.println (NODEID);
    Serial.print(F("NETWORKID:"));Serial.println (NETWORKID);
    Serial.print(F("DATA_GATEWAY_ID:"));Serial.println (DATA_GATEWAY_ID);
    Serial.print(F("OTA_GATEWAY_ID:"));Serial.println (OTA_GATEWAY_ID);
    Serial.println(F("***********************************"));
  #endif    
}




void Blink(byte PIN, int DELAY_MS){
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN,LOW);
}

