#include "hardware/regs/dreq.h"
#include "pinoutRp2350defcon.h"
#include "pioI2C.h"
#include "printf.h"
#include "2350.h"


#define PIO_Nth2(x)							pio ## x ## _hw
#define PIO_Nth(x)							PIO_Nth2(x)
#define FUNCSEL_Nth2(x)						IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_PIO ## x ## _0
#define FUNCSEL_Nth(x)						FUNCSEL_Nth2(x)
#define DREQ_PIO_TYPE_IDX2(p, t, i)			DREQ_PIO ## p ## _ ## t ## i
#define DREQ_PIO_TYPE_IDX(p, t, i)			DREQ_PIO_TYPE_IDX2(p, t, i)
#define MY_PIO								PIO_Nth(I2C_PIO_IDX)
#define MY_FUNCSEL							FUNCSEL_Nth(I2C_PIO_IDX)
#define IRQH_Nthx(n, m)						PIO ## n ## _IRQ_ ## m ## _IRQHandler
#define IRQH_Nth(n, m)						IRQH_Nthx(n, m)
#define I2C_PIO_IRQH						IRQH_Nth(I2C_PIO_IDX, I2C_PIO_EXT_IRQ_IDX)
#define IRQN_Nthx(n, m)						PIO ## n ## _IRQ_ ## m ## _IRQn
#define IRQN_Nth(n, m)						IRQN_Nthx(n, m)
#define I2C_PIO_IRQN						IRQN_Nth(I2C_PIO_IDX, I2C_PIO_EXT_IRQ_IDX)
#define I2C_INTE_REG_Nthx(n)				inte ## n
#define I2C_INTE_REG_Nth(n)					I2C_INTE_REG_Nthx(n)
#define I2C_INTE_REG						MY_PIO->I2C_INTE_REG_Nth(I2C_PIO_EXT_IRQ_IDX)




#define SIDE_SET_HAS_ENABLE_BIT				1
#define SIDE_SET_NUM_BITS					1
#define DEFINE_PIO_INSTRS
#include "pioAsm.h"


#define DMA_I2C_0							(I2C_DMA_FIRST + 0)
#define DMA_I2C_1							(I2C_DMA_FIRST + 1)

static uint8_t mEndPc, mStartPc, mStopPc, mSend1bitPc, mSend8bitsPc, mRecv1bitPc, mRecv8bitsPc, mRecv8bitsPc;
static volatile uint8_t mExtI2cCmds[128], mExtI2cReplies[64];
static I2CtransactionOverCbkF mTransactOverCbkF;
static const struct I2Creq *mCurI2cReq;
static volatile bool mExtI2cBusy;
static void *mTransactOverCbkD;


#define VERBOSE									0



static void i2cClockingInit(void)
{
	uint32_t fBase = TICKS_PER_SECOND / 26000, fClk = 400, fDiv = (fBase + fClk - 1) / fClk, fClkEff = fBase / fDiv;

	pr("picking I2C div %u for a clock of %u KHz\n", fDiv, fClkEff);

	MY_PIO->sm[I2C_PIO_SM].clkdiv = (fDiv << PIO_SM0_CLKDIV_INT_LSB);
}

static void i2cPinsInit(void)
{
	padsbank0_hw->io[PIN_SDA] = PADS_BANK0_GPIO0_DRIVE_VALUE_2MA | PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_PUE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS;
	padsbank0_hw->io[PIN_SCL] = PADS_BANK0_GPIO0_DRIVE_VALUE_2MA | PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_PUE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS;

	iobank0_hw->io[PIN_SDA].ctrl = (iobank0_hw->io[PIN_SDA].ctrl &~ IO_BANK0_GPIO0_CTRL_FUNCSEL_BITS) | (MY_FUNCSEL << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB);
	iobank0_hw->io[PIN_SCL].ctrl = (iobank0_hw->io[PIN_SCL].ctrl &~ IO_BANK0_GPIO0_CTRL_FUNCSEL_BITS) | (MY_FUNCSEL << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB);
}

