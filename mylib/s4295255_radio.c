/**   
 ******************************************************************************   
 * @file    mylib/s4295255_radio.c 
 * @author  Mani Batra – 42952556   
 * @date    25032015   
 * @brief  Radio peripheral driver   
 *
 *			
 ******************************************************************************   
 *     EXTERNAL FUNCTIONS
 ******************************************************************************
 * s4295255_radio_init() – initialise radio
 * s4295255_radio_setchan(unsigned char chan) – Set the channel of the radio
 * s4295255_radio_settxaddress(unsigned char *addr) – Set the transmit address of the radio
 * s4295255_radio_sendpacket(unsigned char *txpacket) - Send a packet
 * s4295255_radio_getpacket(unsigned char *txpacket) - Recieve a packet
 ******************************************************************************   
 *     REVISION HISTORY
 ******************************************************************************
 * 1. 25/3/2015 - Created
 * 2. 15/4/2015 –  Setting the channel via the SPI commands
 *  
 */


/* Includes ------------------------------------------------------------------*/
#include "board.h"
#include "stm32f4xx_hal_conf.h"
#include "debug_printf.h"
//#include "nrf24l01plus.h"


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define NRF24L01P_CONFIG          0x00	// 'Config' register address
#define NRF24L01P_READ_REG        0x00	// Define read command to register
#define NRF24L01P_EN_AA           0x01	// 'Enable Auto Acknowledgment' register address
#define NRF24L01P_EN_RXADDR       0x02	// 'Enabled RX addresses' register address
#define NRF24L01P_RF_CH           0x05
#define NRF24L01P_RF_SETUP        0x06	// 'RF setup' register address
#define NRF24L01P_STATUS          0x07	// 'Status' register address
#define NRF24L01P_WRITE_REG       0x20	// Define write command to register
#define NRF24L01P_TX_ADDR         0x10	// 'TX address' register address
#define NRF24L01P_RX_PW_P0        0x11	// 'RX payload width, pipe0' register address
#define NRF24L01P_RX_ADDR_P0      0x0A	// 'RX address pipe0' register address

#define NRF24L01P_RD_RX_PLOAD     0x61	// Define RX payload register address
#define NRF24L01P_WR_TX_PLOAD     0xA0	// Define TX payload register address
#define NRF24L01P_FLUSH_RX        0xE2	// Define flush RX register command
#define NRF24L01P_TX_PLOAD_WIDTH  32  // 32 unsigned chars TX payload

#define NRF24L01P_RX_DR    0x40
//#define DEBUG 1
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static SPI_HandleTypeDef SpiHandle;

uint8_t source_address[] = {0x12, 0x34, 0x56, 0x78, 0x90}; ///test address
uint8_t s_a[] = {0x42, 0x95, 0x25, 0x56, 0x11}; //used to set the source address
/* Private function prototypes -----------------------------------------------*/
void writebuffer(uint8_t reg_addr, uint8_t *buffer, int buffer_len);
void write_to_register(uint8_t reg_addr, uint8_t val);
uint8_t readRegister(uint8_t reg_addr);
uint8_t sendRecv_Byte(uint8_t byte);
void mode_rx();
void readBuffer(uint8_t reg_addr, uint8_t *buffer, int buffer_len);
void rfDelay(__IO unsigned long nCount);


/**
  * @brief  Initialise the radio
  * @param  None
  * @retval None

  */

