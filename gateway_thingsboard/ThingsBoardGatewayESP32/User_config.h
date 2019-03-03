
#define DEBUG_MODE true
#define SERIAL_BAUD 115200




/*###########################################################################################*/
/*                                   MQTT                                                    */

//maximum MQTT connection attempts before going to wifi setup
#define maxMQTTretry 4 
int time_before_retry = 500;


//https://thingsboard.io/docs/reference/gateway-mqtt-api/
#define subject_rpc "v1/gateway/rpc" 
#define subject_rpc_request "v1/devices/me/rpc/request/+"
#define subject_rpc_response "v1/devices/me/rpc/response/"
#define subject_rpc_attributes "v1/devices/me/attributes"
#define subject_online "v1/gateway/connect" 
#define subject_offline "v1/gateway/disconnect" 
#define subject_publish "v1/gateway/telemetry" 
#define subject_gateway_set_deviceattributes  "v1/gateway/attributes"



// N.B. questi valori sovrascrivono quelli del file PubSubClient.h
#define MQTT_VERSION = 4 //  valori possibili 3 o 4
#define MQTT_KEEPALIVE 9
#define MQTT_SOCKET_TIMEOUT 10
/*###########################################################################################*/




/*###########################################################################################*/
/*                                   NTP                                                     */
const unsigned long intervalNTP = 60000; 

/*###########################################################################################*/




/*###########################################################################################*/
/*                                      WI-FI                                                */

bool shouldSaveConfig = true;
const char config_file_path[100] = "/gatewayconfig.json";

#define Gateway_Name "TB-AntenninoGateway"
#define WifiManager_password "antennino"

char mqtt_server[16];
char mqtt_domain[40];
char mqtt_port[6] = "1883";
char mqtt_user[40]; 
char mqtt_pass[40]; 
char mqtt_token[40];
char timezonedb_key[40];  

//set minimum quality of signal so it ignores AP's under that quality
#define MinimumWifiSignalQuality 8
/*###########################################################################################*/





/*###########################################################################################*/
/*                                      RFM69                                                */

// Definisce la password di criptazione.Deve essere la stessa per tutti i nodi della rete 
// N.B.  MAX 16 caratteri
#define ENCRYPTKEY  "sampleEncryptKey"


// Deve essere lo stesso valore per tutti i nodi della stessa rete che devono comunicare, range: 1-254
#define NETWORKID 1


// id del nodo corrente Gateway
#define NODEID 253   

  
// Definisce la frequenza del modulo
#define FREQUENCY RF69_433MHZ


// Specifica di usare il meccanismo di autonegoziazione della potenza di trasmissione ATC
#define ENABLE_ATC 


// SE ATC abilitato definisce il livello minimo di RSSI
#define ATC_RSSI -80

// Definisce l'intervallo di Acknoledge
#define ACK_TIME 300


//Se settato a true abilita la modalità Sniffing
#define promiscuousMode false

/*###########################################################################################*/





/*###########################################################################################*/
/*                                      PIN                                                  */
/*-------------------PIN DEFINITIONS----------------------*/
// NOTE: On NodeMCU boards the pin labels on the board such as D2 and D5 do 
// not correspond to ESP8266 GPIO pin numbers. For example, D2 is GPIO4. Use
// the Dn labels to connect wires but use GPIO pin numbers in Arduino code.
// http://esp8266.github.io/Arduino/versions/2.0.0/doc/reference.html#digital-io
// https://steve.fi/Hardware/nodemcu/default-pins/


#define OLED_SDA 21
#define OLED_SCL 22


//GPIO15
#define WIFI_RESET 15 

//GPIO36
#define GPS_POWER 36 


//GPIO32
#define LED_VERDE 32 


//MISO  GPIO19  
//MOSI  GPIO23  
//SCK   GPIO18  

//CS    GPIO5   
//DIO0  GPI02  
//RST   GPIO4
//IRQ   GPIO2
   


#define RFM69_CS      5   
#define RFM69_IRQ     2   
#define RFM69_IRQN    digitalPinToInterrupt(RFM69_IRQ)
#define RFM69_RST     4   


// UART2TX GPIO17
// UART2RX GPIO16


/*###########################################################################################*/






/*###########################################################################################*/
/* LCD
/*###########################################################################################*/
#define LOGO16_GLCD_HEIGHT 16 
#define LOGO16_GLCD_WIDTH  16 

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
