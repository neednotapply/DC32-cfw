#include "pinoutRp2350defcon.h"
#include "pioIrdaSIR.h"
#include "printf.h"
#include "2350.h"


#define PIO_Nth2(x)							pio ## x ## _hw
#define PIO_Nth(x)							PIO_Nth2(x)
#define MY_PIO								PIO_Nth(SIR_PIO_IDX)

#define IRQH_Nthx(n, m)						PIO ## n ## _IRQ_ ## m ## _IRQHandler
#define IRQH_Nth(n, m)						IRQH_Nthx(n, m)
#define SIR_PIO_IRQH						IRQH_Nth(SIR_PIO_IDX, SIR_PIO_EXT_IRQ_IDX)
#define IRQN_Nthx(n, m)						PIO ## n ## _IRQ_ ## m ## _IRQn
#define IRQN_Nth(n, m)						IRQN_Nthx(n, m)
#define SIR_PIO_IRQN						IRQN_Nth(SIR_PIO_IDX, SIR_PIO_EXT_IRQ_IDX)
#define SIR_INTE_REG_Nthx(n)				inte ## n
#define SIR_INTE_REG_Nth(n)					SIR_INTE_REG_Nthx(n)
#define SIR_INTE_REG						MY_PIO->SIR_INTE_REG_Nth(SIR_PIO_EXT_IRQ_IDX)




#define SIDE_SET_HAS_ENABLE_BIT 	0
#define SIDE_SET_NUM_BITS			0
#define DEFINE_PIO_INSTRS
#include "pioAsm.h"
#undef DEFINE_PIO_INSTRS

#define RX_MACHINERY_CLOCK				15000000		//fast enough for high rates, slow enough that in-pio-instr delays are enough for 1us


#define CIRC_BUF_SZ						64


static uint_fast8_t (*mParityFunc)(uint_fast8_t val);
static volatile uint16_t mCircBuf[CIRC_BUF_SZ];
static volatile uint8_t mCircBufW, mCircBufR;	//equals means empty
static uint8_t mDataBits, mStopBits;
static IrdaSIRuartRxF mIrRxF;
static volatile bool mCurTx, mCurRx;
static void *mIrRxD;


static bool pioIrdaSirPrvCircBufIsFull(void)
{
	uint8_t mCircBufWnext = (mCircBufW + 1 == CIRC_BUF_SZ ? 0 : mCircBufW + 1);
	
	return mCircBufWnext != mCircBufR;
}

static bool pioIrdaSirPrvCircBufAdd(uint_fast16_t val)
{
	uint_fast8_t cirBufWnow = mCircBufW, circBufWnext = (cirBufWnow + 1 == CIRC_BUF_SZ ? 0 : cirBufWnow + 1);
	
	if (circBufWnext == mCircBufR)
		return false;
	
	mCircBuf[cirBufWnow] = val;
	mCircBufW = circBufWnext;

	return true;
}

static int32_t pioIrdaSirPrvCircBufGet(void)
{
	uint_fast8_t cirBufRnow = mCircBufR;
	uint_fast16_t ret;
	
	if (cirBufRnow == mCircBufW)
		return -1;
	
	ret = mCircBuf[cirBufRnow];
	mCircBufR = (cirBufRnow + 1 == CIRC_BUF_SZ ? 0 : cirBufRnow + 1);
	
	return (uint32_t)ret;
}

static void pioIrdaSirPrvUnsetup(void)
{
	NVIC_DisableIRQ(SIR_PIO_IRQN);
	
	//stop SM
	MY_PIO->ctrl &=~ ((1 << PIO_CTRL_SM_ENABLE_LSB) << SIR_PIO_SM);
	
	//reset SM
	MY_PIO->ctrl |= ((1 << PIO_CTRL_SM_RESTART_LSB) << SIR_PIO_SM);
	
	//wait
	while (MY_PIO->ctrl & ((1 << PIO_CTRL_SM_RESTART_LSB) << SIR_PIO_SM))

	//nop
	MY_PIO->sm[SIR_PIO_SM].instr = I_MOV(0, 0, MOV_DST_X, MOV_OP_COPY, MOV_SRC_X);

	//clear fifos
	MY_PIO->sm[SIR_PIO_SM].shiftctrl = PIO_SM0_SHIFTCTRL_FJOIN_RX_BITS;
	MY_PIO->sm[SIR_PIO_SM].shiftctrl = PIO_SM0_SHIFTCTRL_FJOIN_TX_BITS;
	MY_PIO->sm[SIR_PIO_SM].shiftctrl = 0;
	
	//clear buffer
	mCircBufW = 0;
	mCircBufR = 0;
	
	//clear state
	mCurTx = false;
	mCurRx = false;
	
	SIR_INTE_REG &=~ ((PIO_IRQ0_INTE_SM0_TXNFULL_BITS | PIO_IRQ0_INTE_SM0_RXNEMPTY_BITS) << SIR_PIO_SM);
	NVIC_ClearPendingIRQ(SIR_PIO_IRQN);
}

