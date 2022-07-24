Based on https://github.com/MahdaSystem/TM1638

Modified to properly work with ESP-IDF, added support for hardware SPI for faster communication with the TM1638 utilizing 3WIRE SPI support of the ESP (original code worked on by bitbanging communication). ESP-IDF kgconfig added for proper configuration of ESP32 pins for both GPIO and SPI communication. 

I am using this component with cheap LED&KEY module from aliexpress (TM1638 Module Key Display)

![TM1638 LED&KEY](./lednkey.jpg?raw=true "TM1638 LED&KEY Module from AliExpress")
![TM1638 LED&KEY](./lednkey1.jpg?raw=true "TM1638 LED&KEY Module from AliExpress")