static void i2cSmInit(void)
{
	uint_fast8_t pc = I2C_PIO_FIRST_USED_PC, lblTmp;

	MY_PIO->ctrl &=~ ((1 << PIO_CTRL_SM_ENABLE_LSB) << I2C_PIO_SM);
	MY_PIO->ctrl |= (1 << PIO_CTRL_SM_RESTART_LSB) << I2C_PIO_SM;

	//reset fifos for real
	MY_PIO->sm[I2C_PIO_SM].shiftctrl |= PIO_SM0_SHIFTCTRL_FJOIN_TX_BITS;
	MY_PIO->sm[I2C_PIO_SM].shiftctrl &=~ PIO_SM0_SHIFTCTRL_FJOIN_TX_BITS;
	MY_PIO->sm[I2C_PIO_SM].shiftctrl |= PIO_SM0_SHIFTCTRL_FJOIN_RX_BITS;
	MY_PIO->sm[I2C_PIO_SM].shiftctrl &=~ PIO_SM0_SHIFTCTRL_FJOIN_RX_BITS;

	mStopPc = pc;
	MY_PIO->instr_mem[pc++] = I_SET(7, 0, SET_DST_PINDIRS, 1);
	MY_PIO->instr_mem[pc++] = I_MOV(7, 2, MOV_DST_X, MOV_OP_COPY, MOV_SRC_X);
	MY_PIO->instr_mem[pc++] = I_SET(7, 0, SET_DST_PINDIRS, 0);
	MY_PIO->instr_mem[pc++] = I_IRQ(0, 0, 0, 1, I2C_PIO_INT_IRQ_IDX);			//stop causes an irq we can detect
	mEndPc = pc;
	MY_PIO->instr_mem[pc++] = I_OUT(0, 0, MOV_DST_Y, 3);	//top 3 bits go to Y
	MY_PIO->instr_mem[pc++] = I_OUT(0, 0, MOV_DST_PC, 5);	//bottom 5 go to PC

	mStartPc = pc;
	MY_PIO->instr_mem[pc++] = I_MOV(7, 2, MOV_DST_X, MOV_OP_COPY, MOV_SRC_X);
	MY_PIO->instr_mem[pc++] = I_SET(7, 0, SET_DST_PINDIRS, 1);
	MY_PIO->instr_mem[pc++] = I_JMP(7, 3, JMP_ALWAYS, mEndPc);

	mSend1bitPc = pc;		//the value of 0 in "Y" is a requirement
	MY_PIO->instr_mem[pc++] = I_OUT(7, 0, OUT_DST_NULL, 7);
	//fallthrough

	mSend8bitsPc = pc;		//the value of 7 in "Y" is a requirement
	lblTmp = pc;
	MY_PIO->instr_mem[pc++] = I_OUT(7, 0, OUT_DST_PINDIRS, 1);
	MY_PIO->instr_mem[pc++] = I_MOV(7, 2, MOV_DST_X, MOV_OP_COPY, MOV_SRC_X);
	MY_PIO->instr_mem[pc++] = I_MOV(7, 3, MOV_DST_X, MOV_OP_COPY, MOV_SRC_X);
	MY_PIO->instr_mem[pc++] = I_JMP(0, 0, JMP_Y_POSTDEC, lblTmp);
	MY_PIO->instr_mem[pc++] = I_JMP(0, 0, JMP_ALWAYS, mEndPc);

	mRecv1bitPc = pc;		//the value of 0 in "Y" is a requirement
	MY_PIO->instr_mem[pc++] = I_IN(0, 0, IN_SRC_ZEROES, 7);
	//fallthrough

	mRecv8bitsPc = pc;		//the value of 7 in "Y" is a requirement
	MY_PIO->instr_mem[pc++] = I_SET(7, 0, SET_DST_PINDIRS, 0);
	lblTmp = pc;
	MY_PIO->instr_mem[pc++] = I_MOV(7, 2, MOV_DST_X, MOV_OP_COPY, MOV_SRC_X);
	MY_PIO->instr_mem[pc++] = I_IN(7, 0, IN_SRC_PINS, 1);
	MY_PIO->instr_mem[pc++] = I_MOV(7, 3, MOV_DST_X, MOV_OP_COPY, MOV_SRC_X);
	MY_PIO->instr_mem[pc++] = I_JMP(0, 0, JMP_Y_POSTDEC, lblTmp);
	MY_PIO->instr_mem[pc++] = I_JMP(0, 0, JMP_ALWAYS, mEndPc);

	//set directions and out values for both pins
	MY_PIO->sm[I2C_PIO_SM].pinctrl = (1 << PIO_SM0_PINCTRL_SET_COUNT_LSB) | (PIN_SDA << PIO_SM0_PINCTRL_SET_BASE_LSB);
	MY_PIO->sm[I2C_PIO_SM].instr = I_SET(0, 0, SET_DST_PINDIRS, 0);
	MY_PIO->sm[I2C_PIO_SM].instr = I_SET(0, 0, SET_DST_PINS, 0);
	MY_PIO->sm[I2C_PIO_SM].pinctrl = (1 << PIO_SM0_PINCTRL_SET_COUNT_LSB) | (PIN_SCL << PIO_SM0_PINCTRL_SET_BASE_LSB);
	MY_PIO->sm[I2C_PIO_SM].instr = I_SET(0, 0, SET_DST_PINDIRS, 0);
	MY_PIO->sm[I2C_PIO_SM].instr = I_SET(0, 0, SET_DST_PINS, 0);

	//config the machine
	MY_PIO->sm[I2C_PIO_SM].execctrl = (MY_PIO->sm[I2C_PIO_SM].execctrl &~ PIO_SM0_EXECCTRL_SIDE_EN_BITS) | PIO_SM0_EXECCTRL_SIDE_PINDIR_BITS | (SIDE_SET_HAS_ENABLE_BIT ? PIO_SM0_EXECCTRL_SIDE_EN_BITS : 0);
	MY_PIO->sm[I2C_PIO_SM].shiftctrl = (MY_PIO->sm[I2C_PIO_SM].shiftctrl &~ (PIO_SM0_SHIFTCTRL_PULL_THRESH_BITS | PIO_SM0_SHIFTCTRL_PUSH_THRESH_BITS | PIO_SM0_SHIFTCTRL_IN_SHIFTDIR_BITS | PIO_SM0_SHIFTCTRL_OUT_SHIFTDIR_BITS)) | PIO_SM0_SHIFTCTRL_AUTOPULL_BITS | PIO_SM0_SHIFTCTRL_AUTOPUSH_BITS | (8 << PIO_SM0_SHIFTCTRL_PULL_THRESH_LSB) | (8 << PIO_SM0_SHIFTCTRL_PUSH_THRESH_LSB);
	MY_PIO->sm[I2C_PIO_SM].pinctrl =
		(SIDE_SET_BITS_USED << PIO_SM0_PINCTRL_SIDESET_COUNT_LSB) | (PIN_SCL << PIO_SM0_PINCTRL_SIDESET_BASE_LSB) |
		(1 << PIO_SM0_PINCTRL_OUT_COUNT_LSB) | (PIN_SDA << PIO_SM0_PINCTRL_OUT_BASE_LSB) |
		(1 << PIO_SM0_PINCTRL_SET_COUNT_LSB) | (PIN_SDA << PIO_SM0_PINCTRL_SET_BASE_LSB) |
		(PIN_SDA << PIO_SM0_PINCTRL_IN_BASE_LSB);

	//start it in inifinite loop state
	MY_PIO->sm[I2C_PIO_SM].instr = I_JMP(0, 0, JMP_ALWAYS, mEndPc);
	MY_PIO->ctrl |= (1 << PIO_CTRL_SM_ENABLE_LSB) << I2C_PIO_SM;

	pr("i2c: using %u instrs, next free is %u\n", pc - I2C_PIO_FIRST_USED_PC, pc);

	dma_hw->ch[DMA_I2C_0].write_addr = (uintptr_t)&MY_PIO->txf[I2C_PIO_SM];
	dma_hw->ch[DMA_I2C_0].al1_ctrl  = (DREQ_PIO_TYPE_IDX(I2C_PIO_IDX, TX, I2C_PIO_SM) << DMA_CH0_CTRL_TRIG_TREQ_SEL_LSB) | (DMA_I2C_0 << DMA_CH0_CTRL_TRIG_CHAIN_TO_LSB) | (DMA_CH0_CTRL_TRIG_DATA_SIZE_VALUE_SIZE_BYTE << DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB) | DMA_CH0_CTRL_TRIG_INCR_READ_BITS | DMA_CH0_CTRL_TRIG_EN_BITS;

	dma_hw->ch[DMA_I2C_1].read_addr = (uintptr_t)&MY_PIO->rxf[I2C_PIO_SM];
	dma_hw->ch[DMA_I2C_1].al1_ctrl = (DREQ_PIO_TYPE_IDX(I2C_PIO_IDX, RX, I2C_PIO_SM) << DMA_CH0_CTRL_TRIG_TREQ_SEL_LSB) | (DMA_I2C_1 << DMA_CH0_CTRL_TRIG_CHAIN_TO_LSB) | (DMA_CH0_CTRL_TRIG_DATA_SIZE_VALUE_SIZE_BYTE << DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB) | DMA_CH0_CTRL_TRIG_INCR_WRITE_BITS | DMA_CH0_CTRL_TRIG_EN_BITS;
	I2C_INTE_REG |= PIO_IRQ0_INTE_SM0_BITS << I2C_PIO_INT_IRQ_IDX;
	NVIC_ClearPendingIRQ(I2C_PIO_IRQN);
	NVIC_EnableIRQ(I2C_PIO_IRQN);
}


