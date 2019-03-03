// https://lowpowerlab.com/forum/projects/monteino-esp8266-based-gateway/
// https://techtutorialsx.com/2016/05/22/esp8266-connection-to-ds3231-rtc/



RFM69_ATC radio = RFM69_ATC(RFM69_CS, RFM69_IRQ, false, RFM69_IRQN);





void setupRFM69(void) {  
  // Hard Reset the RFM module
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, HIGH);
  delay(100);
  digitalWrite(RFM69_RST, LOW);
  delay(100);

  radio.initialize(FREQUENCY,NODEID,NETWORKID);
  radio.encrypt(ENCRYPTKEY);
  radio.promiscuous(promiscuousMode);
  radio.enableAutoPower(ATC_RSSI);  
}




void set_online(uint8_t sender_id){ 
  
// https://thingsboard.io/docs/reference/gateway-mqtt-api/  
// ESEMPIO: JSON da inviare {"device":"RFM69_XXX"}

  
char s[10];
sprintf(s ,"RFM69_%d",sender_id); 

String payload = "{";

  payload += "\"device\":\"";
  payload += + s ;
  payload += "\"}";
  char JsonPayload[300];
  payload.toCharArray( JsonPayload, 300);

  #if(DEBUG_MODE) 
       Serial.println(); 
       Serial.println("#################################"); 
       Serial.print("SEND ONLINE: "); 
       Serial.println( JsonPayload );
       Serial.println("#################################"); 
       Serial.println();
  #endif 
  
  client.publish( subject_online, JsonPayload );

}




void Publish_CS_Attributes(char *data, uint8_t sender_id ){
  
  // https://thingsboard.io/docs/user-guide/attributes/#attribute-types
  // https://arduinojson.org/v5/api/jsonobject/createnestedobject/

  //ESEMPIO: JSON da inviare {"RFM69_2": {"node_id": 2,"device_type_2": "WW","firmware_version_2": "RRRR"} }

  String key;
  String value;
  char s[10];
  sprintf(s ,"RFM69_%d",sender_id); 

  String payload = "{";
  payload += s;
  payload += ":";
  payload += "{";
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(data);
  for (auto kv : root) {
    key = kv.key;
    if (key != "DT"){
      value =kv.value.as<char*>(); 
      payload += "\"";  payload += key;  payload += "\":";   
      payload += value;   
      payload += ","; 
    } 
  }  
  payload = payload.substring(0,payload.length()-1) ;
  payload += "}";
  payload += "}";
  
  char JsonPayload[300];
  payload.toCharArray( JsonPayload, 300);


  #if(DEBUG_MODE)
    Serial.println( JsonPayload );
  #endif
  
  if (client.publish(subject_gateway_set_deviceattributes, JsonPayload) == true) {
    #if(DEBUG_MODE)
     Serial.println("Success Publish_DeviceAttributes");
    #endif
  } else {
    #if(DEBUG_MODE)
     Serial.println("Error Publish_DeviceAttributes");
    #endif
  }
    
}




void PublishRPC_Response(char *data, uint8_t sender_id ){

      StaticJsonBuffer<200> jsonBuffer;
      JsonObject& json_data = jsonBuffer.parseObject(data);
      
      int pin = json_data["PIN"];
      boolean value = json_data["VAL"];
      String ri = json_data["RI"];   

      #if(DEBUG_MODE)
        Serial.println();
        Serial.println(F("#######################################"));
        Serial.println("PARSED DATA: ");
        Serial.print("PIN:");
        Serial.println(pin);
        Serial.print("VAL:");
        Serial.println(value);
        Serial.print("RI:");
        Serial.println(ri);
        Serial.println(F("#######################################"));
        Serial.println();
     #endif
     
      char payload[200];
      StaticJsonBuffer<200> jsonBuffer_Build;
      JsonObject& json_buld_data = jsonBuffer_Build.createObject();
      json_buld_data[String(pin)] = value ? true : false;  
      json_buld_data.printTo(payload, sizeof(payload)); 
      String strPayload = String(payload);
      String response_topic = subject_rpc_response + ri;

      #if(DEBUG_MODE)
        Serial.println();
        Serial.println(F("#######################################"));
        Serial.print("PAYLOAD:");
        Serial.println(strPayload);
        Serial.print("TOPIC:");
        Serial.println(response_topic);
        Serial.println(F("#######################################"));
        Serial.println();
      #endif
      
      client.publish(response_topic.c_str(), strPayload.c_str());
       
      client.publish(subject_rpc_attributes, strPayload.c_str());
      
}



