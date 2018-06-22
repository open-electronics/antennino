#include <WString.h>

typedef struct {
  unsigned long uptime; 
  float temp;   
  float battery_level;
  int   lux;
} Payload_t;


typedef struct {
  int family;   
  int fw_version;
} PayloadOTA_t;