void i2cInit(void)
{
	i2cClockingInit();
	i2cPinsInit();
	i2cSmInit();
}

static void i2cPrvStartTransaction(uint32_t cmdsLen, uint32_t repliesLen)
{
	asm volatile("":::"memory");
	dma_hw->ch[DMA_I2C_1].write_addr = (uintptr_t)mExtI2cReplies;
	dma_hw->ch[DMA_I2C_0].read_addr = (uintptr_t)mExtI2cCmds;
	dma_hw->ch[DMA_I2C_1].al1_transfer_count_trig =  repliesLen;
	dma_hw->ch[DMA_I2C_0].al1_transfer_count_trig = cmdsLen;
}

static bool i2cPrvAddByte(volatile uint8_t *buf, uint32_t bufSz, uint32_t *bufPosP, uint8_t val)
{
	if (*bufPosP >= bufSz)
		return false;

	buf[(*bufPosP)++] = val;
	return true;
}

bool i2cTransact(const struct I2Creq *req, I2CtransactionOverCbkF cbkF, void *cbkD)
{
	const struct I2Creq *reqOrig = req;
	uint32_t i, posTx = 0, posRx = 0;

	//cannot do anything if busy
	if (mExtI2cBusy)
		return false;
	mExtI2cBusy = true;

	//reset state just in case
	MY_PIO->sm[I2C_PIO_SM].instr = I_JMP(0, 0, JMP_ALWAYS, mEndPc);

	do {

		if (req->txData) {

			//start
			if (!i2cPrvAddByte(mExtI2cCmds, sizeof(mExtI2cCmds), &posTx, mStartPc))
				goto fail;
			
			//address + nW, expect ACK
			if (!i2cPrvAddByte(mExtI2cCmds, sizeof(mExtI2cCmds), &posTx, 32 * 7 + mSend8bitsPc))
				goto fail;
			if (!i2cPrvAddByte(mExtI2cCmds, sizeof(mExtI2cCmds), &posTx, (uint8_t)~(req->addr * 2 + 0)))
				goto fail;
			if (!i2cPrvAddByte(mExtI2cCmds, sizeof(mExtI2cCmds), &posTx, mRecv1bitPc))
				goto fail;
			if (!i2cPrvAddByte(mExtI2cReplies, sizeof(mExtI2cReplies), &posRx, 0xff))
				goto fail;
			
			for (i = 0; i < req->txLen; i++) {	//bytes and their ACKs
				if (!i2cPrvAddByte(mExtI2cCmds, sizeof(mExtI2cCmds), &posTx, 32 * 7 + mSend8bitsPc))
					goto fail;
				if (!i2cPrvAddByte(mExtI2cCmds, sizeof(mExtI2cCmds), &posTx, (uint8_t)~req->txData[i]))
					goto fail;
				if (!i2cPrvAddByte(mExtI2cCmds, sizeof(mExtI2cCmds), &posTx, mRecv1bitPc))
					goto fail;
				if (!i2cPrvAddByte(mExtI2cReplies, sizeof(mExtI2cReplies), &posRx, 0xff))
					goto fail;
			}
		}

		if (req->rxData) {
			//[re-]start
			if (!i2cPrvAddByte(mExtI2cCmds, sizeof(mExtI2cCmds), &posTx, mStartPc))
				goto fail;
			
			//address + R, expect ACK
			if (!i2cPrvAddByte(mExtI2cCmds, sizeof(mExtI2cCmds), &posTx, 32 * 7 + mSend8bitsPc))
				goto fail;
			if (!i2cPrvAddByte(mExtI2cCmds, sizeof(mExtI2cCmds), &posTx, (uint8_t)~(req->addr * 2 + 1)))
				goto fail;
			if (!i2cPrvAddByte(mExtI2cCmds, sizeof(mExtI2cCmds), &posTx, mRecv1bitPc))
				goto fail;
			if (!i2cPrvAddByte(mExtI2cReplies, sizeof(mExtI2cReplies), &posRx, 0xff))
				goto fail;
			
			for (i = 0; i < req->rxLen; i++) {	//bytes and their ACKs
				if (!i2cPrvAddByte(mExtI2cCmds, sizeof(mExtI2cCmds), &posTx, 32 * 7 + mRecv8bitsPc))
					goto fail;
				if (!i2cPrvAddByte(mExtI2cCmds, sizeof(mExtI2cCmds), &posTx, mSend1bitPc))
					goto fail;
				if (!i2cPrvAddByte(mExtI2cCmds, sizeof(mExtI2cCmds), &posTx, req->rxAcks ? req->rxAcks[i] : (i == req->rxLen - 1 ? 0 : 1)))
					goto fail;
				if (!i2cPrvAddByte(mExtI2cReplies, sizeof(mExtI2cReplies), &posRx, 0xff))
					goto fail;
			}
		}
		req++;
	} while (req[-1].haveNext);

	//stop
	if (!i2cPrvAddByte(mExtI2cCmds, sizeof(mExtI2cCmds), &posTx, mStopPc))
		goto fail;
	
	if (VERBOSE)
		pr("i2c ext: used %u tx slots, %u rx slots\n", posTx, posRx);

	mTransactOverCbkF = cbkF;
	mTransactOverCbkD = cbkD;

	//go
	mCurI2cReq = reqOrig;
	i2cPrvStartTransaction(posTx, posRx);
	return true;

fail:
	mExtI2cBusy = false;
	return false;
}

