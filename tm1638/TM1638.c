#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "TM1638.h"
#include "esp_log.h"

#if CONFIG_SPI_INTERFACE
  #include "driver/spi_master.h"
#elif CONFIG_GPIO_INTERFACE

#else 
	#error You have to select INTERFACE (GPIO or SPI)
#endif


/**
 * @brief  Instruction description
 */
#define DataInstructionSet            0x40  // 0b01000000
#define DisplayControlInstructionSet  0x80  // 0b10000000
#define AddressInstructionSet         0xC0  // 0b11000000

/**
 * @brief  Data instruction set
 */
#define WriteDataToRegister   0x00  // 0b00000000
#define ReadKeyScanData       0x02  // 0b00000010
#define AutoAddressAdd        0x00  // 0b00000000
#define FixedAddress          0x04  // 0b00000100
#define NormalMode            0x00  // 0b00000000
#define TestMode              0x08  // 0b00001000

/**
 * @brief  Display ControlInstruction Set
 */
#define ShowTurnOff   0x00  // 0b00000000
#define ShowTurnOn    0x08  // 0b00001000


/**
 * @brief  Convert HEX number to Seven-Segment code
 */
const uint8_t HexTo7Seg[16] =
{
  0x3F, // 0
  0x06, // 1
  0x5B, // 2
  0x4F, // 3
  0x66, // 4
  0x6D, // 5
  0x7D, // 6
  0x07, // 7
  0x7F, // 8
  0x6F, // 9
  0x77, // A
  0x7c, // b
  0x39, // C
  0x5E, // d
  0x79, // E
  0x71  // F
};

inline void TM1638_SetGPIO_OUT(gpio_num_t GPIO_Pad)
{
  gpio_reset_pin(GPIO_Pad);
  gpio_set_direction(GPIO_Pad, GPIO_MODE_OUTPUT);
}

inline void TM1638_SetGPIO_IN_PU(gpio_num_t GPIO_Pad)
{
  gpio_reset_pin(GPIO_Pad);
  gpio_set_direction(GPIO_Pad, GPIO_MODE_INPUT);
  gpio_set_pull_mode(GPIO_Pad, GPIO_PULLUP_ONLY);
}

static void
TM1638_PlatformInit(void)
{
#ifndef CONFIG_SPI_INTERFACE  
  TM1638_SetGPIO_OUT(TM1638_CLK_GPIO);
  TM1638_SetGPIO_OUT(TM1638_DIO_GPIO);
#endif
  TM1638_SetGPIO_OUT(TM1638_STB_GPIO);
}

static void
TM1638_PlatformDeInit(void *H)
{
#if CONFIG_SPI_INTERFACE  
  TM1638_Handler_t * Handler;
  Handler = (TM1638_Handler_t *)H;
  spi_device_release_bus(Handler->SPIHandle);
  spi_bus_remove_device(Handler->SPIHandle);
  spi_bus_free(HSPI_HOST);
#endif
  gpio_reset_pin(TM1638_CLK_GPIO);
  gpio_reset_pin(TM1638_STB_GPIO);
  gpio_reset_pin(TM1638_DIO_GPIO);
 
}

static uint8_t
TM1638_DioRead(void)
{
  uint8_t Result = 1;
#if CONFIG_SPI_INTERFACE 
#else 
  Result = gpio_get_level(TM1638_DIO_GPIO);
#endif  
  return Result;
}

#ifndef CONFIG_SPI_INTERFACE  
static inline void
TM1638_DioConfigIn(void)
{
  TM1638_SetGPIO_IN_PU(TM1638_DIO_GPIO);
}

static inline void
TM1638_DioConfigOut(void)
{
  TM1638_SetGPIO_OUT(TM1638_DIO_GPIO);
}

static inline void
TM1638_DioWrite(uint8_t Level)
{
  gpio_set_level(TM1638_DIO_GPIO, Level);
}

static inline void
TM1638_ClkWrite(uint8_t Level)
{
  gpio_set_level(TM1638_CLK_GPIO, Level);
}

static void
TM1638_DelayUs(uint8_t Delay)
{
  ets_delay_us(Delay);
}
#endif

static inline void
TM1638_StbWrite(uint8_t Level)
{
  gpio_set_level(TM1638_STB_GPIO, Level);
}

