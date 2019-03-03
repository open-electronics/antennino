timezonedb.com library for Arduino on esp8266
---------------------------------------------

Heavily based on https://github.com/NicHub/ouilogique-ESP8266-Arduino/blob/master/horloge-cycles-ultradiens-esp8266/ouilogique_timezonedb.h


```c++
...

const char* timezonedbAPIkey = "---your api key---";
const char* timezonedbLocation = "Europe/London";

#include <timezonedb.h>

...

  long dstStart, dstEnd, gmtOffset;
  boolean dst;
  getTimezoneDst(&dst, &dstStart, &dstEnd, &gmtOffset);
  Serial.printf("dst: %d, dstStart: %ld, dstEnd: %ld, gmtOffset: %ld\n", dst, dstStart, dstEnd, gmtOffset);

...
```

**dstStart** and **dstEnd** are returned as epoche values for ease of comparison and **gmtOffset** is the current offset. Simply call the function again when the current epoche moves past **dstStart** or **dstEnd**.

See **examples/** for working example.