static void i2cPrvProcessReply(void)	//necessarily the same logic as dispI2cTransact()
{
	const struct I2Creq *req = mCurI2cReq, *reqForCbk = req;
	uint32_t i, posTx = 0, posRx = 0;
	bool success = true;

	do {
		uint8_t *txAcks = req->txAcks;
		uint8_t *rxData = req->rxData;

		if (VERBOSE)
			pr("i2c process reply for req %u\n", req - mCurI2cReq);

		if (req->txData) {

			if (VERBOSE)
				pr(" -> tx (acks at 0x%08x)\n", txAcks, &req->txAcks);

			//start
			posTx++;
			
			//address + nW, expect ACK
			posTx++;	//"send byte"
			posTx++;	//addr byte
			posTx++;	//"recv ack"
			if (VERBOSE)
				pr(" -> ADDR ACK = %d\n", mExtI2cReplies[posRx]);
			if (mExtI2cReplies[posRx])	//expect ACK
				success = false;
			if (txAcks)
				*txAcks++ = mExtI2cReplies[posRx];
			posRx++;
			
			for (i = 0; i < req->txLen; i++) {	//bytes and their ACKs
				posTx++;	//"send byte"
				posTx++;	//the byte
				posTx++;	//"recv ack"

				if (VERBOSE)
					pr(" -> DATA[%u] ACK = %d\n", i, mExtI2cReplies[posRx]);

				if (i != req->txLen - 1 && mExtI2cReplies[posRx])	//expect ACK, excpt on last byte which may be NAKed
					success = false;
				if (txAcks)
					*txAcks++ = mExtI2cReplies[posRx];
				posRx++;
			}
		}

		if (req->rxData) {
			if (VERBOSE)
				pr(" -> rx\n");

			//start
			posTx++;
			
			//address + R, expect ACK
			posTx++;	//"send byte"
			posTx++;	//addr byte
			posTx++;	//"recv ack"
			if (VERBOSE)
				pr(" -> ADDR ACK = %d\n", mExtI2cReplies[posRx]);
			if (mExtI2cReplies[posRx])	//expect ACK
				success = false;
			if (req->rxAddrAck)
				*req->rxAddrAck = mExtI2cReplies[posRx];
			posRx++;
			
			for (i = 0; i < req->rxLen; i++) {	//bytes and their ACKs
				posTx++;	//"recv byte"
				posTx++;	//"send ACK bit"
				posTx++;	//ack bit
				if (VERBOSE)
					pr(" -> DATA[%u] = 0x%02x  (at rx buffer idx %u)\n", i, mExtI2cReplies[posRx], posRx);
				*rxData++ = mExtI2cReplies[posRx];
				posRx++;
			}
		}
		req++;
	} while (req[-1].haveNext);
	//stop
	posTx++;

	mCurI2cReq = NULL;		//if touch is onging, it'll process this and NULL out the pointer and call callback when done
	mExtI2cBusy = false;

	if (VERBOSE)
		pr("-> done\n");

	if (mTransactOverCbkF)
		mTransactOverCbkF(mTransactOverCbkD, reqForCbk, success);
}

