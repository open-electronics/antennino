
#include <RFM69.h>              
#include <RFM69_ATC.h>                  
#include <SPI.h>                
#include <Wire.h>              
#include <PinChangeInt.h>      
#include "U8glib.h"            
#include "MyTypes.h"


#define DEBUG_MODE false 
 

//*********************************************************************************************
//************ PARAMETRI DI CONFIGURAZIONE DELLA RETE                              ************
//*********************************************************************************************

// Deve essere univoco per ogni nodo nella rete
// ( 255 è riservato per Broadcast, 253  è il Gateway Dati  254 è il Gateway OTA)
#define NODEID    253    

// Deve essere lo stesso valore per tutti i nodi che devono comunicare
// range: 1-255
#define NETWORKID 1  


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


#define LED 9 
#define SERIAL_EN  TRUE
#define SERIAL_BAUD  115200



#ifdef ENABLE_ATC
  RFM69_ATC radio;
#else
  RFM69 radio;
#endif



byte sendSize=0;
boolean requestACK = false;

byte datalen;
byte sender;
byte rssi;
byte target;
int yPos;
float temperature;
float battery_level;
char input;
byte ackCount=0;
long counter =1;
unsigned long pulscount = 1;
long previousMillis = 0;      
unsigned long delta;




// INIZIALIZZO Il Display:   
// Verificare il Chip e il  pinout del modulo 
// prima di collegarlo ed alimentare il Gateway_Dati

//U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);
U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NO_ACK);



// Definisco la struttura che conterrà i dati ricevuti dai nodi
Payload_t curData; 





void Configure_OLED(){
  //u8g.setRot180(); //flip screen
  // assign default color value
  if ( u8g.getMode() == U8G_MODE_R3G3B2 )
    u8g.setColorIndex(255);     // white
  else if ( u8g.getMode() == U8G_MODE_GRAY2BIT )
    u8g.setColorIndex(3);         // max intensity
  else if ( u8g.getMode() == U8G_MODE_BW )
    u8g.setColorIndex(1);         // pixel on
  else if ( u8g.getMode() == U8G_MODE_HICOLOR )
    u8g.setHiColorByRGB(255,255,255);
  u8g.begin();
}




void Blink(byte PIN, int DELAY_MS){
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN,LOW);
}




void drawData(Payload_t* theData, int rssi, byte from , unsigned long delta) {
  // https://github.com/olikraus/u8glib/wiki/fontsize
	// http://oleddisplay.squix.ch/#/home

	char buff[30];

	u8g.setFont(u8g_font_unifont);

	u8g.setFont(u8g_font_04b_03);

	sprintf(buff, "ID: %d", from);
	u8g.drawStr(0, 10, buff);

	sprintf(buff, "RSSI: %d", rssi);
	u8g.drawStr(30, 10, buff);

	sprintf(buff, "N: %lu", pulscount);
	u8g.drawStr(80, 10, buff);

	sprintf(buff, "BAT: %d.%02d V", (int)theData->battery_level, (int)(theData->battery_level * 100) % 100);
	u8g.drawStr(0, 20, buff);

	sprintf(buff, "TEMP: %d.%02d C", (int)theData->temp, (int)(theData->temp * 100) % 100);
	u8g.drawStr(0, 30, buff);

	sprintf(buff, "LUX: %d", theData -> lux);
	u8g.drawStr(0, 40, buff);

  sprintf(buff, "ALL: %d", theData -> allarm);
  u8g.drawStr(0, 50, buff);

	sprintf(buff, "UP: %lu ms", theData -> uptime);	
	u8g.drawStr(0, 60, buff);

  sprintf(buff, "ET: %d s", delta);  
  u8g.drawStr(70, 60, buff);
  
}