static void pioIrdaSirPrvSetupTx(uint32_t baudrate, uint_fast8_t dataBits, uint_fast8_t parityBits, uint_fast8_t stopBits)
{
	uint_fast8_t pc = SIR_PIO_FIRST_USED_PC, startPC, restartPC, endPC, jmpDest;
	
	
	//TX operates in non-low-power mode and needs a clock of 16x bandwidth
	//data needs to be inserted precisely as sent, eg for 8n1, insert (0x01 + ~data << 1) where 1 is the start bit, total bitlength should be programmed into PIO, shift right
	//code is more complex than you'd think to allow a SURE determination when TX is done (by examining pc)
	
	startPC = restartPC = pc;
	MY_PIO->instr_mem[pc++] = I_PULL(0, 0, 0, 1);
	MY_PIO->instr_mem[pc++] = I_SET(0, 0, SET_DST_X, dataBits + parityBits + stopBits + 1 /* start bit */ - 1);
	
	jmpDest = pc;
	MY_PIO->instr_mem[pc++] = I_OUT(2, 0, OUT_DST_PINS, 1);
	MY_PIO->instr_mem[pc++] = I_SET(11, 0, SET_DST_PINS, 0);

	MY_PIO->instr_mem[pc++] = I_JMP(0, 0, JMP_X_POSTDEC, jmpDest);
	endPC = pc - 1;
	
	pr("irda: tx: using %u instrs, next free is %u\n", pc - SIR_PIO_FIRST_USED_PC, pc);

	//configure sm0
	MY_PIO->sm[SIR_PIO_SM].clkdiv = ((TICKS_PER_SECOND / 16 * 256ull + baudrate / 2) / baudrate) << PIO_SM0_CLKDIV_FRAC_LSB;
	MY_PIO->ctrl |= ((1 << PIO_CTRL_CLKDIV_RESTART_LSB) << SIR_PIO_SM);
	MY_PIO->sm[SIR_PIO_SM].execctrl = (MY_PIO->sm[SIR_PIO_SM].execctrl &~ (PIO_SM0_EXECCTRL_WRAP_TOP_BITS | PIO_SM0_EXECCTRL_WRAP_BOTTOM_BITS | PIO_SM2_EXECCTRL_SIDE_EN_BITS)) |(endPC << PIO_SM0_EXECCTRL_WRAP_TOP_LSB) | (restartPC << PIO_SM0_EXECCTRL_WRAP_BOTTOM_LSB) | (SIDE_SET_HAS_ENABLE_BIT ? PIO_SM2_EXECCTRL_SIDE_EN_BITS : 0);
	MY_PIO->sm[SIR_PIO_SM].shiftctrl = (MY_PIO->sm[SIR_PIO_SM].shiftctrl &~ (PIO_SM1_SHIFTCTRL_PULL_THRESH_BITS | PIO_SM1_SHIFTCTRL_PUSH_THRESH_BITS | PIO_SM0_SHIFTCTRL_IN_SHIFTDIR_BITS | PIO_SM0_SHIFTCTRL_AUTOPUSH_BITS | PIO_SM0_SHIFTCTRL_AUTOPULL_BITS)) | PIO_SM0_SHIFTCTRL_OUT_SHIFTDIR_BITS | PIO_SM0_SHIFTCTRL_FJOIN_TX_BITS;
	MY_PIO->sm[SIR_PIO_SM].pinctrl = (SIDE_SET_BITS_USED << PIO_SM1_PINCTRL_SIDESET_COUNT_LSB) | (1 << PIO_SM1_PINCTRL_OUT_COUNT_LSB) | (PIN_IRDA_OUT << PIO_SM1_PINCTRL_OUT_BASE_LSB) | (1 << PIO_SM1_PINCTRL_SET_COUNT_LSB)| (PIN_IRDA_OUT << PIO_SM1_PINCTRL_SET_BASE_LSB);

	//set out direction
	MY_PIO->sm[SIR_PIO_SM].instr = I_SET(0, 0, SET_DST_PINDIRS, 1);
	
	//logi("starting TX SM\n");
	MY_PIO->sm[SIR_PIO_SM].instr = I_JMP(0, 0, JMP_ALWAYS, startPC);
	MY_PIO->ctrl |= ((1 << PIO_CTRL_SM_ENABLE_LSB) << SIR_PIO_SM);
	
	//irq on TX not full, but not enabled since it is empty now and we have no data
	SIR_INTE_REG &=~ ((PIO_IRQ0_INTE_SM0_TXNFULL_BITS | PIO_IRQ0_INTE_SM0_RXNEMPTY_BITS) << SIR_PIO_SM);
	NVIC_ClearPendingIRQ(SIR_PIO_IRQN);
	NVIC_EnableIRQ(SIR_PIO_IRQN);
	SIR_INTE_REG = PIO_IRQ0_INTE_SM0_TXNFULL_BITS << SIR_PIO_SM;
}

