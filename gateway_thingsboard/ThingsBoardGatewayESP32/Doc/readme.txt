Hi.
Thanks for your work and time and sorry about my funny english :)
I'm developing a project and I need more informations about SPI on ESP8266.
If I understood ...
There are 2 hardware SPI on ESP8266 (i'm using ESP12-F).
One is called SPI (digital pin 6-11) but is not available because it usually used to connect memory.
One is called HSPI (GPIO12-15).
Ok?
The question:
If I need use two SPI device and connect
SPI CLK = GPIO14
SPI MOSI = GPIO13
SPI MISO = GPIO12
CS1 = GPIO16
CS2 = GPIO4



http://esp8266.github.io/Arduino/versions/2.0.0/doc/reference.html#digital-io
USARE QUESTO:
RFM69HW	ESP-12E

MISO	GPIO12
MOSI	GPIO13
SCK	    GPIO14
CS/SS	GPIO15
DIO0	GPIO5




//SERVER R&D
//mqtt_server = "77.108.55.123"; 
//token ="FMAi3ot2DdR6Amt0Rio2";  


//SERVER ARUBA
//server = "http://217.61.61.122"; 
//token = "vxsZWewHKJ20vFM2X7gI";   

token timedb: U9ZNKNDY6T6M



//SERVER DEMO LIVE
//mqtt_server = "demo.thingsboard.io";   104.196.24.70
//token = "eOlO3RjnhSBs6aazkHOx";  