void __attribute__((used)) I2C_PIO_IRQH(void)	//i2c done
{
	MY_PIO->irq = (1 << PIO_IRQ_LSB) << I2C_PIO_INT_IRQ_IDX;
	i2cPrvProcessReply();
}

bool i2cPrvIsBuysy(void)
{
	return mExtI2cBusy;
}



//util funcs
static void i2cSimpleTransOver(void *userData, const struct I2Creq *req, bool likelySuccess)
{
	volatile bool *results = (volatile bool*)userData;

	//order matters
	results[1] = likelySuccess;
	results[0] = true;	//done
}

bool i2cSimpleWrite(uint_fast8_t sevenBitAddr, const uint8_t *vals, uint32_t numBytes)
{
	const struct I2Creq mWriteReq = {
		.haveNext = false,
		.addr = sevenBitAddr,
		.txData = vals,
		.txAcks = NULL,
		.txLen = numBytes,
		.rxAddrAck = NULL,
		.rxData = NULL,
		.rxAcks = NULL,
		.rxLen = 0,
	};
	volatile bool results[2] = {false};	//{done, success}
	
	if (!i2cTransact(&mWriteReq, i2cSimpleTransOver, (void*)results))
		return false;

	while (!results[0])
		asm volatile("":::"memory");		//data changed
	
	return results[1];
}