static inline void
TM1638_StartComunication(TM1638_Handler_t *Handler)
{
  Handler->StbWrite(0);
}

static inline void
TM1638_StopComunication(TM1638_Handler_t *Handler)
{
  Handler->StbWrite(1);
}

static void
TM1638_WriteBytes(TM1638_Handler_t *Handler, const uint8_t *Data, uint8_t NumOfBytes)
{
#if CONFIG_SPI_INTERFACE 	
  	spi_transaction_t SPITransaction;
	if ( NumOfBytes > 0 ) {
		memset( &SPITransaction, 0, sizeof( spi_transaction_t ) );
		SPITransaction.length = NumOfBytes * 8;
		SPITransaction.tx_buffer = Data;
		spi_device_transmit( Handler->SPIHandle, &SPITransaction );
	}
#else
  uint8_t i, j, Buff;
  Handler->DioConfigOut();

  for (j = 0; j < NumOfBytes; j++)
  {
    for (i = 0, Buff = Data[j]; i < 8; ++i, Buff >>= 1)
    {
      Handler->ClkWrite(0);
      Handler->DelayUs(1);
      Handler->DioWrite(Buff & 0x01);
      Handler->ClkWrite(1);
      Handler->DelayUs(1);
    }
  }
#endif
}

static void
TM1638_ReadBytes(TM1638_Handler_t *Handler, uint8_t *Data, uint8_t NumOfBytes)
{
#if CONFIG_SPI_INTERFACE 	
    spi_transaction_t SPITransaction = {
        .cmd = 0x00,
        .rxlength = 8 * NumOfBytes,
        .flags = 0,
        .tx_buffer = NULL,
        .rx_buffer = Data,
    };
		spi_device_transmit( Handler->SPIHandle, &SPITransaction );

#else
  uint8_t i, j, Buff;

  Handler->DioConfigIn();

  Handler->DelayUs(5);

  for (j = 0; j < NumOfBytes; j++)
  {
    for (i = 0, Buff = 0; i < 8; i++)
    {
      Handler->ClkWrite(0);
      Handler->DelayUs(1);
      Handler->ClkWrite(1);
      Buff |= (Handler->DioRead() << i);
      Handler->DelayUs(1);
    }

    Data[j] = Buff;
    Handler->DelayUs(2);
  }
#endif
}

static void
TM1638_SetMultipleDisplayRegister(TM1638_Handler_t *Handler, const uint8_t *DigitData, uint8_t StartAddr, uint8_t Count)
{
  uint8_t Data = DataInstructionSet | WriteDataToRegister |
                 AutoAddressAdd | NormalMode;

  TM1638_StartComunication(Handler);
  TM1638_WriteBytes(Handler, &Data, 1);
  TM1638_StopComunication(Handler);

  Data = AddressInstructionSet | StartAddr;

  TM1638_StartComunication(Handler);
  TM1638_WriteBytes(Handler, &Data, 1);
  TM1638_WriteBytes(Handler, DigitData, Count);
  TM1638_StopComunication(Handler);
}