extern void s4295255_radio_init(){

	GPIO_InitTypeDef GPIO_spi;	

	BRD_LEDInit();		//Initialise Blue LED
	BRD_LEDOff();		//Turn off Blue LED

	/* Set SPI clock */
	__BRD_SPI_CLK();

	/* Enable GPIO Pin clocks */
	__BRD_SPI_SCK_GPIO_CLK();
	__BRD_SPI_MISO_GPIO_CLK();
	__BRD_SPI_MOSI_GPIO_CLK();
	__BRD_SPI_CS_GPIO_CLK();
	__BRD_D9_GPIO_CLK();
	
	/* Initialise SPI and Pin clocks*/
	/* SPI SCK pin configuration */
  	GPIO_spi.Pin = BRD_SPI_SCK_PIN;
  	GPIO_spi.Mode = GPIO_MODE_AF_PP;
  	GPIO_spi.Speed = GPIO_SPEED_HIGH;
	GPIO_spi.Pull = GPIO_PULLDOWN;
	GPIO_spi.Alternate = BRD_SPI_SCK_AF;
  	HAL_GPIO_Init(BRD_SPI_SCK_GPIO_PORT, &GPIO_spi);

  	/* SPI MISO pin configuration */
  	GPIO_spi.Pin = BRD_SPI_MISO_PIN;
	GPIO_spi.Mode = GPIO_MODE_AF_PP;
	GPIO_spi.Speed = GPIO_SPEED_FAST;
	GPIO_spi.Pull = GPIO_PULLUP;		//Must be set as pull up
	GPIO_spi.Alternate = BRD_SPI_MISO_AF;
  	HAL_GPIO_Init(BRD_SPI_MISO_GPIO_PORT, &GPIO_spi);

	/* SPI  MOSI pin configuration */
	GPIO_spi.Pin =  BRD_SPI_MOSI_PIN;
	GPIO_spi.Mode = GPIO_MODE_AF_PP;
	GPIO_spi.Pull = GPIO_PULLDOWN;
	GPIO_spi.Alternate = BRD_SPI_MOSI_AF;
  	HAL_GPIO_Init(BRD_SPI_MOSI_GPIO_PORT, &GPIO_spi);

	//Set SPI Module
	SpiHandle.Instance = (SPI_TypeDef *)(BRD_SPI);

	 __HAL_SPI_DISABLE(&SpiHandle);

	/* SPI to Master and 16 baudrate prescaler */
    SpiHandle.Init.Mode              = SPI_MODE_MASTER;
    SpiHandle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    SpiHandle.Init.Direction         = SPI_DIRECTION_2LINES;
    SpiHandle.Init.CLKPhase          = SPI_PHASE_1EDGE;
    SpiHandle.Init.CLKPolarity       = SPI_POLARITY_LOW;
    SpiHandle.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLED;
    SpiHandle.Init.CRCPolynomial     = 0;
    SpiHandle.Init.DataSize          = SPI_DATASIZE_8BIT;
    SpiHandle.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    SpiHandle.Init.NSS               = SPI_NSS_SOFT;
    SpiHandle.Init.TIMode            = SPI_TIMODE_DISABLED;

	/* Initailise SPI parameters */
    HAL_SPI_Init(&SpiHandle);

	/* Enable SPI */
   __HAL_SPI_ENABLE(&SpiHandle);


	/* Configure GPIO PIN for SPI Chip select, TFT CS, TFT DC */
  	GPIO_spi.Pin = BRD_SPI_CS_PIN;				//Pin
  	GPIO_spi.Mode = GPIO_MODE_OUTPUT_PP;		//Output Mode
  	GPIO_spi.Pull = GPIO_PULLUP;			//Enable Pull up, down or no pull resister
  	GPIO_spi.Speed = GPIO_SPEED_FAST;			//Pin latency
  	HAL_GPIO_Init(BRD_SPI_CS_GPIO_PORT, &GPIO_spi);	//Initialise Pin

	/* Configure GPIO PIN for RX/TX mode */
  	GPIO_spi.Pin = BRD_D9_PIN;
  	GPIO_spi.Mode = GPIO_MODE_OUTPUT_PP;		//Output Mode
  	GPIO_spi.Pull = GPIO_PULLUP;			//Enable Pull up, down or no pull resister
  	GPIO_spi.Speed = GPIO_SPEED_FAST;			//Pin latency
  	HAL_GPIO_Init(BRD_D9_GPIO_PORT, &GPIO_spi);	//Initialise Pin

	/* Set chip select high */
	HAL_GPIO_WritePin(BRD_SPI_CS_GPIO_PORT, BRD_SPI_CS_PIN, 1);

}

/**

  * @brief  Set the channel of the radio
  * @param  The channel to set to 
  * @retval None

  */
extern void s4295255_radio_setchan(unsigned char chan){
	
	#ifdef DEBUG
		debug_printf("Channel is: %d\n\r", chan);
	#endif



	uint8_t rxbyte;

	uint8_t byte; 
	byte = NRF24L01P_WRITE_REG | NRF24L01P_RF_CH;

	HAL_GPIO_WritePin(BRD_SPI_CS_GPIO_PORT, BRD_SPI_CS_PIN, 0);
	// *hspi, uint8_t *pTxData, uint8_t *pRxData, uint16_t Size, uint32_t Timeout);
	HAL_SPI_TransmitReceive(&SpiHandle, &byte, &rxbyte, 1, 1);
	rfDelay(0x100);
	byte = chan;
	HAL_SPI_TransmitReceive(&SpiHandle, &byte, &rxbyte, 1, 1);
	rfDelay(0x100);
	HAL_GPIO_WritePin(BRD_SPI_CS_GPIO_PORT, BRD_SPI_CS_PIN, 1);
	byte = 	NRF24L01P_WRITE_REG | NRF24L01P_RF_SETUP;
	HAL_GPIO_WritePin(BRD_SPI_CS_GPIO_PORT, BRD_SPI_CS_PIN, 0);
	HAL_SPI_TransmitReceive(&SpiHandle, &byte , &rxbyte, 1, 1);
	rfDelay(0x100);
	byte = 	0x06;
	HAL_SPI_TransmitReceive(&SpiHandle,&byte , &rxbyte, 1, 1);
	rfDelay(0x100);
	HAL_GPIO_WritePin(BRD_SPI_CS_GPIO_PORT, BRD_SPI_CS_PIN, 1);
	byte = 	NRF24L01P_WRITE_REG | NRF24L01P_CONFIG;
		HAL_GPIO_WritePin(BRD_SPI_CS_GPIO_PORT, BRD_SPI_CS_PIN, 0);
	HAL_SPI_TransmitReceive(&SpiHandle,&byte , &rxbyte, 1, 1);
	rfDelay(0x100);
	byte = 	0x02;
	HAL_SPI_TransmitReceive(&SpiHandle,&byte , &rxbyte, 1, 1);
	rfDelay(0x100);
		HAL_GPIO_WritePin(BRD_SPI_CS_GPIO_PORT, BRD_SPI_CS_PIN, 1);

	

	Delay(0x04FF00);  							// Set PWR_UP bit, enable CRC(2 unsigned chars) & Prim:TX. MAX_RT & TX_DS enabled..


}


