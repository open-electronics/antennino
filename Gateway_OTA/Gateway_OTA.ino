
#include <RFM69.h>          
#include <RFM69_OTA.h>     
#include <RFM69_ATC.h>      
#include <SPIFlash.h>       
#include <SPI.h>            
#include <SerialCommand.h>  
#include "MyTypes.h"


#define TIME_OUT 30000L  
#define DEBUG_MODE false    
#define SERIAL_BAUD 115200
#define LED  9 
#define FLASH_SS  8 


#define P1_INPUT 4
#define P2_INPUT 5
#define LED_V 6
#define LED_R 7
#define TIMEOUT_OTA    3000




// PARAMETRI INTERPRETE COMANDI
#define HELP "HELP"
#define SET_LOAD_LOCAL_FLASH "SET_LOAD_LOCAL_FLASH"
#define SET_DOWNLOAD "SET_DOWNLOAD"
#define SET_UPLOAD "SET_UPLOAD"
#define ERASE_FLASH "ERASE_FLASH"
#define READ_FLASH "READ_FLASH"
#define SET_PARAMETERS "SET_PARAMETERS"
#define SET_TARGET "SET_TARGET"









SPIFlash flash(FLASH_SS, 0xEF30);



byte ackCount=0;
byte datalen;
byte sender;
byte rssi;
byte targetID=0;
int programming = -1;
uint16_t index=0;
char valore[3];
uint8_t i= 0;
uint8_t valoreb;





byte FIRMWAREVERSION;
byte FAMILY;
byte REMOTE_FIRMWAREVERSION;
byte REMOTE_FAMILY;



unsigned long StartTime;
static unsigned long timeLastInput = 0;
long previousMillis = 0;        





//****************************************************************************************************************
//     IMPORTANT RADIO SETTINGS - YOU MUST CHANGE/CONFIGURE TO MATCH YOUR HARDWARE TRANSCEIVER CONFIGURATION!
//****************************************************************************************************************
// Deve essere univoco per ogni nodo nella rete
// ( 255 è riservato per Broadcast, 253  è il Gateway Dati  254 è il Gateway OTA)
byte NODEID  =  254;    

// Deve essere lo stesso valore per tutti i nodi che devono comunicare
// range: 1-254
byte NETWORKID = 1;  

// Definisce la frequenza del modulo
#define FREQUENCY RF69_433MHZ

// Definisce la password di criptazione.  
// Deve essere la stessa per tutti i nodi della rete
#define ENCRYPTKEY  "sampleEncryptKey" 

// Specifica di usare il meccanismo di autonegoziazione della potenza di trasmissione ATC
#define ENABLE_ATC 

// SE ATC abilitato definisce il livello minimo di RSSI
#define ATC_RSSI  -80

// Definisce l'intervallo di Acknoledge
#define ACK_TIME   300

//Se setteto a true abilita la modalità Sniffing
bool promiscuousMode = false; 

//*********************************************************************************************



//definisco la struttura dati per la richiesta OTA
PayloadOTA_t dataOTA;

byte flag = 0;



#ifdef ENABLE_ATC
  RFM69_ATC radio;
#else
  RFM69 radio;
#endif




// Istanzio il gestore dei comandi Seriali
SerialCommand sCmd;   




void setup(){ 
 
  Serial.begin(SERIAL_BAUD);

  pinMode( LED_R, OUTPUT);
  pinMode( LED_V, OUTPUT);
  
  digitalWrite(LED_V, LOW);
  digitalWrite(LED_R, HIGH);


  Serial.println(F("ENTER SETUP"));

  radio.initialize(FREQUENCY,NODEID,NETWORKID);
  radio.encrypt(ENCRYPTKEY);
  radio.promiscuous(promiscuousMode);

  
  #ifdef ENABLE_ATC
    radio.enableAutoPower(ATC_RSSI);
  #endif

  delay(300);


  if (flash.initialize()){
    Serial.println(F("FLASH INIT OK!"));
  }else{
    Serial.println(F("FLASH INIT FAILED!"));
  }


  Serial.println(F("EXIT  SETUP"));
  Serial.println(F("ENTER LOOP"));
  Serial.println(F("WAIT FOR COMMAND"));
  Serial.println("");

 
  sCmd.addCommand(HELP, showHelp);
  sCmd.addCommand(SET_LOAD_LOCAL_FLASH, setLoadLocalFlash);
  sCmd.addCommand(SET_DOWNLOAD, setDownload);
  sCmd.addCommand(SET_UPLOAD, setUpload);
  sCmd.addCommand(ERASE_FLASH, EraseFLASH);
  sCmd.addCommand(READ_FLASH, read_flash_table);
  sCmd.addCommand(SET_PARAMETERS, set_Parameters); 
  sCmd.addCommand(SET_TARGET, set_Target); 
  sCmd.setDefaultHandler(unrecognized); 
    
  Serial.println(F("Ready"));
  
}