static void pioIrdaSirPrvSetupRx(uint32_t baudrate, uint_fast8_t dataBits, uint_fast8_t parityBits, uint_fast8_t stopBits)
{
	uint32_t bitcounterValue = (RX_MACHINERY_CLOCK + baudrate / 2) / baudrate - 3;
	uint_fast8_t pc = SIR_PIO_FIRST_USED_PC, startPC, restartPC, endPC, jmpDest1, jmpDest2;
	
	
	//1.6276 us is the pulse width of 115,200 and of low power IrDA, so we wait for a low, check again in 1.5 us, and if it is still low, we consider this a start bit
	//clock should be ~10MHz, shifter should be to the right, autopush at 32 bits. autopull at 32 bits, input should be an infinite stream of words that represent the
	//inter-bit delay, 10e6 / baudrate - 33
	
	
	restartPC = startPC = pc;
	jmpDest1 = pc;
	MY_PIO->instr_mem[pc++] = I_WAIT(8, 0, 0, WAIT_FOR_GPIO, PIN_IRDA_IN);			//wait for low
	MY_PIO->instr_mem[pc++] = I_JMP(0, 0, JMP_PIN, jmpDest1);							//1us later, if high now, consider it a glitch
		
	MY_PIO->instr_mem[pc++] = I_SET(0, 0, SET_DST_Y, dataBits + stopBits + parityBits - 1);			//num bits per transport unit minus two
	
	jmpDest1 = pc;
	MY_PIO->instr_mem[pc++] = I_MOV(0, 0, MOV_DST_X, MOV_OP_COPY, IN_SRC_OSR);
	
	jmpDest2 = pc;
	MY_PIO->instr_mem[pc++] = I_JMP(0, 0, JMP_X_POSTDEC, jmpDest2);					//delay (wastes "bitcounterValue + 1" cycles)
	
	MY_PIO->instr_mem[pc++] = I_IN(0, 0, IN_SRC_PINS, 1);
	
	MY_PIO->instr_mem[pc++] = I_JMP(0, 0, JMP_Y_POSTDEC, jmpDest1);
	
	MY_PIO->instr_mem[pc++] = I_IN(0, 0, IN_SRC_ZEROES, 32 - dataBits - stopBits - parityBits);
	
	MY_PIO->instr_mem[pc++] = I_WAIT(4, 0, 1, WAIT_FOR_GPIO, PIN_IRDA_IN);				//wait for high
	endPC = pc - 1;
	
	pr("irda: rx: using %u instrs, next free is %u\n", pc - SIR_PIO_FIRST_USED_PC, pc);

	MY_PIO->sm[SIR_PIO_SM].clkdiv = ((TICKS_PER_SECOND * 256ull + RX_MACHINERY_CLOCK / 2) / RX_MACHINERY_CLOCK) << PIO_SM0_CLKDIV_FRAC_LSB;
	MY_PIO->ctrl |= ((1 << PIO_CTRL_CLKDIV_RESTART_LSB) << SIR_PIO_SM);
	MY_PIO->sm[SIR_PIO_SM].execctrl = (MY_PIO->sm[SIR_PIO_SM].execctrl &~ (PIO_SM0_EXECCTRL_WRAP_TOP_BITS | PIO_SM0_EXECCTRL_WRAP_BOTTOM_BITS | PIO_SM2_EXECCTRL_SIDE_EN_BITS)) |(endPC << PIO_SM0_EXECCTRL_WRAP_TOP_LSB) | (restartPC << PIO_SM0_EXECCTRL_WRAP_BOTTOM_LSB) | (SIDE_SET_HAS_ENABLE_BIT ? PIO_SM2_EXECCTRL_SIDE_EN_BITS : 0) | (PIN_IRDA_IN << PIO_SM2_EXECCTRL_JMP_PIN_LSB);
	MY_PIO->sm[SIR_PIO_SM].shiftctrl = (MY_PIO->sm[SIR_PIO_SM].shiftctrl &~ (PIO_SM1_SHIFTCTRL_PULL_THRESH_BITS | PIO_SM1_SHIFTCTRL_PUSH_THRESH_BITS | PIO_SM0_SHIFTCTRL_OUT_SHIFTDIR_BITS | PIO_SM0_SHIFTCTRL_AUTOPULL_BITS)) | PIO_SM0_SHIFTCTRL_IN_SHIFTDIR_BITS | PIO_SM0_SHIFTCTRL_AUTOPUSH_BITS;
	MY_PIO->sm[SIR_PIO_SM].pinctrl = (SIDE_SET_BITS_USED << PIO_SM1_PINCTRL_SIDESET_COUNT_LSB) | (PIN_IRDA_IN << PIO_SM1_PINCTRL_IN_BASE_LSB);
	
	//prepare OSR for SM1
	MY_PIO->txf[SIR_PIO_SM] = bitcounterValue;
	MY_PIO->sm[SIR_PIO_SM].instr = I_PULL(0, 0, 0, 0);
	MY_PIO->sm[SIR_PIO_SM].shiftctrl |= PIO_SM0_SHIFTCTRL_FJOIN_RX_BITS;
	
	//logi("starting RX SM\n");
	MY_PIO->sm[SIR_PIO_SM].instr = I_JMP(0, 0, JMP_ALWAYS, startPC);
	MY_PIO->ctrl |= ((1 << PIO_CTRL_SM_ENABLE_LSB) << SIR_PIO_SM);
	
	//irq on RX not empty
	SIR_INTE_REG &=~ ((PIO_IRQ0_INTE_SM0_TXNFULL_BITS | PIO_IRQ0_INTE_SM0_RXNEMPTY_BITS) << SIR_PIO_SM);
	NVIC_ClearPendingIRQ(SIR_PIO_IRQN);
	NVIC_EnableIRQ(SIR_PIO_IRQN);
	SIR_INTE_REG = PIO_IRQ0_INTE_SM0_RXNEMPTY_BITS << SIR_PIO_SM;
}