boolean PublishTelemetryData(char *data, uint8_t sender_id , uint16_t rss){
  // https://thingsboard.io/docs/reference/gateway-mqtt-api/
  String key;
  String value;
  String ts = getRTCDate();
  boolean result=false;


  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("PUBLISH MQTT ->");
  display.setCursor(0, 20);
  display.print("From Node: ");
  display.println(sender_id);
  display.setCursor(0, 40);
  display.print("TS:");
  display.println(ts);
  display.display(); 


  String payload = "{";
  payload += "\"RFM69_"; payload += sender_id; payload += "\":";
  payload += "[";
  payload += "{";
  
  payload += "\"ts\":"; payload += "\""; payload += ts;  ; payload += "\""; payload += ",";
  
  payload += "\"values\":";
  payload += "{";

  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(data);
  for (auto kv : root) {
    key = kv.key;
    if (key != "DT"){
      value =kv.value.as<char*>(); 
      payload += "\"";  payload += key;  payload += "\":";   
      payload += "\""; payload += value;  payload += "\""; 
      payload += ","; 
    }    
  }  

  payload += "\"RSS\":";payload += "\""; payload += rss; payload += "\"";  

   
  
  // rimuovo l'ultima "," 
  //payload = payload.substring(0,payload.length()-1) ;

  payload += "}";  
  payload += "}";
  payload += "]";  
  payload += "}";

  //Esempio:
  //payload = "{\"RFM69_1\":[{\"ts\":1483228800000,\"values\":{\"BAT\":2.51,\"T\":26.38,\"L\":876.00}}]}";
  
  char JsonPayload[300];
  payload.toCharArray( JsonPayload, 300);


  #if(DEBUG_MODE)
         Serial.println();
         Serial.println("##################################");  
         Serial.print("SENT PAYLOAD:");
         Serial.println( JsonPayload );
         Serial.println("##################################"); 
         Serial.println();       
  #endif


  
  //Invio i dati telemetrici alla dashboard TB

  client.publish(subject_publish, JsonPayload);

  n_mqtt_packet_sent++;  
 
  result=true;

  return result;
  
}






boolean PublishTelemetryData_OLD(char *data, uint8_t sender_id , uint16_t rss){
  // https://thingsboard.io/docs/reference/gateway-mqtt-api/
  String key;
  String value;
  String ts = getRTCDate();
  boolean result=false;

  String payload = "{";
  payload += "\"RFM69_"; payload += sender_id; payload += "\":";
  payload += "[";
  payload += "{";
  payload += "\"ts\":";payload += ts ; payload += ",";
  payload += "\"values\":";
  payload += "{";

  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(data);
  for (auto kv : root) {
    key = kv.key;
    if (key != "DT"){
      value =kv.value.as<char*>(); 
      payload += "\"";  payload += key;  payload += "\":";   
      payload += value;   
      payload += ","; 
    }    
  }  

  payload += "\"RSS\":"; payload += rss;   
  
  // rimuovo l'ultima "," 
  //payload = payload.substring(0,payload.length()-1) ;

  payload += "}";  
  payload += "}";
  payload += "]";  
  payload += "}";

  //Esempio:
  //payload = "{\"RFM69_1\":[{\"ts\":1483228800000,\"values\":{\"BAT\":2.51,\"T\":26.38,\"L\":876.00}}]}";
  
  char JsonPayload[300];
  payload.toCharArray( JsonPayload, 300);


  #if(DEBUG_MODE)
         Serial.println();
         Serial.println("##################################");  
         Serial.print("SENT PAYLOAD:");
         Serial.println( JsonPayload );
         Serial.println("##################################"); 
         Serial.println();       
  #endif


  
  //Invio i dati telemetrici alla dashboard TB

  client.publish(subject_publish, JsonPayload);

  n_mqtt_packet_sent++;  
 
  result=true;

  return result;
  
}




