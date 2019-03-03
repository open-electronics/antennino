
#include "TimeZonedB.h"
#include "Arduino.h"




TimeZonedB::TimeZonedB(char* timezonedbLocation, Client& client, long timeout, bool debug) {
    this->_timezonedbLocation = timezonedbLocation;
    this->_timeout = timeout;
    this->_debug = debug;  
    setClient(client);  
}



TimeZonedB& TimeZonedB::setClient(Client& client){
    this->_client = &client;
    return *this;
}



 // Open connection to the HTTP server
bool TimeZonedB::httpConnect( const char* hostName ){

  if  (_debug ) {
    Serial.print( "HTTP connection to : " );
    Serial.print( hostName );
    long T1 = millis();
  }
    
  bool httpStatus = _client->connect( hostName, 80 );
  
  if  (_debug ) {
    Serial.print  ( httpStatus ? " / OK" : " / Error" );
    Serial.print  ( " / dT = " );
    Serial.print  ( millis() - T1 );
    Serial.println( " ms " );
  }
  
  return httpStatus;
}



// Send the HTTP GET request to the server
bool TimeZonedB::sendRequest( const char* host, const char* getURL ){

  if  (_debug ) {
    Serial.print( "GET:" );
  }
    _client->print( "GET " );
    _client->print( getURL );
    _client->print( " HTTP/1.1\r\nHost: " );
    _client->print( timezonedbServer );
    _client->print( "\r\nConnection: close\r\n\r\n" );
   
    
   if  (_debug ) {
    Serial.println( getURL );
   }
  
  return true;
  
}



// Close the connection with the HTTP server
void TimeZonedB::httpDisconnect(){
   if  (_debug ) {
     Serial.println( "Disconnect" );
   }
  _client->stop();
}



// Skip HTTP headers so that we are at the beginning of the response's body
bool TimeZonedB::skipResponseHeaders(){

  // HTTP headers end with an empty line
  char endOfHeaders[] = "\r\n\r\n";
  
  _client->setTimeout( HTTP_TIMEOUT );
  
  bool ok = _client->find( endOfHeaders );
  
  if  (_debug ) {
    Serial.print( "HTTP response:" ); 
    if( !ok ) {
      Serial.println( "Invalid response" );
    }else{
      Serial.println( "OK" );
    }
  }

return ok;
  
}




// Read the body of the response from the HTTP server
void TimeZonedB::readReponseContent( char* content, size_t maxSize ){
  
  if  (_debug ) {
    long T1 = millis();
    Serial.print("Client read time:" );
  }

  size_t length = 0;
  
  while( _client->available() ){
    content[ length++ ] = _client->read();
  } 
  content[ length ] = 0;
  
  if  (_debug ) {
    long dT = millis() - T1;
    Serial.println( dT );
    Serial.print( "No of bytes:" );
    Serial.println( length );
  }

  int contentSize = ( int )strlen( content );
  char * pch;
  int msgStart = 0;
  pch = strchr( content, '{' );
  msgStart = pch-content+1;
  int msgStop = 0;
  pch = strrchr( content, '}' );
  msgStop = pch-content+1;

  for( int i=0; i<msgStop; i++ ){ 
    content[ i ] = content[ i + msgStart - 1 ] ; 
  }
    
  for( int i=msgStop-3; i<=contentSize; i++ ){ 
    content[ i ] = '\0' ; 
  }

if  (_debug ) { 
  Serial.print("Response from server:" );
  Serial.println( content );
}

}





bool TimeZonedB::parseUserData( char* content){
  //Compute optimal size of the JSON buffer according to what we need to parse.
  //This is only required if you use StaticJsonBuffer.
  //const size_t BUFFER_SIZE =
  //JSON_OBJECT_SIZE( 3 );

  // Allocate a temporary memory pool on the stack
  //StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
  // If the memory pool is too big for the stack, use this instead:
  
  DynamicJsonBuffer jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject( content );

  if (!root.success()){   
   if  (_debug ) {
     Serial.println( "JSON parsing failed!" );
   }
    return false; 
  }
  

   strcpy( _status , root["status"]);  
   strcpy( _timestamp , root["timestamp"]);
   strcpy( _formatted , root["formatted"]);
   

  // It's not mandatory to make a copy, you could just use the pointers
  // Since, they are pointing inside the "content" buffer, so you need to make
  // sure it's still in memory when you read the string
  
  return true;
}






bool TimeZonedB::getTimezoneInfo(char* timezonedbAPIkey) {

  bool result = false;
  
  if( ! httpConnect( timezonedbServer ) ) {
    return result;
  }
  
  sprintf( getURL, timezonedbGetTimezoneInfoURLFormat, timezonedbAPIkey, _timezonedbLocation );
  
  
  if( sendRequest( timezonedbServer, getURL ) && skipResponseHeaders() ){
    char response[ MAX_CONTENT_SIZE ];     
    readReponseContent( response, MAX_CONTENT_SIZE ); 
      
    if( parseUserData(response) ){

      result = true; 
       
      /*  
      if (_status == 'OK') {
          result = true;
      }
      */
       
    }
    
         
  } 
  
  
  httpDisconnect();
  
  return result;
  
}

  

unsigned long  TimeZonedB::get_UnixTimestamp() { 
   return atol(_timestamp);      
}


char * TimeZonedB::get_FormattedTime() {
  return  _formatted;  
}