bool i2cSimpleRead(uint_fast8_t sevenBitAddr, uint8_t *vals, uint32_t numBytes)
{
	const struct I2Creq mWriteReq = {
		.haveNext = false,
		.addr = sevenBitAddr,
		.txData = NULL,
		.txAcks = NULL,
		.txLen = 0,
		.rxAddrAck = NULL,
		.rxData = vals,
		.rxAcks = NULL,
		.rxLen = numBytes,
	};
	volatile bool results[2] = {false};	//{done, success}
	
	if (!i2cTransact(&mWriteReq, i2cSimpleTransOver, (void*)results))
		return false;

	while (!results[0])
		asm volatile("":::"memory");		//data changed
	
	return results[1];
}

bool i2cRegRead(uint_fast8_t sevenBitAddr, uint8_t reg, uint8_t *vals, uint32_t numBytes)
{
	const struct I2Creq mWriteReq = {
		.haveNext = false,
		.addr = sevenBitAddr,
		.txData = &reg,
		.txAcks = NULL,
		.txLen = 1,
		.rxAddrAck = NULL,
		.rxData = vals,
		.rxAcks = NULL,
		.rxLen = numBytes,
	};
	bool results[2] = {false};	//{done, success}
	
	if (!i2cTransact(&mWriteReq, i2cSimpleTransOver, (void*)results))
		return false;

	while (!results[0])
		asm volatile("":::"memory");		//data changed
	
	return results[1];
}

bool i2cOneByteRegWrite(uint_fast8_t sevenBitAddr, uint8_t reg, uint8_t val)
{
	uint8_t bytes[] = {reg, val};

	return i2cSimpleWrite(sevenBitAddr, bytes, sizeof(bytes));
}



