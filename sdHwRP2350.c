#include "hardware/regs/dreq.h"
#include "pinoutRp2350defcon.h"
#include "timebase.h"
#include <string.h>
#include "printf.h"
#include "2350.h"
#include "sdHw.h"
#include "sd.h"


static uint16_t mTimeoutBytes;
static uint32_t mRdTimeoutTicks, mWrTimeoutTicks;
static volatile uint32_t mTrash;

#define SD_DMA_CH_READ		(SD_DMA_FIRST + 0)
#define SD_DMA_CH_WRITE		(SD_DMA_FIRST + 1)


void sdHwNotifyRCA(uint_fast16_t rca)
{
	//nothing
}

void sdHwSetTimeouts(uint_fast16_t timeoutBytes, uint32_t rdTimeoutTicks, uint32_t wrTimeoutTicks)
{
	mTimeoutBytes = timeoutBytes;
	mRdTimeoutTicks = rdTimeoutTicks;
	mWrTimeoutTicks = wrTimeoutTicks;
}

bool sdHwSetBusWidth(bool useFourWide)
{
	return !useFourWide;
}

static void sdHwPrvSetBrg(uint_fast16_t brg)
{
	uint_fast16_t scr = 1, dvsr = 1, brgTry, i;
		
	//brg must be even
	if (brg % 2)
		brg++;
	
	do {
		
		brgTry = brg;
		brg += 2;
	
		//brg is aproduct of two values (DVSR and SCR), one of which (DVSR) must be even. each is 8 bits long
		while (brgTry > 1) {
			for (i = 2; i <= brgTry && brgTry % i; i++);
						
			if (i == brgTry + 1)	//unattainable
				break;
			
			brgTry /= i;
			if (dvsr * i < 0x100)
				dvsr *= i;
			else
				scr *= i;
		}
	} while (brgTry != 1);
	
	while (spi1_hw->sr & SPI_SSPSR_BSY_BITS);
	spi1_hw->cr1 &=~ SPI_SSPCR1_SSE_BITS;
	spi1_hw->cr0 = (spi1_hw->cr0 &~ SPI_SSPCR0_SCR_BITS) | ((scr - 1) << SPI_SSPCR0_SCR_LSB);
	spi1_hw->cpsr = (spi1_hw->cpsr &~ (SPI_SSPCPSR_CPSDVSR_BITS)) | (dvsr << SPI_SSPCPSR_CPSDVSR_LSB);		//div 8 (in addition to the div 256 above)
	spi1_hw->cr1 |= SPI_SSPCR1_SSE_BITS;
}

void sdHwSetSpeed(uint32_t maxSpeed)
{
	//XXX: this driver and chip can go fast. the shitty SD card chosen for this project cannot.
	//it advertises HIGH SPEED SIGNALLING support, which implies it can handle clocks up to 52MHz
	//in reality it chokes on clocks over 1MHz, and thus this limit here. We considered trying to
	//identify it so as to not cripple other card, but given that this was found at the 11th hour,
	//we did not go that route, so enjoy your meditative SD card experience...huuuuummmmmmm.....
	if (maxSpeed > 500000)
		maxSpeed = 500000;

	sdHwPrvSetBrg((TICKS_PER_SECOND + maxSpeed - 1) / maxSpeed);
}

