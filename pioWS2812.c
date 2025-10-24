#include "hardware/regs/dreq.h"
#include "pinoutRp2350defcon.h"
#include "pioWS2812.h"
#include "timebase.h"
#include "printf.h"
#include "2350.h"


static uint32_t mLedsBuffer[NUM_WS2812s * 3];	//each word repersents 24 bits sent = 8 bits of data. thus 3 per LED

#define PIO_Nth2(x)							pio ## x ## _hw
#define PIO_Nth(x)							PIO_Nth2(x)
#define DREQ_PIO_TYPE_IDX2(p, t, i)			DREQ_PIO ## p ## _ ## t ## i
#define DREQ_PIO_TYPE_IDX(p, t, i)			DREQ_PIO_TYPE_IDX2(p, t, i)
#define MY_PIO								PIO_Nth(I2C_PIO_IDX)


#define SIDE_SET_HAS_ENABLE_BIT				0
#define SIDE_SET_NUM_BITS					0
#define DEFINE_PIO_INSTRS
#include "pioAsm.h"

void ws2812refresh(void)
{
	//sync is 300us of low = 240 bit times. we make it 250
	const uint64_t minPeriodInBitTimes = (NUM_WS2812s * 24 + 250), minPeriodInTicks = TICKS_PER_SECOND * minPeriodInBitTimes / 800000;
	static uint64_t lastStart = 0;

	while (getTime() - lastStart < minPeriodInTicks);
	dma_hw->ch[WS2812_DMA].al3_read_addr_trig = (uintptr_t)&mLedsBuffer;
	lastStart = getTime();
}

void ws2812Set(uint32_t ledIdx, uint_fast8_t colorIdx, uint_fast8_t val)
{
	uint32_t *dst, spread = 0, mask = 0x40000000;
	uint_fast8_t i;

	if (colorIdx >= 3 || ledIdx >= NUM_WS2812s)
		return;

	for (i = 0; i < 8; i++, val <<= 1, mask >>= 3) {
		if (val & 0x80)
			spread += mask;
	}

	dst = &mLedsBuffer[ledIdx * 3 + colorIdx];
	*dst = ((*dst) & 0xb6db6d00) | spread;
}


void ws2812init(void)	//PIO0.SM1, last instr in instr memory
{
	const uint32_t desiredRate = 800000 * 3 /*rate for 1/3 of a bit*/, clkDiv = (TICKS_PER_SECOND + desiredRate / 2) / desiredRate;
	uint_fast8_t pc = WS2812_PIO_FIRST_USED_PC;
	uint32_t i;
	
	//stop
	MY_PIO->ctrl &=~ ((1 << PIO_CTRL_SM_ENABLE_LSB) << WS2812_PIO_SM);
	MY_PIO->ctrl |= (1 << PIO_CTRL_SM_RESTART_LSB) << WS2812_PIO_SM;

	//"code"
	MY_PIO->instr_mem[pc++] = I_OUT(0, 0, OUT_DST_PINS, 1);

	pr("ws2812: using %u instrs, next free is %u\n", pc - WS2812_PIO_FIRST_USED_PC, pc);

	//config
	MY_PIO->sm[WS2812_PIO_SM].clkdiv = (clkDiv << PIO_SM0_CLKDIV_INT_LSB);
	MY_PIO->sm[WS2812_PIO_SM].execctrl = (MY_PIO->sm[WS2812_PIO_SM].execctrl &~ (PIO_SM0_EXECCTRL_WRAP_TOP_BITS | PIO_SM0_EXECCTRL_WRAP_BOTTOM_BITS | PIO_SM0_EXECCTRL_SIDE_EN_BITS)) | (WS2812_PIO_FIRST_USED_PC << PIO_SM0_EXECCTRL_WRAP_TOP_LSB) | (WS2812_PIO_FIRST_USED_PC << PIO_SM0_EXECCTRL_WRAP_BOTTOM_LSB) | (SIDE_SET_HAS_ENABLE_BIT ? PIO_SM2_EXECCTRL_SIDE_EN_BITS : 0);
	MY_PIO->sm[WS2812_PIO_SM].shiftctrl = (MY_PIO->sm[WS2812_PIO_SM].shiftctrl &~ (PIO_SM0_SHIFTCTRL_PULL_THRESH_BITS | PIO_SM0_SHIFTCTRL_PUSH_THRESH_BITS | PIO_SM0_SHIFTCTRL_IN_SHIFTDIR_BITS | PIO_SM0_SHIFTCTRL_OUT_SHIFTDIR_BITS | PIO_SM0_SHIFTCTRL_AUTOPUSH_BITS | PIO_SM1_SHIFTCTRL_FJOIN_RX_BITS)) | PIO_SM0_SHIFTCTRL_AUTOPULL_BITS | PIO_SM1_SHIFTCTRL_FJOIN_TX_BITS | (24 << PIO_SM0_SHIFTCTRL_PULL_THRESH_LSB);
	MY_PIO->sm[WS2812_PIO_SM].pinctrl = (SIDE_SET_BITS_USED << PIO_SM0_PINCTRL_SIDESET_COUNT_LSB) | (1 << PIO_SM0_PINCTRL_SET_COUNT_LSB) | (1 << PIO_SM0_PINCTRL_OUT_COUNT_LSB) | (PIN_WS2812 << PIO_SM0_PINCTRL_SET_BASE_LSB) | (PIN_WS2812 << PIO_SM0_PINCTRL_OUT_BASE_LSB);

	//pin
	MY_PIO->sm[WS2812_PIO_SM].instr = I_SET(0, 0, SET_DST_PINS, 0);
	MY_PIO->sm[WS2812_PIO_SM].instr = I_SET(0, 0, SET_DST_PINDIRS, 1);

	//start
	MY_PIO->sm[WS2812_PIO_SM].instr = I_JMP(0, 0, JMP_ALWAYS, WS2812_PIO_FIRST_USED_PC);
	MY_PIO->ctrl |= (1 << PIO_CTRL_SM_ENABLE_LSB) << WS2812_PIO_SM;

	//dma
	dma_hw->ch[WS2812_DMA].write_addr = (uintptr_t)&MY_PIO->txf[WS2812_PIO_SM];
	dma_hw->ch[WS2812_DMA].transfer_count = sizeof(mLedsBuffer) / sizeof(*mLedsBuffer);
	dma_hw->ch[WS2812_DMA].al1_ctrl = (DREQ_PIO_TYPE_IDX(WS2812_PIO_IDX, TX, WS2812_PIO_SM) << DMA_CH0_CTRL_TRIG_TREQ_SEL_LSB) | (WS2812_DMA << DMA_CH0_CTRL_TRIG_CHAIN_TO_LSB) | (DMA_CH0_CTRL_TRIG_DATA_SIZE_VALUE_SIZE_WORD << DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB) | DMA_CH1_CTRL_TRIG_INCR_READ_BITS | DMA_CH0_CTRL_TRIG_EN_BITS;

	//init frame bits
	for (i = 0; i < NUM_WS2812s * 3; i++)
		mLedsBuffer[i] = 0x92492400;

	//refresh twice (first can be bad due to initial line high)
	ws2812refresh();
	ws2812refresh();
}

