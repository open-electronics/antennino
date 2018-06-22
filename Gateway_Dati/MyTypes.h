#include <WString.h>

typedef struct {
  unsigned long uptime; 
  float         temp;   
  float         battery_level;
  int           lux;
  int           allarm;
} Payload_t;