//  RFM69 -> MQTT
boolean RFM69toMQTT(void) {
  
   //https://arduinojson.org/v5/api/jsonbuffer/parseobject/
  
  //check if something was received (could be an interrupt from the radio)   
  if (radio.receiveDone()){
    
    char data[RF69_MAX_DATA_LEN+1]; // +1 For the null character
    uint8_t SENDERID = radio.SENDERID;
    uint8_t DATALEN = radio.DATALEN;
    uint16_t RSSI = radio.RSSI;

    if (devices[radio.SENDERID]==0) {
       devices[radio.SENDERID]= 1;
       client.disconnect();
    }
    
    //save packet because it may be overwritten
    memcpy(data, (void *)radio.DATA, DATALEN);
    data[DATALEN] = '\0';  // Terminate the string

    // Ack as soon as possible
    //check if sender wanted an ACK
    if (radio.ACKRequested()){
      radio.sendACK();
    }

    #if(DEBUG_MODE)
      Serial.println();
      Serial.println(F("#######################################"));
      Serial.print("Raw Received Data From NODE_ID: ");
      Serial.println(SENDERID);   
      if (DATALEN > 0){
        for (byte i = 0; i < DATALEN; i++){
          Serial.print((char)data[i]);
        }
      }
      Serial.println();
      Serial.println(F("#######################################"));
      Serial.println();
    #endif


   
   StaticJsonBuffer<200> jsonBuffer;
   // definendo in questo modo data  evito che venga corrotto: viene duplicato
   JsonObject& json_data = jsonBuffer.parseObject(jsonBuffer.strdup(data));
 
  
   if (!json_data.success()){
     Serial.println("parseObject() failed");
     return false;
   }

   String data_type = json_data["DT"];


   //set_online(SENDERID);


   if (data_type=="RPC_R"){
      PublishRPC_Response(data, SENDERID);     
   }


   if (data_type=="DATA"){     
      PublishTelemetryData(data, SENDERID, RSSI);           
   }


   if (data_type=="CS_ATTR"){         
      Publish_CS_Attributes(data, SENDERID);  
   }

   return true;

  } else {
    return false;
  }

  radio.receiveDone();
  
}



//  MQTT --> RFM69 
// The callback for when a PUBLISH message is received from the server.
// https://randomnerdtutorials.com/decoding-and-encoding-json-with-arduino-or-esp8266/


void MQTTtoRFM69(char* topic, byte* payload, unsigned int length) {
  char json[length + 1];
  strncpy (json, (char*)payload, length);
  json[length] = '\0';

  // Decode JSON request
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& data = jsonBuffer.parseObject((char*)json);

  if (!data.success()){
    Serial.println("parseObject() failed");
  }else {
    Serial.println("RECEIVED MQTT JSON: ");
    Serial.println(json);
  }


  String methodName = String((const char*)data["method"]);
  int remoteTarget = data["params"]["target"];
  int pin   = data["params"]["pin"];
  String value = data["params"]["enabled"];
  int rcp_value; 
  String remoteRPC ;
  String command;
  char buffer[50];
  byte sendLen;  


  #if(DEBUG_MODE)
    Serial.println();
    Serial.println("---------------------------------------");
    Serial.print("METHOD: ");
    Serial.print(methodName);
    Serial.print(" PIN:");
    Serial.print(pin);
    Serial.print(" VALUE:");
    Serial.print(value);
    Serial.print(" TARGET:");
    Serial.println(remoteTarget);
    Serial.println("---------------------------------------");
    Serial.println();
  #endif


  if (methodName.equals("getGpioStatus")) {
     remoteRPC = "GET_PIN";
  } else if (methodName.equals("setGpioStatus")) {
     remoteRPC = "SET_PIN";
  }

  if (value.equals("true")) {
     rcp_value = 1;
  } else {
     rcp_value = 0;
  }
  

  String responseTopic = String(topic);
  responseTopic.replace("request", "response"); 
  // Estraggo il request_id da passare al nodo RFM69 che verrà usato per postare la response Json tramite MQTT
  // response topic = "v1/devices/me/rpc/response/" + request_id
  String request_id = responseTopic.substring(responseTopic.lastIndexOf("/") + 1, responseTopic.length()); 
  
  if (pin != 0) { 

    if  (remoteRPC == "SET_PIN") {   
      command = "RPC:" + remoteRPC + ";" + "PIN:" + pin + ";" + "VAL:"+  rcp_value + ";" + "RI:" + request_id;         
    } 
    
    if(remoteRPC == "GET_PIN"){   
      command = "RPC:" + remoteRPC + ";" + "PIN:" + pin + ";" + "RI:" + request_id;       
    } 

    command.toCharArray(buffer, 50); 

    #if(DEBUG_MODE)
      Serial.println();
      Serial.println("############################");
      Serial.println("SENDING RPC TO RFM69");
      Serial.print("Target: ");
      Serial.print(remoteTarget);
      Serial.print(" Data[");
      Serial.print(buffer);
      Serial.println("]");
    #endif



    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("RECEIVED MQTT <--");
    display.setCursor(0, 20);
    display.print("For Node: ");
    display.println(remoteTarget);
    display.setCursor(0, 40);
    display.println(buffer);
    display.display(); 


    if(radio.sendWithRetry(remoteTarget, buffer, strlen(buffer))) {     
      #if(DEBUG_MODE)
        Serial.println("RCP ACK-OK");   
      #endif  
    } else {   
      #if(DEBUG_MODE)
        Serial.println("RCP ACK-KO");
      #endif
    }


    #if(DEBUG_MODE)
     Serial.println("############################");
     Serial.println();
    #endif
    
   }

}
