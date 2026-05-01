#include "pinoutRp2350defcon.h"
#include "timebase.h"
#include "2350.h"
#include "ir_tx.h"

#define IR_TX_PIO			pio0_hw
#define IR_TX_SM			1
#define IR_TX_FIRST_PC		2

#define SIDE_SET_HAS_ENABLE_BIT	0
#define SIDE_SET_NUM_BITS		0
#define DEFINE_PIO_INSTRS
#include "pioAsm.h"
#undef DEFINE_PIO_INSTRS

static uint32_t mSavedIrOutFunc;
static bool mInited;

static void irTxCarrier(bool on)
{
	if (on) {
		IR_TX_PIO->ctrl |= (1 << (PIO_CTRL_SM_ENABLE_LSB + IR_TX_SM));
	}
	else {
		IR_TX_PIO->sm[IR_TX_SM].instr = I_SET(0, 0, SET_DST_PINS, 0);
		IR_TX_PIO->ctrl &=~ (1 << (PIO_CTRL_SM_ENABLE_LSB + IR_TX_SM));
	}
}

static void irTxBegin(uint32_t frequency)
{
	uint32_t div;

	if (!frequency)
		frequency = 38000;

	mSavedIrOutFunc = iobank0_hw->io[PIN_IRDA_OUT].ctrl & IO_BANK0_GPIO0_CTRL_FUNCSEL_BITS;

	IR_TX_PIO->ctrl &=~ (1 << (PIO_CTRL_SM_ENABLE_LSB + IR_TX_SM));
	IR_TX_PIO->ctrl |= (1 << (PIO_CTRL_SM_RESTART_LSB + IR_TX_SM));
	while (IR_TX_PIO->ctrl & (1 << (PIO_CTRL_SM_RESTART_LSB + IR_TX_SM)));

	IR_TX_PIO->instr_mem[IR_TX_FIRST_PC + 0] = I_SET(0, 0, SET_DST_PINS, 1);
	IR_TX_PIO->instr_mem[IR_TX_FIRST_PC + 1] = I_SET(0, 0, SET_DST_PINS, 0);

	div = (TICKS_PER_SECOND * 256ull + frequency) / (frequency * 2ull);
	if (div < 256)
		div = 256;

	IR_TX_PIO->sm[IR_TX_SM].clkdiv = div << PIO_SM0_CLKDIV_FRAC_LSB;
	IR_TX_PIO->sm[IR_TX_SM].execctrl = (IR_TX_PIO->sm[IR_TX_SM].execctrl &~ (PIO_SM0_EXECCTRL_WRAP_TOP_BITS | PIO_SM0_EXECCTRL_WRAP_BOTTOM_BITS)) |
		((IR_TX_FIRST_PC + 1) << PIO_SM0_EXECCTRL_WRAP_TOP_LSB) | (IR_TX_FIRST_PC << PIO_SM0_EXECCTRL_WRAP_BOTTOM_LSB);
	IR_TX_PIO->sm[IR_TX_SM].pinctrl = (1 << PIO_SM0_PINCTRL_SET_COUNT_LSB) | (PIN_IRDA_OUT << PIO_SM0_PINCTRL_SET_BASE_LSB);
	IR_TX_PIO->sm[IR_TX_SM].instr = I_JMP(0, 0, JMP_ALWAYS, IR_TX_FIRST_PC);

	iobank0_hw->io[PIN_IRDA_OUT].ctrl = (iobank0_hw->io[PIN_IRDA_OUT].ctrl &~ IO_BANK0_GPIO0_CTRL_FUNCSEL_BITS) |
		(IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_PIO0_0 << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB);
	sio_hw->gpio_oe_set = 1 << PIN_IRDA_SD;
	sio_hw->gpio_clr = 1 << PIN_IRDA_SD;
	mInited = true;
}

static void irTxEnd(void)
{
	if (!mInited)
		return;

	irTxCarrier(false);
	iobank0_hw->io[PIN_IRDA_OUT].ctrl = (iobank0_hw->io[PIN_IRDA_OUT].ctrl &~ IO_BANK0_GPIO0_CTRL_FUNCSEL_BITS) | mSavedIrOutFunc;
	mInited = false;
}

bool irTxSendRaw(const uint16_t *timings, uint32_t numTimings, uint32_t frequency, bool (*cancelF)(void *userData), void *userData)
{
	uint32_t i;

	if (!timings || !numTimings)
		return false;

	irTxBegin(frequency);
	for (i = 0; i < numTimings; i++) {
		if (cancelF && cancelF(userData))
			break;

		irTxCarrier(!(i & 1));
		delayUsec(timings[i]);
	}
	irTxEnd();
	delayMsec(50);

	return i == numTimings;
}