void loop() {

  if (programming == -1){ 
     sCmd.readSerial();
  }

  if (programming == 0){ 
      read_flash_table(); 
      programming = -1 ;     
  }

  if (programming == 1) {  
      getFlasData();      
  }

  if (programming == 2) {  
      HandleOTARequest();      
  }

  if (programming == 3) {  
      SendOTAUpdate();      
  }

}







void showHelp(){
Serial.println(F("Commands available:"));
Serial.println("");
Serial.println(F(HELP));
Serial.println(F(SET_LOAD_LOCAL_FLASH));
Serial.println(F(SET_DOWNLOAD));
Serial.println(F(SET_UPLOAD));
Serial.println(F(ERASE_FLASH));
Serial.println(F(READ_FLASH));
Serial.println(F(SET_PARAMETERS));
Serial.println(F(SET_TARGET));
}






void getFlasData(){
  
    while (1==1) {
    
      timeLastInput = millis();
      
      if(Serial.available() > 0) {   
          timeLastInput = StartTime;
      
          // get the new byte:
          char inChar = Serial.read(); 
      
          if (int(inChar) != 10){
            valore[i]= inChar;
            i++;
          }
          
          if (int(inChar) == 10) {   
            valoreb = atoi(valore);  
            index++;
            i=0;  
            flash.writeByte(index, valoreb);   
          }  
      }
       
       if(timeLastInput - StartTime > TIME_OUT) {
            Serial.println(F("Time out!"));
            Serial.println(F("Now Ready for other commands"));
            
            index = 0;
            programming = -1;
            break;
       }
            
    }

}





void SendOTAUpdate(){
      // N.B. In questa porzione di codice non devo fare operazioni critiche dal punto di vista computazionale e NESSUN delay !!!!


      if (!ProgramProcessIsComplete()){  
        CheckForFlashHEX(flash, radio, targetID, TIMEOUT_OTA, ACK_TIME, false);
      }

   
      if (radio.ACKRequested()) {  
          radio.sendACK();
          Serial.print(F(" - > ACK sent."));         
          Serial.println();
      }
                     
     
      Serial.flush();
    
}





void HandleOTARequest(){

unsigned long currentMillis = millis();

if (radio.receiveDone()){ 

           if (radio.DATALEN == sizeof(PayloadOTA_t)) {  

             if (!ProgrammerIsBusy()){
         
                      previousMillis = currentMillis;  
          
                      // N.B. In questa porzione di codice non devo fare operazioni critiche dal punto di vista computazionale e NESSUN delay !!!!
                      // salvo il PayLoad in una variabile e la visualizzo dopo  radio.sendACK();
                      
                      //assume radio.DATA actually contains our struct and not something else
                      dataOTA = *(PayloadOTA_t*)radio.DATA; 
        
                      // N.B. il TargetID è il sender del messaggio che ha richiesto OTA
                      targetID = radio.SENDERID;
        		
                      byte myOwnBuffer[radio.DATALEN];
                      memcpy(myOwnBuffer, (const void *)radio.DATA, radio.DATALEN);
                      Cast_To_Struct(dataOTA , myOwnBuffer);
        
                      // Estraggo dal Payload ricevuto le informazioni 
                      // necessarie per sapere se l'aggiornamento è necessario e possibile
                      REMOTE_FIRMWAREVERSION = dataOTA.fw_version ;
                      REMOTE_FAMILY = dataOTA.family;
        
                   #if (DEBUG_MODE)
                      Serial.print(F("targetID:"));
                      Serial.println(targetID);  
                      Serial.print(F("PAYLOAD: ["));
                      Serial.print(F(" REMOTE_FAMILY="));
                      Serial.print(REMOTE_FAMILY);
                      Serial.print(F(" LOCAL_FAMILY="));
                      Serial.print(FAMILY);     
                      Serial.print(F(" REMOTE_FIRMWAREVERSION="));
                      Serial.print(REMOTE_FIRMWAREVERSION);  
                      Serial.print(F(" LOCAL_FIRMWAREVERSION="));
                      Serial.print(FIRMWAREVERSION);       
                      Serial.println("]"); 
                      Serial.println(""); 
                    #endif
                    
                      if(REMOTE_FAMILY != FAMILY){                   
                           Serial.println(F("FAMILY DO NOT MATCH!"));    
                           flag = false;     
                      }else {              
                         if (FIRMWAREVERSION > REMOTE_FIRMWAREVERSION) {
                             flag = true;  
                             Serial.println(F("UPDATE IN PROGRESS...... PLEASE WAIT!"));                      
                         }else{
                             flag = false;
                             Serial.println(F("UPDATE UNNECESSARY!"));              
                         }         
                      }
        
                   }else{
                  
                      Serial.print(F("Invalid payload: "));
                      Serial.print("[");Serial.print(radio.SENDERID, DEC);Serial.print("] ");     
                      for (byte i = 0; i < radio.DATALEN; i++){
                        Serial.print((char)radio.DATA[i]);
                      }   
                      Serial.print(F(" [RX_RSSI:"));Serial.print(radio.RSSI);Serial.print("]");
                      Serial.println("");
                   }
                         
                
                  if (flag) {   
                      CheckForFlashHEX(flash, radio, targetID, TIMEOUT_OTA, ACK_TIME, false);              
                  }
        
                  if (radio.ACKRequested()) {  
                     radio.sendACK();
                     Serial.print(F(" - > ACK sent."));      
                     Serial.println();
                  }
                  
       }else {
            Serial.print(F("PROGRAMMER IS BUSY SERVING NODE ID:"));
            Serial.println(targetID);
       }

                       
     }

 
     Serial.flush();
     

}




