#ifndef _PIO_IRDA_SIR_H_
#define _PIO_IRDA_SIR_H_

#include <stdbool.h>
#include <stdint.h>


union UartCfg {
	struct {
		uint32_t baudrate	:24;
		uint32_t charBits	:2;	//{5,6,7,8}
		uint32_t stopBits	:2;	//{0.5, 1, 1.5, 2}
		uint32_t parEna		:1;
		uint32_t parEven	:1;
		uint32_t rxEn		:1;	//this and next off for port OFF
		uint32_t txEn		:1;
	};
	uint32_t raw32;
};
#define UART_BIT_MASK_PAR_ERR		0x0100
#define UART_BIT_MASK_BRK_RXED		0x0200	//not really used in IR
#define UART_BIT_MASK_FRM_ERR		0x0400
#define UART_BIT_MASK_RX_OVERRUN	0x0800

#define UART_STA_BIT_TX_FIFO_EMPTY	0x00000001
#define UART_STA_BIT_TX_FIFO_FULL	0x00000002
#define UART_STA_BIT_RX_FIFO_EMPTY	0x00000004


//rawBuf[] items have RXed data in low bits and UART_BIT_MASK_* flags ORRed in
typedef void (*IrdaSIRuartRxF)(void *userData, uint16_t *rawBuf, uint32_t nItems);

bool irdaSIRuartConfig(union UartCfg *cfg, IrdaSIRuartRxF rxf, void *userData);							//cfg modified to what you got
uint32_t irdaSIRuartTx(const uint8_t *data, uint32_t len, bool block);									//call with NULL data and nonzero len to send break, NULL data and zero len to stop break
uint32_t irdaSIRuartGetSta(void);																		//clears returned stausses if they signal errors




#endif