uint32_t sdHwInit(void)
{
	uint_fast8_t i;
	
	//configure SPI
	spi1_hw->cr1 &=~ SPI_SSPCR1_SSE_BITS;
	spi1_hw->cr0 = (spi1_hw->cr0 &~ (SPI_SSPCR0_SCR_BITS | SPI_SSPCR0_SPH_BITS | SPI_SSPCR0_SPO_BITS | SPI_SSPCR0_FRF_BITS | SPI_SSPCR0_DSS_BITS)) | ((256 - 1) << SPI_SSPCR0_SCR_LSB) | ((8 - 1) << SPI_SSPCR0_DSS_LSB);
	spi1_hw->cr1 &=~ SPI_SSPCR1_MS_BITS;
	spi1_hw->cpsr = (spi1_hw->cpsr &~ (SPI_SSPCPSR_CPSDVSR_BITS)) | (8 << SPI_SSPCPSR_CPSDVSR_LSB);		//div 8 (in addition to the div 256 above)
	spi1_hw->dmacr &=~ (SPI_SSPDMACR_TXDMAE_BITS | SPI_SSPDMACR_RXDMAE_BITS);
	spi1_hw->cr1 |= SPI_SSPCR1_SSE_BITS;
	
	//configure DMA channels 2 (for read) & 3 (for write). default cfg optimized for reads
	dma_hw->ch[SD_DMA_CH_READ].read_addr = (uintptr_t)&spi1_hw->dr;
	dma_hw->ch[SD_DMA_CH_READ].transfer_count = SD_BLOCK_SIZE / sizeof(uint16_t);
	dma_hw->ch[SD_DMA_CH_READ].al1_ctrl = (dma_hw->ch[SD_DMA_CH_READ].al1_ctrl &~ (DMA_CH2_CTRL_TRIG_TREQ_SEL_BITS | DMA_CH2_CTRL_TRIG_INCR_WRITE_BITS | DMA_CH2_CTRL_TRIG_INCR_READ_BITS | DMA_CH6_CTRL_TRIG_DATA_SIZE_BITS | DMA_CH0_CTRL_TRIG_CHAIN_TO_BITS)) | (SD_DMA_CH_READ << DMA_CH0_CTRL_TRIG_CHAIN_TO_LSB) | (DREQ_SPI1_RX << DMA_CH2_CTRL_TRIG_TREQ_SEL_LSB) | (DMA_CH2_CTRL_TRIG_DATA_SIZE_VALUE_SIZE_HALFWORD << DMA_CH2_CTRL_TRIG_DATA_SIZE_LSB) | DMA_CH2_CTRL_TRIG_EN_BITS | DMA_CH2_CTRL_TRIG_BSWAP_BITS | DMA_CH2_CTRL_TRIG_INCR_WRITE_BITS;
	
	dma_hw->ch[SD_DMA_CH_WRITE].write_addr = (uintptr_t)&spi1_hw->dr;
	dma_hw->ch[SD_DMA_CH_WRITE].transfer_count = SD_BLOCK_SIZE / sizeof(uint16_t);
	dma_hw->ch[SD_DMA_CH_WRITE].al1_ctrl = (dma_hw->ch[SD_DMA_CH_WRITE].al1_ctrl &~ (DMA_CH3_CTRL_TRIG_TREQ_SEL_BITS | DMA_CH3_CTRL_TRIG_INCR_WRITE_BITS | DMA_CH3_CTRL_TRIG_INCR_READ_BITS | DMA_CH3_CTRL_TRIG_DATA_SIZE_BITS | DMA_CH0_CTRL_TRIG_CHAIN_TO_BITS)) | (SD_DMA_CH_WRITE << DMA_CH0_CTRL_TRIG_CHAIN_TO_LSB) | (DREQ_SPI1_TX << DMA_CH3_CTRL_TRIG_TREQ_SEL_LSB) | (DMA_CH3_CTRL_TRIG_DATA_SIZE_VALUE_SIZE_HALFWORD << DMA_CH3_CTRL_TRIG_DATA_SIZE_LSB) | DMA_CH3_CTRL_TRIG_EN_BITS | DMA_CH3_CTRL_TRIG_BSWAP_BITS;

	sdHwSetSpeed(400000);
	
	//default timeouts
	sdHwSetTimeouts(10000, 0, 0);
	
	return SD_HW_FLAG_INITED;
}

static uint8_t sdSpiByte(uint_fast8_t val)
{
	while (!(spi1_hw->sr & SPI_SSPSR_TNF_BITS));
	spi1_hw->dr = val;
	while (!(spi1_hw->sr & SPI_SSPSR_RNE_BITS));
	return spi1_hw->dr;
}

void sdHwGiveInitClocks(void)
{
	uint_fast8_t i;
	
	for (i = 0; i < 16; i++)	//give card time to init with CS deasserted
		sdSpiByte(0xff);
}

static void sdChipSelect(void)
{
	sio_hw->gpio_clr = 1 << PIN_SD_NCS;
}

void sdHwChipDeselect(void)
{
	while (spi1_hw->sr & SPI_SSPSR_BSY_BITS);
	sio_hw->gpio_set = 1 << PIN_SD_NCS;
	sdSpiByte(0xff);
	while (spi1_hw->sr & SPI_SSPSR_BSY_BITS);
}

static uint_fast8_t sdCrc7(uint_fast8_t data, uint_fast8_t crc)
{
	uint_fast8_t i;
	
	for (i = 0; i < 8; i++, data <<= 1) {
		
		crc <<= 1;

		if ((data ^ crc) & 0x80)
			crc ^= 0x09;
	}
	
	return crc & 0x7F;
}

static void sdPrvSendCmd(uint8_t cmd, uint32_t param, bool cmdCrcRequired)
{
	uint_fast8_t crc = 0;
	
	cmd |= 0x40;
	
	if (cmdCrcRequired) {
		
		crc = sdCrc7(cmd, crc);
		crc = sdCrc7(param >> 24, crc);
		crc = sdCrc7(param >> 16, crc);
		crc = sdCrc7(param >> 8, crc);
		crc = sdCrc7(param, crc);
	}
	
	sdSpiByte(cmd);
	sdSpiByte(param >> 24);
	sdSpiByte(param >> 16);
	sdSpiByte(param >> 8);
	sdSpiByte(param >> 0);
	sdSpiByte(crc * 2 + 1);
}

