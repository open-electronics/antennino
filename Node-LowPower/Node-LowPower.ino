
#include <RFM69.h>              
#include <RFM69_ATC.h>         
#include <RFM69_OTA.h>          
#include <SPIFlash.h>           
#include <SPI.h>                
#include <OneWire.h>            
#include <DallasTemperature.h>  
#include <PinChangeInt.h>       
#include <LowPower.h>           
#include <EEPROM.h>
#include "MyTypes.h"




//****************************************************************************************************************
//                                         PARAMETRI FIRMWARE                                                   
//
// FAMILY:          definisce la famiglia del dispositivo
// FIRMWAREVERSION: definisce la versione corrente del firmware 
//****************************************************************************************************************
byte FAMILY =1;
byte FIRMWAREVERSION = 1;
//****************************************************************************************************************

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


#define BATT_CYCLES  2 


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
byte Sleep_Time = 1;


volatile boolean interrupt_received;
volatile uint16_t Trigger_Interrupt_Count = 0;
boolean TriggerDetected=false;
boolean packet_sent_correctly;
int number_of_attempts=0;
byte TriggerStatus = 0;






//  INIZIALIZZO DS18B20
OneWire ourWire(ONE_WIRE_BUS);
DallasTemperature sensors(&ourWire);



// Questi sono i  PayLoad per i Dati e per OTA
Payload_t dataGateway;
PayloadOTA_t dataOTA;



float triggerlevel;
float batteryVolts = 5;
float temperature;
float battery_level;
char input;
byte ackCount=0;
long counter =1;
byte batteryReportCycles = 0;
 




void setup() {
  
  analogReference (DEFAULT);
  Serial.begin(SERIAL_BAUD);

  Serial.println(F("--START SETUP--"));
  Serial.println("");
  pinMode(LED, OUTPUT);
  pinMode(SENSORS_POWER, OUTPUT);


  // Se la EEPROM è stata inizializzata, ricavo i parametri dalla EEPROM altrimenti salvo quelli presenti nel codice nella EEPROM
  if (EEPROM.read(0) == 1) {
    getparams();
  } else {
    storeparams();
    getparams();
  }


 #if(DEBUG_MODE)
  //delay(2000);
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
    Serial.print(F("FIRMWAREVERSION: "));Serial.println (FIRMWAREVERSION);
    Serial.print(F("FAMILY: "));Serial.println (FAMILY);
    Serial.println(F("***********************************"));
    Serial.println("");
  #endif 



  digitalWrite(LED ,HIGH);

  // Da rimuovere in produzione
  delay(1000);

  Serial.println("");
  Serial.println(F("--END SETUP--"));
  Serial.println("");


  digitalWrite(LED,LOW);


  if (flash.initialize()){
    Serial.println(F("SPI Flash Init OK!"));
  }else{
    Serial.println(F("SPI Flash Init FAIL!"));
  }


  Serial.println("");
  Serial.println(F("--START LOOP--"));
  Serial.println("");


  Serial.flush();
  
  packet_sent_correctly = false;

}






void loop(){

     if (radio.receiveDone()){  
            
          senderID = radio.SENDERID;
          Serial.print("Sender:");  
          Serial.println(senderID);

          // Per ricevere aggiornamenti OTA
          CheckForWirelessHEX(radio, flash, false); 

          if (radio.ACKRequested()) {      
            radio.sendACK();
            Serial.print(" - ACK sent");   
          }
            
      }


     
      if (!packet_sent_correctly) {
         Serial.println("");
         Serial.println(F("--START TASK--"));    
         sendData();   
      }


     if (Trigger_Interrupt_Count >= 1){
        TriggerDetected = true;
        Trigger_Interrupt_Count=0;     
     }
    


     if (TriggerDetected) {    
          batteryReportCycles =0;        
          TriggerStatus = digitalRead(TRIGGER_INPUT); 
          
          sendData();   

          #if (DEBUG_MODE)
             Blink(LED,3);
          #endif
            
     }  else if (batteryReportCycles >= BATT_CYCLES) {    
        // invio le notifiche sullo stato della batteria solo se sono in modalità LowPower
        if ( Sleep_Time > 0 ) {
          
            sendData();    
             
            #if(DEBUG_MODE)
             Blink(LED,3);
            #endif        
        }
        
        batteryReportCycles=0;     
     }




     if (!TriggerDetected){
       sendOTARequest();
     }



     Serial.flush();
     radio.receiveDone(); 
     if  (Sleep_Time >0) {
       for (byte i=0; i < Sleep_Time; i++) {  
        LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
       }
     }


     batteryReportCycles++;
      
}






// Gestore Interrupt  Windows
void Trigger_Interrupt_Function() {
  Trigger_Interrupt_Count++;
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

  //fill in the struct with new values
  dataGateway.uptime = millis();
  dataGateway.temp = ReadTemperature();
  dataGateway.battery_level = ReadBatteryLevel();
  dataGateway.lux = ReadLux();
  //dataGateway.allarm = TriggerStatus;
  dataGateway.allarm = TriggerDetected;
  
  
  #if (DEBUG_MODE)
    Serial.print("Sending struct (");
    Serial.print(" PAYLOAD: [");
    Serial.print(" uptime=");
    Serial.print(dataGateway.uptime);
    Serial.print(" temp=");
    Serial.print(dataGateway.temp);
    Serial.print(" Battery=");
    Serial.print(dataGateway.battery_level);
    Serial.print(" Lux=");
    Serial.print(dataGateway.lux);
    Serial.print(" Allarm=");
    Serial.print(dataGateway.allarm);
    Serial.print("]");
    
    Serial.print("  (");
    Serial.print(sizeof(dataGateway));
    Serial.print(" bytes) ");
    Blink(LED, 30);
  #endif
  
  if (radio.sendWithRetry(DATA_GATEWAY_ID, (const void*)(&dataGateway), sizeof(dataGateway) , ACK_TIME)) {  
     #if (DEBUG_MODE)
      Serial.println(" -> Reply OK!");    
     #endif
     packet_sent_correctly = true; 
     TriggerDetected = false;      
  }else { 
     #if (DEBUG_MODE)
       Serial.println(" -> NO reply...");
     #endif
     number_of_attempts = number_of_attempts +1;   
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

  if (radio.sendWithRetry(OTA_GATEWAY_ID, (const void*)(&dataOTA), sizeof(dataOTA) , ACK_TIME)) {    
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