void drawSplashScreen() {
	u8g.drawStr(16, yPos, "ANTENNINO 1.0");
  u8g.drawStr(35, yPos+20, "FUTURA");
  u8g.drawStr(14, yPos+40, "ELETTRONICA");
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






void setup() {
  
  Serial.begin(SERIAL_BAUD);
  pinMode(LED, OUTPUT);
 
  Serial.println(F("--START SETUP--"));
  Serial.println("");
        
  Configure_OLED();

  // Splash Screen
  u8g.setFont(u8g_font_unifont);
  u8g.setColorIndex(1); 
  
  for (yPos = 0; yPos < 18; yPos++) {
	  u8g.firstPage();
	  do {
		  drawSplashScreen();
	  } while (u8g.nextPage());
  }


  delay(1000);

  radio.initialize(FREQUENCY,NODEID,NETWORKID);
  radio.encrypt(ENCRYPTKEY);
  radio.promiscuous(promiscuousMode);

  #ifdef ENABLE_ATC
    radio.enableAutoPower(ATC_RSSI);
  #endif

  Serial.print("Start node...");
  Serial.print("Node ID = ");
  Serial.println(NODEID);


  digitalWrite(LED ,HIGH);
  delay(1000);

  Serial.println("");
  Serial.println(F("--END SETUP--"));
  Serial.println("");

  digitalWrite(LED ,LOW);
  
  Serial.println(F("--START LOOPING--"));

  #ifdef SERIAL_EN
         Serial.flush();
  #endif

}




void loop(){
  
     unsigned long currentMillis = millis();

     if (radio.receiveDone()){ 

           if (radio.DATALEN == sizeof(Payload_t)) {
       
              Serial.print(F("Delta: "));
              delta = (currentMillis - previousMillis) /1000;   
              Serial.println(delta);             
              previousMillis = currentMillis;  

    
              // N.B. In questa porzione di codice non devo fare operazioni critiche dal punto di vista computazionale e NESSUN delay !!!!
              // salvo il PayLoad in una variabile e la visualizzo dopo  radio.sendACK();
              
              //assume radio.DATA actually contains our struct and not something else
    		      //theData = *(Payload_t*)radio.DATA; 

              // Salvo i dati nelle variabili 
              datalen = radio.DATALEN;
              sender = radio.SENDERID;
              target = radio.TARGETID;
              rssi = radio.RSSI;
    
              byte myOwnBuffer[radio.DATALEN];
              memcpy(myOwnBuffer, (const void *)radio.DATA, radio.DATALEN);
              Cast_To_Struct(curData , myOwnBuffer);
    
              #if defined(DEBUG_MODE)
                Serial.print(F("PAYLOAD: ["));
                Serial.print(F(" uptime="));
                Serial.print(curData.uptime);
                Serial.print(F(" temp="));
                Serial.print(curData.temp);
                Serial.print(F(" Battery="));
                Serial.print(curData.battery_level);  
                Serial.print(F(" Lux="));
                Serial.print(curData.lux);  
                Serial.print(F(" Allarme="));
                Serial.print(curData.allarm);
                Serial.println(F("]")); 
             #endif
              
    		      pulscount++;


             u8g.firstPage();
             do {                         
                drawData( &curData , rssi, sender , delta);           
             } while (
               u8g.nextPage()
             );

    
    		     Blink(LED, 30);
    
    
           }else{
          
              Serial.print(F("Invalid payload: "));
              Serial.print('[');Serial.print(radio.SENDERID, DEC);Serial.print("] ");     
              for (byte i = 0; i < radio.DATALEN; i++){
                Serial.print((char)radio.DATA[i]);
              }   
              Serial.print(F("   [RX_RSSI:"));Serial.print(radio.RSSI);Serial.print("]");
              Serial.println("");
           }


      	   if (radio.ACKRequested()) {  
      		   radio.sendACK();
      		   Serial.print(F(" - > ACK sent."));
             Serial.println();

             /*
             u8g.firstPage();
             do {                         
                drawData( &curData , rssi, sender , delta);           
             } while (
               u8g.nextPage()
             );
             */
                           
      	   }
           
     }



	   #ifdef SERIAL_EN
			   Serial.flush();
	   #endif
   
}
