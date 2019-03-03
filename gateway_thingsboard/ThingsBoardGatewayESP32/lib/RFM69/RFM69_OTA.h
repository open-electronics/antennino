#ifndef RFM69_OTA_H
#define RFM69_OTA_H
#include <RFM69.h>
#include <SPIFlash.h>

#if defined(MOTEINO_ZERO)
  #define LED           13 // Moteino M0
#elif defined(__AVR_ATmega1284P__)
  #define LED           15 // Moteino MEGAs have LEDs on D15
#elif defined (__AVR_ATmega328P__)
  #define LED           9 // Moteinos have LEDs on D9
#else
  #define LED           13 // catch all others (UNO, pro mini etc)
#endif


#define SHIFTCHANNEL 1000000 //amount to shift frequency of HEX transmission to keep original channel free of the HEX transmission traffic

#ifndef DEFAULT_TIMEOUT
  #define DEFAULT_TIMEOUT 3000
#endif

#ifndef ACK_TIMEOUT
  #define ACK_TIMEOUT 20
#endif




//functions used in the REMOTE node
void CheckForWirelessHEX(RFM69& radio, SPIFlash& flash, uint8_t DEBUG=false, uint8_t LEDpin=LED);
void HandleHandshakeACK(RFM69& radio, SPIFlash& flash, uint8_t flashCheck=true);
void resetUsingWatchdog(uint8_t DEBUG=false);
uint8_t HandleWirelessHEXData(RFM69& radio, uint8_t remoteID, SPIFlash& flash, uint8_t DEBUG=false, uint8_t LEDpin=LED);

#ifdef SHIFTCHANNEL
uint8_t HandleWirelessHEXDataWrapper(RFM69& radio, uint8_t remoteID, SPIFlash& flash, uint8_t DEBUG=false, uint8_t LEDpin=LED);
#endif

//functions used in the MAIN node
uint8_t CheckForSerialHEX(uint8_t* input, uint8_t inputLen, RFM69& radio, uint8_t targetID, uint16_t TIMEOUT=DEFAULT_TIMEOUT, uint16_t ACKTIMEOUT=ACK_TIMEOUT, uint8_t DEBUG=false);
uint8_t CheckForFlashHEX(SPIFlash flash, RFM69& radio, uint8_t targetID, uint16_t TIMEOUT = DEFAULT_TIMEOUT, uint16_t ACKTIMEOUT = ACK_TIMEOUT, uint8_t DEBUG = false);

uint8_t ProgrammerIsBusy();
uint8_t ProgramProcessIsComplete();

uint8_t HandleFlashHandshake(RFM69& radio, uint8_t targetID, uint8_t isEOF, uint16_t TIMEOUT = DEFAULT_TIMEOUT, uint16_t ACKTIMEOUT = ACK_TIMEOUT, uint8_t DEBUG = false);
uint8_t HandleSerialHandshake(RFM69& radio, uint8_t targetID, uint8_t isEOF, uint16_t TIMEOUT=DEFAULT_TIMEOUT, uint16_t ACKTIMEOUT=ACK_TIMEOUT, uint8_t DEBUG=false);
uint8_t HandleSerialHEXData(RFM69& radio, uint8_t targetID, uint16_t TIMEOUT=DEFAULT_TIMEOUT, uint16_t ACKTIMEOUT=ACK_TIMEOUT, uint8_t DEBUG=false);
uint8_t HandleFlashHEXData(SPIFlash flash, RFM69& radio, uint8_t targetID, uint16_t TIMEOUT = DEFAULT_TIMEOUT, uint16_t ACKTIMEOUT = ACK_TIMEOUT, uint8_t DEBUG = false);

#ifdef SHIFTCHANNEL
uint8_t HandleSerialHEXDataWrapper(RFM69& radio, uint8_t targetID, uint16_t TIMEOUT=DEFAULT_TIMEOUT, uint16_t ACKTIMEOUT=ACK_TIMEOUT, uint8_t DEBUG=false);
#endif

#ifdef SHIFTCHANNEL
uint8_t HandleFlashHEXDataWrapper(SPIFlash flash,RFM69& radio, uint8_t targetID, uint16_t TIMEOUT = DEFAULT_TIMEOUT, uint16_t ACKTIMEOUT = ACK_TIMEOUT, uint8_t DEBUG = false);
#endif

uint8_t validateHEXData(void* data, uint8_t length , uint8_t DEBUG);
uint8_t prepareSendBuffer(char* hexdata, uint8_t*buf, uint8_t length, uint16_t seq);
uint8_t sendHEXPacket(RFM69& radio, uint8_t remoteID, uint8_t* sendBuf, uint8_t hexDataLen, uint16_t seq, uint16_t TIMEOUT=DEFAULT_TIMEOUT, uint16_t ACKTIMEOUT=ACK_TIMEOUT, uint8_t DEBUG=false);
uint8_t BYTEfromHEX(char MSB, char LSB);
uint8_t readSerialLine(char* input, char endOfLineChar=10, uint8_t maxLength=115, uint16_t timeout=1000);
void PrintHex83(uint8_t* data, uint8_t length);

#endif