enum SdHwCmdResult sdHwCmd(uint_fast8_t cmd, uint32_t param, bool cmdCrcRequired, enum SdHwRespType respTyp, void *respBufOut, enum SdHwDataDir dataDir, uint_fast16_t blockSz, uint32_t numBlocks)
{
	uint8_t *rsp = (uint8_t*)respBufOut;
	uint_fast8_t ret, i = 0;
		
	sdChipSelect();
	
	if (cmd & 0x40) {		//cmd is usually 0..0x3f, we use 0x40 bit to signal that init failed, and we need to bit-resync. try that by injecting a bit
		
		//send an extra clock pulse some cards get into a weird state on boot, this will resync with them
		iobank0_hw->io[18].ctrl = (iobank0_hw->io[18].ctrl &~ IO_BANK0_GPIO18_CTRL_FUNCSEL_BITS) | IO_BANK0_GPIO18_CTRL_FUNCSEL_VALUE_SIO_18;
		delayMsec(1);
		sio_hw->gpio_set = 1 << 18;
		delayMsec(1);
		sio_hw->gpio_clr = 1 << 18;
		delayMsec(1);
		iobank0_hw->io[18].ctrl = (iobank0_hw->io[18].ctrl &~ IO_BANK0_GPIO18_CTRL_FUNCSEL_BITS) | IO_BANK0_GPIO18_CTRL_FUNCSEL_VALUE_SPI0_SCLK;
	}
	
	sdPrvSendCmd(cmd, param, cmdCrcRequired);
	
	if (cmd == 12)		//do not ask!
		sdSpiByte(0xff);
	
	while ((ret = sdSpiByte(0xff)) == 0xff) {
		
		if (++i == 128) {
			sdHwChipDeselect();
			return SdHwCmdResultRespTimeout;
		}
	}
	
	switch (respTyp) {
		case SdRespTypeNone:
			break;
		
		case SdRespTypeR1:
		case SdRespTypeR1withBusy:
			*rsp++ = ret;
			break;
		
		case SdRespTypeR3:
		case SdRespTypeR7:
			if (ret & FLAG_ILLEGAL_CMD) {
				sdHwChipDeselect();
				return SdCmdInvalid;
			}
			if (ret &~ FLAG_IN_IDLE_MODE) {
				sdHwChipDeselect();
				return SdCmdInternalError;
			}
			for (i = 0; i < 4; i++)
				*rsp++ = sdSpiByte(0xff);
			break;
		
		case SdRespTypeSpiR2:
			if (sdSpiByte(0xff))
				ret |= FLAG_MISC_ERR;
			*rsp = ret;
			if (ret &~ FLAG_IN_IDLE_MODE) {
				sdHwChipDeselect();
				return SdCmdInternalError;
			}
			break;
		
		default:
			sdHwChipDeselect();
			return SdCmdInternalError;
	}
	
	if (dataDir == SdHwDataNone)
		sdHwChipDeselect();
		
	return SdHwCmdResultOK;
}

static bool sdHwPrvDataWait(void)
{
	uint_fast16_t tries, timeoutBytes = mTimeoutBytes;
	uint_fast8_t byte;
	uint64_t time, rt;
	
	for (tries = 0; tries < timeoutBytes; tries++) {
		
		byte = sdSpiByte(0xFF);
		
		if (!(byte & 0xf0))
			return false;
		
		if (byte == 0xfe)
			return true;
	}
	
	time = getTime();
	do {
		byte = sdSpiByte(0xFF);
		
		if (!(byte & 0xf0))
			return false;
		
		if (byte == 0xfe)
			return true;
	
	} while ((rt = getTime()) - time < mRdTimeoutTicks);
	
	pr("read timeout. waited %u ticks, %lu msec. max was %u ticks\n",
		(uint32_t)(rt - time), (uint32_t)(((getTime() - time) * 1000) / TICKS_PER_SECOND), mRdTimeoutTicks);
	
	return false;
}

