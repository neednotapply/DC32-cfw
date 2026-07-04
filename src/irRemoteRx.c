#include <stdbool.h>
#include <stdint.h>
#include "irRemote.h"
#include "pinoutRp2350defcon.h"
#include "pioIrdaSIR.h"
#include "timebase.h"
#include "2350.h"

#define IRRX_PIO pio0_hw
#define IRRX_SM 1u
#define IRRX_PC 2u
#define IRRX_CLOCK_HZ 1000000u
#define IRRX_RING_SIZE 128u
#define IRRX_SM_ENABLE_BIT ((1u << PIO_CTRL_SM_ENABLE_LSB) << IRRX_SM)
#define IRRX_SM_RESTART_BIT ((1u << PIO_CTRL_SM_RESTART_LSB) << IRRX_SM)
#define IRRX_IRQ_MASK (PIO_IRQ0_INTE_SM0_RXNEMPTY_BITS << IRRX_SM)
#define IRRX_QUALIFY_USEC 64u

#define SIDE_SET_HAS_ENABLE_BIT 0
#define SIDE_SET_NUM_BITS 0
#define DEFINE_PIO_INSTRS
#include "pioAsm.h"
#undef DEFINE_PIO_INSTRS

static volatile uint32_t mIrRxRing[IRRX_RING_SIZE];
static volatile uint8_t mIrRxRead, mIrRxWrite;
static bool mIrRxActive;

void PIO0_IRQ_1_IRQHandler(void)
{
	while (!(IRRX_PIO->fstat & ((1u << PIO_FSTAT_RXEMPTY_LSB) << IRRX_SM))) {
		uint32_t raw = IRRX_PIO->rxf[IRRX_SM];
		uint32_t count = ~raw;
		uint32_t usec = IRRX_QUALIFY_USEC + count * 2u;
		uint8_t next = (uint8_t)((mIrRxWrite + 1u) % IRRX_RING_SIZE);

		if (next != mIrRxRead) {
			mIrRxRing[mIrRxWrite] = usec;
			mIrRxWrite = next;
		}
	}
}

static void irRemoteRxPrvEnd(void)
{
	NVIC_DisableIRQ(PIO0_IRQ_1_IRQn);
	IRRX_PIO->inte1 &= ~IRRX_IRQ_MASK;
	IRRX_PIO->ctrl &= ~IRRX_SM_ENABLE_BIT;
	IRRX_PIO->ctrl |= IRRX_SM_RESTART_BIT;
	IRRX_PIO->sm[IRRX_SM].shiftctrl = PIO_SM0_SHIFTCTRL_FJOIN_RX_BITS;
	IRRX_PIO->sm[IRRX_SM].shiftctrl = 0;
	IRRX_PIO->fdebug = (1u << (PIO_FDEBUG_RXSTALL_LSB + IRRX_SM)) |
		(1u << (PIO_FDEBUG_RXUNDER_LSB + IRRX_SM));
	NVIC_ClearPendingIRQ(PIO0_IRQ_1_IRQn);
	mIrRxRead = 0;
	mIrRxWrite = 0;
	mIrRxActive = false;
	sio_hw->gpio_set = 1u << PIN_IRDA_SD;
}

