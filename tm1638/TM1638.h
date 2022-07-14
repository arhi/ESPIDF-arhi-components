#ifndef _TM1638_H_
#define _TM1638_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#if CONFIG_SPI_INTERFACE  
#include <string.h>
#include "driver/spi_master.h"
#endif

#define TM1638_CLK_GPIO     CONFIG_TM1638_CLK_GPIO
#define TM1638_DIO_GPIO     CONFIG_TM1638_DIO_GPIO
#define TM1638_STB_GPIO     CONFIG_TM1638_STB_GPIO

#define TM1638_SUPPORT_COM_ANODE  1
#define TM1638DisplayTypeComCathode 0
#define TM1638DisplayTypeComAnode   1

#define TM1638DisplayStateOFF 0
#define TM1638DisplayStateON  1

#define TM1638DecimalPoint    0x80

/**
 * @brief  Handler data type
 * @note   User must initialize this this functions before using library:
 *         - PlatformInit
 *         - PlatformDeInit
 *         - DioConfigOut
 *         - DioConfigIn
 *         - DioWrite
 *         - DioRead
 *         - ClkWrite
 *         - StbWrite
 *         - DelayUs
 */
typedef struct TM1638_Handler_s
{
  // Initialize the platform-dependent layer
  void (*PlatformInit)(void);
  // Uninitialize the platform-dependent layer
  void (*PlatformDeInit)(void *);

  // Config the GPIO that connected to DIO PIN of SHT1x as output
  void (*DioConfigOut)(void);
  // Config the GPIO that connected to DIO PIN of SHT1x as input
  void (*DioConfigIn)(void);
  // Set level of the GPIO that connected to DIO PIN of SHT1x
  void (*DioWrite)(uint8_t);
  // Read the GPIO that connected to DIO PIN of SHT1x
  uint8_t (*DioRead)(void);

  // Set level of the GPIO that connected to CLK PIN of SHT1x
  void (*ClkWrite)(uint8_t);

  // Set level of the GPIO that connected to STB PIN of SHT1x
  void (*StbWrite)(uint8_t);

  // Delay (us)
  void (*DelayUs)(uint8_t);

  uint8_t DisplayType;
#if CONFIG_SPI_INTERFACE   
  spi_device_handle_t SPIHandle;
#endif

#if (TM1638_SUPPORT_COM_ANODE)
  uint8_t DisplayRegister[16];
#endif
} TM1638_Handler_t;


/**
 * @brief  Data type of library functions result
 */
typedef enum TM1638_Result_e
{
  TM1638_OK      = 0,
  TM1638_FAIL    = -1,
} TM1638_Result_t;




/**
 * @brief  Initialize TM1638.
 * @param  Handler: Pointer to handler
 * @param  Type: Determine the type of display
 *         - TM1638DisplayTypeComCathode: Common-Cathode
 *         - TM1638DisplayTypeComAnode:   Common-Anode
 * @note   If 'TM1638_SUPPORT_COM_ANODE' switch is set to 0, the 'Type' argument
 *         will be ignored 
 *         
 * @retval TM1638_Result_t
 *         - TM1638_OK: Operation was successful.
 */
TM1638_Result_t
TM1638_Init(TM1638_Handler_t *Handler, uint8_t Type);


/**
 * @brief  De-Initialize TM1638.
 * @param  Handler: Pointer to handler
 * @retval TM1638_Result_t
 *         - TM1638_OK: Operation was successful.
 */
TM1638_Result_t
TM1638_DeInit(TM1638_Handler_t *Handler);
 



/**
 * @brief  Config display parameters
 * @param  Handler: Pointer to handler
 * @param  Brightness: Set brightness level
 *         - 0: Display pulse width is set as 1/16
 *         - 1: Display pulse width is set as 2/16
 *         - 2: Display pulse width is set as 4/16
 *         - 3: Display pulse width is set as 10/16
 *         - 4: Display pulse width is set as 11/16
 *         - 5: Display pulse width is set as 12/16
 *         - 6: Display pulse width is set as 13/16
 *         - 7: Display pulse width is set as 14/16
 * 
 * @param  DisplayState: Set display ON or OFF
 *         - TM1638DisplayStateOFF: Set display state OFF
 *         - TM1638DisplayStateON: Set display state ON
 * 
 * @retval TM1638_Result_t
 *         - TM1638_OK: Operation was successful
 */