/**

  * @brief  Set address of the radio
  * @param  address to which set the radio to
  * @retval None

  */
extern void s4295255_radio_settxaddress(unsigned char *addr){

	/* Set CE low for idle state */
	HAL_GPIO_WritePin(BRD_D9_GPIO_PORT, BRD_D9_PIN, 0);
	
	writebuffer(NRF24L01P_WRITE_REG | NRF24L01P_TX_ADDR, addr, 5);		// Writes TX_Address to nRF24L0
	writebuffer(NRF24L01P_WRITE_REG | NRF24L01P_RX_ADDR_P0, s_a, 5);	//NRF24L01P_TX_ADR_WIDTH);

    write_to_register(NRF24L01P_EN_AA, 0x00);      							// Disable Auto.Ack
    write_to_register(NRF24L01P_EN_RXADDR, 0x01);  							// Enable Pipe0
    write_to_register(NRF24L01P_RX_PW_P0, NRF24L01P_TX_PLOAD_WIDTH); 			// Select same RX payload width as TX Payload width


}


/**
  * @brief  Send the packet via the radio
  * @param  the packet to be sent
  * @retval None

  */
extern void s4295255_radio_sendpacket(unsigned char *txpacket){

	write_to_register(NRF24L01P_CONFIG, 0x72);     // Set PWR_UP bit, enable CRC(2 unsigned chars) & Prim:TX.
    //nrf24l01plus_WriteRegister(NRF24L01P_FLUSH_TX, 0);                                  
    writebuffer(NRF24L01P_WR_TX_PLOAD, txpacket, NRF24L01P_TX_PLOAD_WIDTH);   // write playload to TX_FIFO

	/* Generate 10us pulse on CE pin for transmission */
	rfDelay(0x40);//rfDelay(0x100);
	HAL_GPIO_WritePin(BRD_D9_GPIO_PORT, BRD_D9_PIN, 0);
	rfDelay(0x40);//rfDelay(0x100);
    HAL_GPIO_WritePin(BRD_D9_GPIO_PORT, BRD_D9_PIN, 1);	//Set CE pin low to enable TX mode
	rfDelay(0x40*3);	//rfDelay(0x100);
	HAL_GPIO_WritePin(BRD_D9_GPIO_PORT, BRD_D9_PIN, 0);
	//rfDelay(0x40*132);
	Delay(0x04FF00);	
	mode_rx();

}

/**
  * @brief  get the packet via the radio
  * @param  the buffer to receive the packet
  * @retval 1 if the receive is enabled or -1 else

  */
extern int s4295255_radio_getpacket(unsigned char *txpacket){

	mode_rx();

	int rec = 0;
    unsigned char status = readRegister(NRF24L01P_STATUS);                  // read register STATUS's value

#ifdef DEBUG
	debug_printf("DEBUG:RCV packet status: %X\n\r", status & NRF24L01P_RX_DR);
#endif

	
    //if((status & NRF24L01P_RX_DR) && !nrf24l01plus_rxFifoEmpty()) {    // if receive data ready interrupt and FIFO full.
	if(status & NRF24L01P_RX_DR) {
		rfDelay(0x100);
        readBuffer(NRF24L01P_RD_RX_PLOAD, txpacket, NRF24L01P_TX_PLOAD_WIDTH);  // read playload to rx_buf
        write_to_register(NRF24L01P_FLUSH_RX,0);                             // clear RX_FIFO
        rec = 1; //can be used for debugging

		HAL_GPIO_WritePin(BRD_D9_GPIO_PORT, BRD_D9_PIN, 0);
		rfDelay(0x100);

		write_to_register(NRF24L01P_STATUS, status);                  // clear RX_DR or TX_DS or MAX_RT interrupt flag
    }  

	return rec;

}