static bool irRemoteRxPrvBegin(void)
{
	union UartCfg offCfg = {.raw32 = 0};
	uint_fast8_t pc = IRRX_PC;
	uint_fast8_t start, qualify, measure, end;

	irRemoteRxPrvEnd();
	(void)irdaSIRuartConfig(&offCfg, NULL, NULL);

	/*
	 * The badge transceiver can expose either one demodulated low mark or the
	 * individual 38 kHz carrier pulses. Wait for a low pulse, then require the
	 * following high interval to survive 64 us. Short carrier gaps restart the
	 * filter; real NEC spaces are measured and pushed to the resident ring.
	 */
	start = pc;
	IRRX_PIO->instr_mem[pc++] = I_WAIT(0, 0, 0, WAIT_FOR_GPIO, PIN_IRDA_IN);
	IRRX_PIO->instr_mem[pc++] = I_WAIT(0, 0, 1, WAIT_FOR_GPIO, PIN_IRDA_IN);
	IRRX_PIO->instr_mem[pc++] = I_SET(0, 0, SET_DST_Y, 31);
	qualify = pc;
	IRRX_PIO->instr_mem[pc++] = I_JMP(0, 0, JMP_PIN, qualify + 2);
	IRRX_PIO->instr_mem[pc++] = I_JMP(0, 0, JMP_ALWAYS, start);
	IRRX_PIO->instr_mem[pc++] = I_JMP(0, 0, JMP_Y_POSTDEC, qualify);
	IRRX_PIO->instr_mem[pc++] = I_MOV(0, 0, MOV_DST_X, MOV_OP_INVERT, MOV_SRC_ZEROES);
	measure = pc;
	IRRX_PIO->instr_mem[pc++] = I_JMP(0, 0, JMP_PIN, measure + 4);
	IRRX_PIO->instr_mem[pc++] = I_MOV(0, 0, MOV_DST_ISR, MOV_OP_COPY, MOV_SRC_X);
	IRRX_PIO->instr_mem[pc++] = I_PUSH(0, 0, 0, 1);
	IRRX_PIO->instr_mem[pc++] = I_JMP(0, 0, JMP_ALWAYS, start);
	IRRX_PIO->instr_mem[pc++] = I_JMP(0, 0, JMP_X_POSTDEC, measure);
	end = pc - 1u;

	iobank0_hw->io[PIN_IRDA_IN].ctrl =
		(iobank0_hw->io[PIN_IRDA_IN].ctrl & ~IO_BANK0_GPIO0_CTRL_FUNCSEL_BITS) |
		(IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_PIO0_0 << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB);
	iobank0_hw->io[PIN_IRDA_SD].ctrl =
		(iobank0_hw->io[PIN_IRDA_SD].ctrl & ~IO_BANK0_GPIO0_CTRL_FUNCSEL_BITS) |
		(IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SIO_0 << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB);
	sio_hw->gpio_oe_set = 1u << PIN_IRDA_SD;
	sio_hw->gpio_clr = 1u << PIN_IRDA_SD;

	IRRX_PIO->sm[IRRX_SM].clkdiv =
		(uint32_t)(((uint64_t)TICKS_PER_SECOND * 256u + IRRX_CLOCK_HZ / 2u) / IRRX_CLOCK_HZ) <<
		PIO_SM0_CLKDIV_FRAC_LSB;
	IRRX_PIO->ctrl |= (1u << PIO_CTRL_CLKDIV_RESTART_LSB) << IRRX_SM;
	IRRX_PIO->sm[IRRX_SM].execctrl =
		(IRRX_PIO->sm[IRRX_SM].execctrl & ~(PIO_SM0_EXECCTRL_WRAP_TOP_BITS |
		 PIO_SM0_EXECCTRL_WRAP_BOTTOM_BITS | PIO_SM2_EXECCTRL_SIDE_EN_BITS)) |
		(end << PIO_SM0_EXECCTRL_WRAP_TOP_LSB) |
		(start << PIO_SM0_EXECCTRL_WRAP_BOTTOM_LSB) |
		(PIN_IRDA_IN << PIO_SM2_EXECCTRL_JMP_PIN_LSB);
	IRRX_PIO->sm[IRRX_SM].shiftctrl = PIO_SM0_SHIFTCTRL_FJOIN_RX_BITS;
	IRRX_PIO->sm[IRRX_SM].pinctrl = PIN_IRDA_IN << PIO_SM1_PINCTRL_IN_BASE_LSB;
	IRRX_PIO->sm[IRRX_SM].instr = I_JMP(0, 0, JMP_ALWAYS, start);

	IRRX_PIO->inte1 &= ~IRRX_IRQ_MASK;
	NVIC_ClearPendingIRQ(PIO0_IRQ_1_IRQn);
	NVIC_EnableIRQ(PIO0_IRQ_1_IRQn);
	IRRX_PIO->inte1 |= IRRX_IRQ_MASK;
	IRRX_PIO->ctrl |= IRRX_SM_ENABLE_BIT;
	mIrRxActive = true;
	return true;
}

bool irRemoteRxControl(enum IrRemoteRxControl control, uint32_t *spaceUsecP)
{
	switch (control) {
	case IrRemoteRxControlBegin:
		return irRemoteRxPrvBegin();
	case IrRemoteRxControlEnd:
		irRemoteRxPrvEnd();
		return true;
	case IrRemoteRxControlPoll:
		if (!mIrRxActive)
			return false;
		if (mIrRxRead != mIrRxWrite) {
			uint8_t read = mIrRxRead;
			if (spaceUsecP)
				*spaceUsecP = mIrRxRing[read];
			mIrRxRead = (uint8_t)((read + 1u) % IRRX_RING_SIZE);
			return true;
		}
		return false;
	}
	return false;
}
