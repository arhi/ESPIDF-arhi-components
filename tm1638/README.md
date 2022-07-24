Based on https://github.com/MahdaSystem/TM1638

Modified to properly work with ESP-IDF, added support for hardware SPI for faster communication with the TM1638 utilizing 3WIRE SPI support of the ESP (original code worked on by bitbanging communication). ESP-IDF kgconfig added for proper configuration of ESP32 pins for both GPIO and SPI communication. 

I am using this component with cheap LED&KEY module from aliexpress (TM1638 Module Key Display)

![TM1638 LED&KEY](./lednkey.jpg?raw=true "TM1638 LED&KEY Module from AliExpress")
![TM1638 LED&KEY](./lednkey1.jpg?raw=true "TM1638 LED&KEY Module from AliExpress")

example usage:

```C
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "TM1638.h"

void app_main(void)
{

  TM1638_Handler_t Handler;

  TM1638_Init(&Handler, TM1638DisplayTypeComCathode);
  TM1638_ConfigDisplay(&Handler, 7, TM1638DisplayStateON);

  for (uint8_t i = 0; i<8; i++)
  {
    TM1638_SetSingleDigit_HEX(&Handler, (i+0) | TM1638DecimalPoint, i*2);
    TM1638_SetSingleDigit_HEX(&Handler, 0xff, i*2+1);
  }
  vTaskDelay(2000);

  // On LED&KEY module from Ali every second position is 7seg digit and between them are the LEDS
  // show 3.5 on the 7segs
  uint8_t kita[16] = {3| TM1638DecimalPoint,0xff,5,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
  TM1638_SetMultipleDigit_HEX(&Handler, kita, 0, 16);
  vTaskDelay(5000);

  // now read keys and for every pressed key turn on appropriate led 
  uint32_t tasteri;
  while(1){
    
    TM1638_ScanKeys(&Handler, &tasteri);
    //ESP_LOGI(TAG, "TASTERI %d", tasteri);
    
    if (tasteri & TM1638_MASK_S1) {
      TM1638_SetSingleDigit_HEX(&Handler, 0, TM1638_LED(1));
    } else {
      TM1638_SetSingleDigit_HEX(&Handler, 0xff, TM1638_LED(1));
    }
    
    if (tasteri & TM1638_MASK_S2) {
      TM1638_SetSingleDigit_HEX(&Handler, 0, TM1638_LED(2));
    } else {
      TM1638_SetSingleDigit_HEX(&Handler, 0xff, TM1638_LED(2));
    }

    if (tasteri & TM1638_MASK_S3) {
      TM1638_SetSingleDigit_HEX(&Handler, 0, TM1638_LED(3));
    } else {
      TM1638_SetSingleDigit_HEX(&Handler, 0xff, TM1638_LED(3));
    }

    if (tasteri & TM1638_MASK_S4) {
      TM1638_SetSingleDigit_HEX(&Handler, 0, TM1638_LED(4));
    } else {
      TM1638_SetSingleDigit_HEX(&Handler, 0xff, TM1638_LED(4));
    }

    if (tasteri & TM1638_MASK_S5) {
      TM1638_SetSingleDigit_HEX(&Handler, 0, TM1638_LED(5));
    } else {
      TM1638_SetSingleDigit_HEX(&Handler, 0xff, TM1638_LED(5));
    }
    
    if (tasteri & TM1638_MASK_S6) {
      TM1638_SetSingleDigit_HEX(&Handler, 0, TM1638_LED(6));
    } else {
      TM1638_SetSingleDigit_HEX(&Handler, 0xff, TM1638_LED(6));
    }


    if (tasteri & TM1638_MASK_S7) {
      TM1638_SetSingleDigit_HEX(&Handler, 0, TM1638_LED(7));
    } else {
      TM1638_SetSingleDigit_HEX(&Handler, 0xff, TM1638_LED(7));
    }

    if (tasteri & TM1638_MASK_S8) {
      TM1638_SetSingleDigit_HEX(&Handler, 0, TM1638_LED(8));
    } else {
      TM1638_SetSingleDigit_HEX(&Handler, 0xff, TM1638_LED(8));
    }

    vTaskDelay(1);
  } 

}

```