static uint_fast16_t pioIrdaSirPrvProcessInput(uint32_t rawVal)		//data is missing start bit, but otherwise is correct
{
	uint_fast16_t ret = 0, stopBitMask, dataValMask = (1 << mDataBits) - 1, dataVal = rawVal & dataValMask;
	uint_fast8_t i;
	
	//pre-calc stop bit mask
	stopBitMask = ((1 << mStopBits) - 1) << mDataBits;
	
	//calc parity
	if (mParityFunc) {
		
		uint_fast16_t corectParity = mParityFunc(dataVal) << mDataBits, parityMask = 1 << mDataBits;
		
		if ((rawVal ^ corectParity) & parityMask)
			ret |= UART_BIT_MASK_PAR_ERR;
		
		stopBitMask <<= 1;	//stop bits come after parity
	}
	
	//verify stop bits
	if ((rawVal & stopBitMask) != stopBitMask)
		ret |= UART_BIT_MASK_FRM_ERR;
	
	//add data val to return value
	ret += dataVal;
	
	return ret;
}

void __attribute__((used)) SIR_PIO_IRQH(void)
{
	if (mCurTx) {	//in tx mode
	
		pr("tx irq\n");

		while (!(MY_PIO->fstat & ((1 << PIO_FSTAT_TXFULL_LSB) << SIR_PIO_SM))) {		//space in fifo?
			
			int32_t val = pioIrdaSirPrvCircBufGet();
			
			pr("TX 0x%08x\n", val);

			if (val < 0) {	//no more data
				
				SIR_INTE_REG &=~ ((PIO_IRQ0_INTE_SM0_TXNFULL_BITS | PIO_IRQ0_INTE_SM0_RXNEMPTY_BITS) << SIR_PIO_SM);
				break;
			}
			else {			//have data
				
				MY_PIO->txf[SIR_PIO_SM] = val;
			}
		}
	}
	else if (mCurRx) {	//in rx mode
		
		pr("r irq\n");

		uint_fast8_t nItems = 0;
		uint16_t buf[29];
	
		while (!(MY_PIO->fstat & ((1 << PIO_FSTAT_RXEMPTY_LSB) << SIR_PIO_SM))) {		//process data as long as we have it
			
			uint_fast16_t input = MY_PIO->rxf[SIR_PIO_SM], val = pioIrdaSirPrvProcessInput(input);
		
			//did we overrun?
			if (nItems == sizeof(buf) / sizeof(*buf)) {

				buf[sizeof(buf) / sizeof(*buf) - 1] |= UART_BIT_MASK_RX_OVERRUN;
				break;
			}

			buf[nItems++] = val;
		}
		
		if (mIrRxF)
			mIrRxF(mIrRxD, buf, nItems);
	}
	else {
		
		//spurious
		SIR_INTE_REG &=~ ((PIO_IRQ0_INTE_SM0_TXNFULL_BITS | PIO_IRQ0_INTE_SM0_RXNEMPTY_BITS) << SIR_PIO_SM);
	}
}