static void
TM1638_ScanKeyRegs(TM1638_Handler_t *Handler, uint8_t *KeyRegs)
{
  uint8_t Data = DataInstructionSet | ReadKeyScanData | AutoAddressAdd | NormalMode;

  TM1638_StartComunication(Handler);
  TM1638_WriteBytes(Handler, &Data, 1);
  TM1638_ReadBytes(Handler, KeyRegs, 4);
  TM1638_StopComunication(Handler);
}



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
TM1638_Result_t TM1638_Init(TM1638_Handler_t *Handler, uint8_t Type)
{
#ifndef CONFIG_SPI_INTERFACE 	
  Handler->DioConfigOut = TM1638_DioConfigOut;
  Handler->DioConfigIn = TM1638_DioConfigIn;
  Handler->DioWrite = TM1638_DioWrite;
  Handler->ClkWrite = TM1638_ClkWrite;
  Handler->DelayUs = TM1638_DelayUs;
#endif
  Handler->PlatformInit = TM1638_PlatformInit;
  Handler->PlatformDeInit = TM1638_PlatformDeInit;
  Handler->DioRead = TM1638_DioRead;
  Handler->StbWrite = TM1638_StbWrite;
  Handler->DisplayType = TM1638DisplayTypeComCathode;

  for (uint8_t i = 0; i < 16; i++)
  {
    Handler->DisplayRegister[i] = 0;
  }
  
  if (Type == TM1638DisplayTypeComCathode){
    Handler->DisplayType = TM1638DisplayTypeComCathode;
  } else {
    Handler->DisplayType = TM1638DisplayTypeComAnode;
  }

  Handler->PlatformInit();
  
#if CONFIG_SPI_INTERFACE 
  gpio_reset_pin(TM1638_CLK_GPIO);
  gpio_reset_pin(TM1638_DIO_GPIO);
	esp_err_t ret;
	
  spi_bus_config_t spi_bus_config = {
		.mosi_io_num = TM1638_DIO_GPIO,
		.miso_io_num = -1,
		.sclk_io_num = TM1638_CLK_GPIO,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,
		.max_transfer_sz = 0,
		.flags = 0
	};
	
	ret = spi_bus_initialize( HSPI_HOST, &spi_bus_config, SPI_DMA_CH_AUTO );
	assert(ret==ESP_OK);
  ESP_LOGI("TM1638", "SPI BUS INITIALIZED");

  spi_device_interface_config_t devcfg;
	memset( &devcfg, 0, sizeof( spi_device_interface_config_t ) );
	devcfg.clock_speed_hz = CONFIG_SPI_FREQUENCY; //1000000; // 1MHz
	devcfg.spics_io_num = -1;
	devcfg.queue_size = 1;
	devcfg.mode = 3;
	devcfg.flags = SPI_DEVICE_BIT_LSBFIRST;

	spi_device_handle_t handle;
	ret = spi_bus_add_device( HSPI_HOST, &devcfg, &handle);
	assert(ret==ESP_OK);
  ESP_LOGI("TM1638", "SPI DEVICE ADDED");
	
	Handler->SPIHandle = handle;
#endif  
  
  return TM1638_OK;
}

/**
 * @brief  De-Initialize TM1638.
 * @param  Handler: Pointer to handler
 * @retval TM1638_Result_t
 *         - TM1638_OK: Operation was successful.
 */
TM1638_Result_t
TM1638_DeInit(TM1638_Handler_t *Handler)
{
  Handler->PlatformDeInit(Handler);
  return TM1638_OK;
}



