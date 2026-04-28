#ifndef _PIO_I2C_H_
#define _PIO_I2C_H_

#include <stdbool.h>
#include <stdint.h>



struct I2Creq {
	bool haveNext;				//set if this is not the last req in the chain
	uint8_t addr;				//7bit
	const uint8_t *txData;		//non-NULL means TX is done (even of zero length)
	uint8_t *txAcks;			//if NULL, you'll get no report on ACKs. always one larger than txLen since ADDR ack is reported too
	uint32_t txLen;
	uint8_t *rxAddrAck;			//ack for RX addr will go here if not NULL
	uint8_t *rxData;			//non-NULL means RX is done (even of zero length)
	const uint8_t *rxAcks;		//if NULL, we'll ACK all but last byte
	uint32_t rxLen;
};

void i2cInit(void);

typedef void (*I2CtransactionOverCbkF)(void *userData, const struct I2Creq *req, bool likelySuccess);
bool i2cTransact(const struct I2Creq *req, I2CtransactionOverCbkF cbkF, void *cbkD);			//"req" and "req->txAck" buffers must remain valid until callback. callback can be called before this even returns...
bool i2cPrvIsBuysy(void);



//utilities that are nicer than the raw interface above
bool i2cSimpleWrite(uint_fast8_t sevenBitAddr, const uint8_t *vals, uint32_t numBytes);
bool i2cSimpleRead(uint_fast8_t sevenBitAddr, uint8_t *vals, uint32_t numBytes);
bool i2cRegRead(uint_fast8_t sevenBitAddr, uint8_t reg, uint8_t *vals, uint32_t numBytes);
bool i2cOneByteRegWrite(uint_fast8_t sevenBitAddr, uint8_t reg, uint8_t val);


#endif
