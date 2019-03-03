#ifndef  TimeZonedB_h
#define  TimeZonedB_h


#include <Arduino.h>
#include "Client.h"
#include <WiFi.h>  
#include <ArduinoJson.h>



class TimeZonedB {
private:
Client* _client;
bool  _debug;
char _status[32];
char _timestamp[32];
char _formatted[32];
char* _timezonedbLocation;
long _timeout;
const char* timezonedbServer = "api.timezonedb.com";
const char* timezonedbGetURLFormat = "http://api.timezonedb.com/v2/get-time-zone?key=%s&format=json&by=zone&zone=%s&fields=gmtOffset";
const char* timezonedbGetTimezoneInfoURLFormat = "http://api.timezonedb.com/v2/get-time-zone?key=%s&format=json&by=zone&zone=%s";
const char* timezonedbGetTimezoneDstURLFormat = "http://api.timezonedb.com/v2/get-time-zone?key=%s&format=json&by=zone&zone=%s&fields=dst,dstStart,dstEnd,gmtOffset";
char getURL[ 115 ];
const unsigned long HTTP_TIMEOUT = 10000;
const size_t MAX_CONTENT_SIZE = 512;
bool httpConnect( const char* hostName );
bool sendRequest( const char* host, const char* getURL );
void httpDisconnect();
bool skipResponseHeaders();
void readReponseContent( char* content, size_t maxSize );
bool parseUserData( char* content);
TimeZonedB& setClient(Client& client);
public:
TimeZonedB(char* timezonedbLocation, Client& client, long timeout, bool debug);
bool getTimezoneInfo(char* timezonedbAPIkey);
unsigned long  get_UnixTimestamp();
char* get_FormattedTime();
};
#endif