/**
 ==================================================================================
                        ##### Public Display Functions #####                       
 ==================================================================================
 */

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
                     uint8_t Brightness, uint8_t DisplayState)
{
  uint8_t Data = DisplayControlInstructionSet;
  Data |= Brightness & 0x07;
  Data |= (DisplayState) ? (ShowTurnOn) : (ShowTurnOff);

  TM1638_StartComunication(Handler);
  TM1638_WriteBytes(Handler, &Data, 1);
  TM1638_StopComunication(Handler);

  return TM1638_OK;
}


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
                      uint8_t DigitData, uint8_t DigitPos)
{ 
  if (Handler->DisplayType == TM1638DisplayTypeComCathode){
    TM1638_SetMultipleDisplayRegister(Handler, &DigitData, DigitPos, 1);
  }else{
    TM1638_SetMultipleDigit(Handler, &DigitData, DigitPos, 1);
  }
  return TM1638_OK;
}


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
TM1638_SetMultipleDigit(TM1638_Handler_t *Handler, const uint8_t *DigitData, uint8_t StartAddr, uint8_t Count)
{
  uint8_t Shift = 0;
  uint8_t DigitDataBuff = 0;
  uint8_t i = 0, j = 0;

  if (Handler->DisplayType == TM1638DisplayTypeComCathode)
    TM1638_SetMultipleDisplayRegister(Handler, DigitData, StartAddr, Count);
  else
  {
    for (j = 0; j < Count; j++)
    {
      DigitDataBuff = DigitData[j];

      if ((j + StartAddr) >= 0 && (j + StartAddr) <= 7)
      {
        Shift = j + StartAddr;
        i = 0;
      }
      else if ((j + StartAddr) == 8 || (j + StartAddr) == 9)
      {
        Shift = (j + StartAddr) - 8;
        i = 1;
      }
      else
      {
        i = 16;
      }

      for (; i < 16; i += 2, DigitDataBuff >>= 1)
      {
        if (DigitDataBuff & 0x01)
          Handler->DisplayRegister[i] |= (1 << Shift);
        else
          Handler->DisplayRegister[i] &= ~(1 << Shift);
      }
    }
    TM1638_SetMultipleDisplayRegister(Handler, Handler->DisplayRegister, 0, 16);
  }

  return TM1638_OK;
}

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
                          uint8_t DigitData, uint8_t DigitPos)
{
  uint8_t DigitDataHEX = 0;
  uint8_t DecimalPoint = DigitData & 0x80;

  DigitData &= 0x7F;

  if (DigitData <= 15)
  {
    DigitDataHEX = HexTo7Seg[DigitData] | DecimalPoint;
  }
  else
  {
    switch (DigitData)
    {
    case 'A':
    case 'a':
      DigitDataHEX = HexTo7Seg[0x0A] | DecimalPoint;
      break;

    case 'B':
    case 'b':
      DigitDataHEX = HexTo7Seg[0x0B] | DecimalPoint;
      break;

    case 'C':
    case 'c':
      DigitDataHEX = HexTo7Seg[0x0C] | DecimalPoint;
      break;

    case 'D':
    case 'd':
      DigitDataHEX = HexTo7Seg[0x0D] | DecimalPoint;
      break;

    case 'E':
    case 'e':
      DigitDataHEX = HexTo7Seg[0x0E] | DecimalPoint;
      break;

    case 'F':
    case 'f':
      DigitDataHEX = HexTo7Seg[0x0F] | DecimalPoint;
      break;

    default:
      DigitDataHEX = 0;
      break;
    }
  }

  return TM1638_SetSingleDigit(Handler, DigitDataHEX, DigitPos);
}


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
                            uint8_t StartAddr, uint8_t Count)
{
  uint8_t DigitDataHEX[10];
  uint8_t DecimalPoint = 0;

  for (uint8_t i = 0; i < Count; i++)
  {
    DecimalPoint = DigitData[i] & 0x80;

    if ((DigitData[i] & 0x7F) >= 0 && (DigitData[i] & 0x7F) <= 15)
    {
      DigitDataHEX[i] = HexTo7Seg[DigitData[i] & 0x7F] | DecimalPoint;
    }
    else
    {
      switch (DigitData[i] & 0x7F)
      {
      case 'A':
      case 'a':
        DigitDataHEX[i] = HexTo7Seg[0x0A] | DecimalPoint;
        break;

      case 'B':
      case 'b':
        DigitDataHEX[i] = HexTo7Seg[0x0B] | DecimalPoint;
        break;

      case 'C':
      case 'c':
        DigitDataHEX[i] = HexTo7Seg[0x0C] | DecimalPoint;
        break;

      case 'D':
      case 'd':
        DigitDataHEX[i] = HexTo7Seg[0x0D] | DecimalPoint;
        break;

      case 'E':
      case 'e':
        DigitDataHEX[i] = HexTo7Seg[0x0E] | DecimalPoint;
        break;

      case 'F':
      case 'f':
        DigitDataHEX[i] = HexTo7Seg[0x0F] | DecimalPoint;
        break;

      default:
        DigitDataHEX[i] = 0;
        break;
      }
    }
  }

  return TM1638_SetMultipleDigit(Handler,
                                 (const uint8_t *)DigitDataHEX, StartAddr, Count);
}


/** 
 ==================================================================================
                      ##### Public Keypad Functions #####                         
 ==================================================================================
 */

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
TM1638_ScanKeys(TM1638_Handler_t *Handler, uint32_t *Keys)
{
  uint8_t KeyRegs[4];
  uint32_t KeysBuff = 0;
  uint8_t Kn = 0x01;

  TM1638_ScanKeyRegs(Handler, KeyRegs);

  for (uint8_t i = 0; i < 3; i++)
  {
    for (int8_t i = 3; i >= 0; i--)
    {
      KeysBuff <<= 1;

      if (KeyRegs[i] & (Kn << 4))
        KeysBuff |= 1;

      KeysBuff <<= 1;

      if (KeyRegs[i] & Kn)
        KeysBuff |= 1;
    }

    Kn <<= 1;
  }

  *Keys = KeysBuff;

  return TM1638_OK;
}