static uint_fast8_t pioIrdaSirPrvEvenParity(uint_fast8_t val)	//calc bit to add to data to keep numbr of high bits even
{
	val ^= val >> 4;
	val ^= val >> 2;
	val ^= val >> 1;
	
	return val & 1;
}

static uint_fast8_t pioIrdaSirPrvOddParity(uint_fast8_t val)	//calc bit to add to data to keep numbr of high bits odd
{
	val ^= val >> 4;
	val ^= val >> 2;
	val ^= val >> 1;
	
	return 1 - (val & 1);
}

static uint_fast16_t pioIrdaSirPrvXformData(uint8_t byte)
{
	uint32_t val = 1 + ((uint32_t)((uint8_t)~byte)) * 2;		//start bit and data
	
	if (mParityFunc)
		val += ((uint32_t)mParityFunc(byte)) << (mDataBits + 1);
	
	//stop bits are zero here and thus no work to do for them
	
	return val;
}

uint32_t irdaSIRuartTx(const uint8_t *data, uint32_t len, bool block)
{
	uint32_t lenOrig = len;
		
	if (!data)			//we do not support sending breaks using IrDA
		return 0;
	
	if (!mCurTx)
		return 0;
	
	while (len) {
		
		if (pioIrdaSirPrvCircBufAdd(pioIrdaSirPrvXformData(*data))) {
						
			SIR_INTE_REG = PIO_IRQ0_INTE_SM0_TXNFULL_BITS << SIR_PIO_SM;
			
			len--;
			data++;
			continue;
		}
				
		if (!block)
			break;
	}
	
	return lenOrig - len;
}

static uint32_t pioIrdaSirPrvCalcProperBaudrate(uint32_t requested)
{
	static const uint32_t acceptableBaudrates[] = {2400, 9600, 19200, 38400, 57600, 115200};
	uint_fast8_t i;
	
	for (i = 0; i < sizeof(acceptableBaudrates) / sizeof(*acceptableBaudrates) && requested >= acceptableBaudrates[i]; i++);
	
	return i ? acceptableBaudrates[i - 1] : 0;
}