bool sdHwReadData(uint8_t* data, uint_fast16_t sz)	//length must be even, pointer must be halfword aligned
{
	if (!sdHwPrvDataWait())
		return false;
		
	if (sz != SD_BLOCK_SIZE || ((uintptr_t)data)  & 3) {	//slow case
		
		while (sz--)
			*data++ = sdSpiByte(0xFF);
		sdSpiByte(0xFF);
		sdSpiByte(0xFF);
	}
	else {								//fast case

		mTrash = 0xffffffff;
		
		spi1_hw->cr1 &=~ SPI_SSPCR1_SSE_BITS;
		spi1_hw->cr0 = (spi1_hw->cr0 &~ SPI_SSPCR0_DSS_BITS) |  ((16 - 1) << SPI_SSPCR0_DSS_LSB);
		spi1_hw->cr1 |= SPI_SSPCR1_SSE_BITS;
	
		spi1_hw->dmacr |= SPI_SSPDMACR_TXDMAE_BITS | SPI_SSPDMACR_RXDMAE_BITS;
		
		dma_hw->ch[SD_DMA_CH_READ].al2_write_addr_trig = (uintptr_t)data;
		dma_hw->ch[SD_DMA_CH_WRITE].al3_read_addr_trig = (uintptr_t)&mTrash;
		
		//wait
		while (dma_hw->ch[SD_DMA_CH_WRITE].al1_ctrl & DMA_CH3_CTRL_TRIG_BUSY_BITS);
		while (dma_hw->ch[SD_DMA_CH_READ].al1_ctrl & DMA_CH2_CTRL_TRIG_BUSY_BITS);
		
		spi1_hw->cr1 &=~ SPI_SSPCR1_SSE_BITS;
		spi1_hw->cr0 = (spi1_hw->cr0 &~ SPI_SSPCR0_DSS_BITS) |  ((8 - 1) << SPI_SSPCR0_DSS_LSB);
		spi1_hw->cr1 |= SPI_SSPCR1_SSE_BITS;
	
		
		spi1_hw->dmacr &=~ (SPI_SSPDMACR_TXDMAE_BITS | SPI_SSPDMACR_RXDMAE_BITS);
		
		//eat the crc
		sdSpiByte(0xFF);
		sdSpiByte(0xFF);
	
		asm volatile("":::"memory");
	}
	
	return true;
}

enum SdHwWriteReply sdHwWriteData(const uint8_t *data, uint_fast16_t sz, bool isMultiblock)
{
	uint_fast16_t tries, timeoutBytes = mTimeoutBytes;
	uint_fast8_t byte;
	uint64_t time;
	
	sdSpiByte(isMultiblock ? 0xFC : 0xFE);	//start block
	while (sz--)
		sdSpiByte(*data++);
	//crc
	sdSpiByte(0xff);
	sdSpiByte(0xff);
	
	//wait for a reply
	
	for (tries = 0; tries < timeoutBytes; tries++) {
		
		byte = sdSpiByte(0xFF);
		
		if ((byte & 0x11) == 0x01) {
		
			switch (byte & 0x1f) {
				case 0x05:
					return SdHwWriteAccepted;
				
				case 0x0b:
					return SdHwWriteCrcErr;
				
				case 0x0d:
					return SdHwWriteError;
				
				default:
					return SdHwCommErr;
			}
		}
	}
	
	time = getTime();
	do {
		byte = sdSpiByte(0xFF);
		
		if ((byte & 0x11) == 0x01) {
		
			switch (byte & 0x1f) {
				case 0x05:
					return SdHwWriteAccepted;
				
				case 0x0b:
					return SdHwWriteCrcErr;
				
				case 0x0d:
					return SdHwWriteError;
				
				default:
					return SdHwCommErr;
			}
		}
	
	} while (getTime() - time < mWrTimeoutTicks);
	
	return SdHwTimeout;
}

bool sdHwPrgBusyWait(void)
{
	uint_fast16_t tries, timeoutBytes = mTimeoutBytes;
	uint32_t timeoutTicks = mWrTimeoutTicks;
	uint64_t time;
	
	for (tries = 0; tries < timeoutBytes; tries++) {
		
		if (sdSpiByte(0xFF) == 0xff)
			return true;
	}
	
	time = getTime();
	do {
		if (sdSpiByte(0xFF) == 0xff)
			return true;
	
	} while (getTime() - time < timeoutTicks);
	
	return false;
}

void sdHwRxRawBytes(void *dstP /* can be NULL*/, uint_fast16_t numBytes)
{
	uint8_t *dst = (uint8_t*)dstP;
	
	while (numBytes--) {
		
		uint_fast8_t val = sdSpiByte(0xff);
		
		if (dst)
			*dst++ = val;
	}
}

bool sdHwMultiBlockWriteSignalEnd(void)
{
	//stoptran token
	(void)sdSpiByte(0xFD);
	
	return true;
}

bool sdHwMultiBlockReadSignalEnd(void)
{
	//nothing
	
	return true;
}