TM1638_Result_t
TM1638_ConfigDisplay(TM1638_Handler_t *Handler,
                     uint8_t Brightness, uint8_t DisplayState);


/**
 * @brief  Set data to single digit in 7-segment format
 * @param  Handler: Pointer to handler
 * @param  DigitData: Digit data
 * @param  DigitPos: Digit position
 *         - 0: Seg1
 *         - 1: Seg2
 *         - .
 *         - .
 *         - .
 * 
 * @retval TM1638_Result_t
 *         - TM1638_OK: Operation was successful
 */
TM1638_Result_t
TM1638_SetSingleDigit(TM1638_Handler_t *Handler,
                      uint8_t DigitData, uint8_t DigitPos);


/**
 * @brief  Set data to multiple digits in 7-segment format
 * @param  Handler: Pointer to handler
 * @param  DigitData: Array to Digits data
 * @param  StartAddr: First digit position
 *         - 0: Seg1
 *         - 1: Seg2
 *         - .
 *         - .
 *         - .
 * 
 * @param  Count: Number of segments to write data
 * @retval TM1638_Result_t
 *         - TM1638_OK: Operation was successful
 */
TM1638_Result_t
TM1638_SetMultipleDigit(TM1638_Handler_t *Handler, const uint8_t *DigitData,
                        uint8_t StartAddr, uint8_t Count);


/**
 * @brief  Set data to multiple digits in 7-segment format
 * @param  Handler: Pointer to handler
 * @param  DigitData: Digit data (0, 1, ... , 15, a, A, b, B, ... , f, F) 
 * @param  DigitPos: Digit position
 *         - 0: Seg1
 *         - 1: Seg2
 *         - .
 *         - .
 *         - .
 * 
 * @retval TM1638_Result_t
 *         - TM1638_OK: Operation was successful
 */
TM1638_Result_t
TM1638_SetSingleDigit_HEX(TM1638_Handler_t *Handler,
                          uint8_t DigitData, uint8_t DigitPos);


/**
 * @brief  Set data to multiple digits in hexadecimal format
 * @param  Handler: Pointer to handler
 * @param  DigitData: Array to Digits data. 
 *                    (0, 1, ... , 15, a, A, b, B, ... , f, F)
 * 
 * @param  StartAddr: First digit position
 *         - 0: Seg1
 *         - 1: Seg2
 *         - .
 *         - .
 *         - .
 * 
 * @param  Count: Number of segments to write data
 * @retval TM1638_Result_t
 *         - TM1638_OK: Operation was successful
 */
TM1638_Result_t
TM1638_SetMultipleDigit_HEX(TM1638_Handler_t *Handler, const uint8_t *DigitData,
                            uint8_t StartAddr, uint8_t Count);




/**
 * @brief  Scan all 24 keys connected to TM1638
 * @note   
 *                   SEG1         SEG2         SEG3       ......      SEG8
 *                     |            |            |                      |
 *         K1  --  |K1_SEG1|    |K1_SEG2|    |K1_SEG3|    ......    |K1_SEG8|
 *         K2  --  |K2_SEG1|    |K2_SEG2|    |K2_SEG3|    ......    |K2_SEG8|
 *         K3  --  |K3_SEG1|    |K3_SEG2|    |K3_SEG3|    ......    |K3_SEG8|
 * 
 * @param  Handler: Pointer to handler
 * @param  Keys: pointer to save key scan result
 *         - bit0=>K1_SEG1, bit1=>K1_SEG2, ..., bit7=>K1_SEG8,
 *         - bit8=>K2_SEG1, bit9=>K2_SEG2, ..., bit15=>K2_SEG8,
 *         - bit16=>K3_SEG1, bit17=>K3_SEG2, ..., bit23=>K3_SEG8,
 * 
 * @retval TM1638_Result_t
 *         - TM1638_OK: Operation was successful
 */
TM1638_Result_t
TM1638_ScanKeys(TM1638_Handler_t *Handler, uint32_t *Keys);



#ifdef __cplusplus
}
#endif

#endif //! _TM1638_H_