static bool pioIrdaSirPrvIsTxOngoing(void)
{
	//we must be in TX mode for TX to be ongiong
	if (!mCurTx)
		return false;
	
	//if ints are on that means TX is ongoing
	if (SIR_INTE_REG &((PIO_IRQ0_INTE_SM0_TXNFULL_BITS | PIO_IRQ0_INTE_SM0_RXNEMPTY_BITS) << SIR_PIO_SM))
		return true;
	
	//if the tx FIFO is not empty, TX is ongoing
	if (!(MY_PIO->fstat & ((1 << PIO_FSTAT_TXEMPTY_LSB) << SIR_PIO_SM)))
		return true;
	
	//if PX has not yet reached our "parking" pc, tx is ongoing
	if (MY_PIO->sm[SIR_PIO_SM].addr != SIR_PIO_FIRST_USED_PC)
		return true;
	
	//we could have read the above just as the SM read the last word from the FIFO, so recheck again
	if (MY_PIO->sm[SIR_PIO_SM].addr != SIR_PIO_FIRST_USED_PC)
		return true;
	
	return false;
}

static void pioIrdaSirPrvXcvrSetEnabled(bool en)
{
	#ifndef BOARD_01
		if (en)
			sio_hw->gpio_clr = 1 << PIN_IRDA_SD;
		else
			sio_hw->gpio_set = 1 << PIN_IRDA_SD;
	#endif
}

bool irdaSIRuartConfig(union UartCfg *cfg, IrdaSIRuartRxF rxf, void *userData)		///xxx
{
	if (mCurTx) {	//wait for TX to be done
	
		while(pioIrdaSirPrvIsTxOngoing());
	}
	
	pioIrdaSirPrvUnsetup();
	mIrRxF = NULL;
	
	
	if (cfg->rxEn && cfg->txEn) {		//invalid config
		pr("IR port requested RX and TX at once\n");
		return false;
	}
	
	if (!cfg->rxEn && !cfg->txEn) {	//off
		
		pioIrdaSirPrvXcvrSetEnabled(false);
		return true;
	}
	
	cfg->baudrate = pioIrdaSirPrvCalcProperBaudrate(cfg->baudrate);
	if (!cfg->baudrate) {
		pr("IR port requested impossible baudrate\n");
		return false;
	}
	
	if (!cfg->parEna)
		mParityFunc = NULL;
	else
		mParityFunc = cfg->parEven ? pioIrdaSirPrvEvenParity : pioIrdaSirPrvOddParity;
	
	mStopBits = 1 + cfg->stopBits / 2;	//round up to full bits
	mDataBits = 5 + cfg->charBits;
	cfg->stopBits = 2 * mStopBits - 1;
	
	if (cfg->rxEn) {
		
		mCurRx = true;
		pioIrdaSirPrvSetupRx(cfg->baudrate, mDataBits, mParityFunc ? 1 : 0, mStopBits);
	}
	else if (cfg->txEn) {
		
		mCurTx = true;
		pioIrdaSirPrvSetupTx(cfg->baudrate, mDataBits, mParityFunc ? 1 : 0, mStopBits);
	}
	
	pioIrdaSirPrvXcvrSetEnabled(true);
	
	asm volatile("":::"memory");
	mIrRxD = userData;
	asm volatile("":::"memory");
	mIrRxF = rxf;

	return true;
}

uint32_t irdaSIRuartGetSta(void)
{
	uint32_t ret = 0;
	
	if (mCurTx) {
		
		if (!pioIrdaSirPrvIsTxOngoing())
			ret += UART_STA_BIT_TX_FIFO_EMPTY;
		
		if (pioIrdaSirPrvCircBufIsFull())
			ret += UART_STA_BIT_TX_FIFO_FULL;
		
		ret += UART_STA_BIT_RX_FIFO_EMPTY;
	}
	else if (mCurRx) {
		
		ret += UART_STA_BIT_TX_FIFO_EMPTY;
		
		if (MY_PIO->fstat & ((1 << PIO_FSTAT_RXEMPTY_LSB) << SIR_PIO_SM))
			ret += UART_STA_BIT_RX_FIFO_EMPTY;
	}
	
	return ret;
}