/**
  * @brief  write to the buffer
  * @param  the packet to be sent
  * @retval None

  */

void writebuffer(uint8_t reg_addr, uint8_t *buffer, int buffer_len){

	int i;

	HAL_GPIO_WritePin(BRD_SPI_CS_GPIO_PORT, BRD_SPI_CS_PIN, 0);
	rfDelay(0x8FF);
	sendRecv_Byte(NRF24L01P_WRITE_REG | reg_addr);

	rfDelay(0x7FFF00/950);


	for (i = 0; i < buffer_len; i++) {
		
		/* Return the Byte read from the SPI bus */
		sendRecv_Byte(buffer[i]);
		rfDelay(0x100);	
#ifdef DEBUG
		//debug_printf("%X ", buffer[i]);
#endif
	}

#ifdef DEBUG
	//debug_printf("\n\r");
#endif
	rfDelay(0x8FF);
	HAL_GPIO_WritePin(BRD_SPI_CS_GPIO_PORT, BRD_SPI_CS_PIN, 1);
	rfDelay(0x8FF);

}

/**
  * @brief  Write to the registor
  * @param  The registor to write to and the value to write
  * @retval None

  */
void write_to_register(uint8_t reg_addr, uint8_t val){

	HAL_GPIO_WritePin(BRD_SPI_CS_GPIO_PORT, BRD_SPI_CS_PIN, 0);

	sendRecv_Byte(NRF24L01P_WRITE_REG | reg_addr);
	sendRecv_Byte(val);

	HAL_GPIO_WritePin(BRD_SPI_CS_GPIO_PORT, BRD_SPI_CS_PIN, 1);

}

/**
  * @brief  send the byte via the radio
  * @param  byte to send
  * @retval None

  */
uint8_t sendRecv_Byte(uint8_t byte) {

	uint8_t rxbyte;

	// *hspi, uint8_t *pTxData, uint8_t *pRxData, uint16_t Size, uint32_t Timeout);
	HAL_SPI_TransmitReceive(&SpiHandle, &byte, &rxbyte, 1, 1);

	rfDelay(0x100);	
		
	return rxbyte; 
}


/**
  * @brief  Read the value of the register
  * @param  the register to read
  * @retval None

  */
uint8_t readRegister(uint8_t reg_addr) {

	uint8_t rxbyte;

	HAL_GPIO_WritePin(BRD_SPI_CS_GPIO_PORT, BRD_SPI_CS_PIN, 0);

	rxbyte = sendRecv_Byte(reg_addr);
	rxbyte = sendRecv_Byte(0xFF);

	HAL_GPIO_WritePin(BRD_SPI_CS_GPIO_PORT, BRD_SPI_CS_PIN, 1);
		
	return rxbyte; 
}

/**
  * @brief  REad the buffer from the radio
  * @param  the buffer to store to and the length
  * @retval None

  */
void readBuffer(uint8_t reg_addr, uint8_t *buffer, int buffer_len) {

	int i;
	
	HAL_GPIO_WritePin(BRD_SPI_CS_GPIO_PORT, BRD_SPI_CS_PIN, 0);

	sendRecv_Byte(reg_addr);
	rfDelay(0x8FF);

#ifdef DEBUG
	//debug_printf("DEBUG:RB ");
#endif
	for (i = 0; i < buffer_len; i++) {
		
		/* Return the Byte read from the SPI bus */
		buffer[i] = sendRecv_Byte(0xFF);

#ifdef DEBUG
		//debug_printf("%X ", buffer[i]);
#endif

		rfDelay(0x8FF);

	}

#ifdef DEBUG
	//debug_printf("\n\r");
#endif
	rfDelay(0x8FF);
	HAL_GPIO_WritePin(BRD_SPI_CS_GPIO_PORT, BRD_SPI_CS_PIN, 1);
	rfDelay(0x8FF);	 
}

/**
  * @brief  change to receive mode
  * @param  none
  * @retval None

  */
void mode_rx() {

    write_to_register(NRF24L01P_CONFIG, 0x73);	//0x0f     	// Set PWR_UP bit, enable CRC(2 unsigned chars) & Prim:RX. 
    HAL_GPIO_WritePin(BRD_D9_GPIO_PORT, BRD_D9_PIN, 1);                            		// Set CE pin high to enable RX device
}

/**
  * @brief  delay
  * @param  none
  * @retval None

  */
void rfDelay(__IO unsigned long nCount) {
  while(nCount--) {
  }
}