void setLoadLocalFlash(){
  Serial.println(F("SET LOAD LOCAL FLASH"));
  Serial.println(F("Wait for file .HEX to load in FLASH Memory"));
  Serial.print(F("Timeout is: "));
  Serial.print (TIME_OUT/1000);
  Serial.println(F(" seconds"));
  programming =1;
  StartTime = millis();
}



void setDownload(){
  Serial.println(F("SET DOWNLOAD MODE"));
  programming =2;
}



void setUpload(){
  Serial.println(F("SET UPLOAD MODE"));
  programming =3;
}






void EraseFLASH(){
  Serial.print(F("Erasing Flash chip ... "));
  flash.chipErase();
  while (flash.busy());
  Serial.println(F("DONE"));
}



void read_flash_table(){ 
  uint32_t counter = 1;
  byte value;
  long raw_counter = 0;
  Serial.println(F("READING FLASH"));  
  
  do { 
   value = flash.readByte(counter); 
   if (counter == 1){
    Serial.print("N[");
    Serial.print (raw_counter); 
    Serial.print("] ");
   }

   if (value == 10) { 
      raw_counter++;  
      if (flash.readByte(counter+1)!= 255){
        Serial.println("");
        Serial.print("N[");
        Serial.print(raw_counter);
        Serial.print("] "); 
      }        
   } else if (value == 255) {    
      break;      
   } else {
      Serial.print(value);
      Serial.print(" ");             
   } 

    counter++;
    
 } while (1==1); 

 Serial.println("");
 Serial.println("");  
 Serial.println(F("#####################################################################"));  
 Serial.print("Numero Righe: ");  
 Serial.println (raw_counter -1 ); 
 Serial.print("Numero Byte: ");  
 Serial.println (counter -1 ); 
 Serial.println("#####################################################################"); 

}




void Blink(byte PIN, int DELAY_MS){
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN,LOW);
}





void set_Parameters() {
  int aNumber;
  char *arg;
  
  arg = sCmd.next();
  
  if (arg != NULL) {
    // Converts a char string to an integer
    aNumber = atoi(arg);    
    Serial.print("Family is: ");
    FAMILY = aNumber;
    Serial.println(aNumber);
  }else {
    Serial.println(F("No arguments"));
  }

  arg = sCmd.next();
  if (arg != NULL) {
    aNumber = atol(arg);
    Serial.print(F("Version is: "));
    FIRMWAREVERSION = aNumber;
    Serial.println(aNumber); 
  }else {
    Serial.println(F("No second argument"));
  }


 
}



void set_Target() {
  int aNumber;
  char *arg;
  arg = sCmd.next();
  
  if (arg != NULL) {
    // Converts a char string to an integer
    aNumber = atoi(arg);    
    Serial.print(F("Target_ID is: "));
    targetID = aNumber;
    Serial.println(aNumber);
  }else {
    Serial.println(F("No arguments"));
  }
}



void unrecognized(const char *command) {
  Serial.println(F("What?"));
}



template <typename T> unsigned int Cast_To_Struct(T& value, byte buffer[]){
    byte * p = (byte*) &value;
    unsigned int i;
    *p++ = buffer[0];  
    
    for (i = 1; i < sizeof value; i++){
      *p++ = buffer[i];
    }         
    return i; 
}


