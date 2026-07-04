#include <stdbool.h>
#include "pinoutRp2350defcon.h"
#include "pioIrdaSIR.h"
#include "timebase.h"
#include "2350.h"
#include "irRemote.h"

static bool mIrRemoteActive;
static bool mOpenLasirRxActive;
static uint32_t mOpenLasirRaw;
static uint8_t mOpenLasirBit;

static void irRemotePrvOut(bool on)
{
	if (on)
		sio_hw->gpio_set = 1 << PIN_IRDA_OUT;
	else
		sio_hw->gpio_clr = 1 << PIN_IRDA_OUT;
}

void irRemoteBegin(void)
{
	union UartCfg offCfg = {.raw32 = 0};

	if (mIrRemoteActive)
		return;
	(void)irdaSIRuartConfig(&offCfg, NULL, NULL);

	iobank0_hw->io[PIN_IRDA_OUT].ctrl = (iobank0_hw->io[PIN_IRDA_OUT].ctrl &~ IO_BANK0_GPIO0_CTRL_FUNCSEL_BITS) | (IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SIO_0 << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB);
	iobank0_hw->io[PIN_IRDA_SD].ctrl = (iobank0_hw->io[PIN_IRDA_SD].ctrl &~ IO_BANK0_GPIO0_CTRL_FUNCSEL_BITS) | (IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SIO_0 << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB);

	irRemotePrvOut(false);
	sio_hw->gpio_oe_set = (1 << PIN_IRDA_OUT) | (1 << PIN_IRDA_SD);
	sio_hw->gpio_clr = 1 << PIN_IRDA_SD;
	mIrRemoteActive = true;
}

void irRemoteEnd(void)
{
	union UartCfg offCfg = {.raw32 = 0};

	if (!mIrRemoteActive)
		return;
	irRemotePrvOut(false);
	sio_hw->gpio_set = 1 << PIN_IRDA_SD;
	delayUsec(1000);
	(void)irdaSIRuartConfig(&offCfg, NULL, NULL);
	mIrRemoteActive = false;
}

void irRemoteSpaceUsec(uint32_t usec)
{
	irRemotePrvOut(false);
	delayUsec(usec);
}

void irRemoteMarkUsec(uint32_t carrierHz, uint32_t usec)
{
	uint64_t end, period, half;

	if (!carrierHz) {
		irRemoteSpaceUsec(usec);
		return;
	}

	period = (TICKS_PER_SECOND + carrierHz / 2) / carrierHz;
	if (period < 2)
		period = 2;
	half = period / 2;
	end = getTime() + (uint64_t)usec * (TICKS_PER_SECOND / 1000000);

	while (getTime() < end) {
		uint64_t now = getTime();

		irRemotePrvOut(true);
		while (getTime() - now < half && getTime() < end);

		irRemotePrvOut(false);
		while (getTime() - now < period && getTime() < end);
	}

	irRemotePrvOut(false);
}

static bool irRemotePrvNear(uint32_t value, uint32_t target, uint32_t tolerance)
{
	return value >= target - tolerance && value <= target + tolerance;
}

bool irRemoteOpenLasirBeginReceive(void)
{
	mOpenLasirRaw = 0;
	mOpenLasirBit = 0xff;
	mOpenLasirRxActive = irRemoteRxControl(IrRemoteRxControlBegin, NULL);
	return mOpenLasirRxActive;
}

void irRemoteOpenLasirEndReceive(void)
{
	if (mOpenLasirRxActive)
		(void)irRemoteRxControl(IrRemoteRxControlEnd, NULL);
	mOpenLasirRxActive = false;
	mOpenLasirBit = 0xff;
}

bool irRemoteOpenLasirFeedSpaceUsec(uint32_t spaceUsec, struct IrRemoteOpenLasirFrame *frame)
{
	uint8_t block, inverse, commandLow, commandHigh;

	if (irRemotePrvNear(spaceUsec, 4500, 900)) {
		mOpenLasirRaw = 0;
		mOpenLasirBit = 0;
		return false;
	}
	if (mOpenLasirBit >= 32)
		return false;
	if (irRemotePrvNear(spaceUsec, 563, 220)) {
		/* zero bit */
	}
	else if (irRemotePrvNear(spaceUsec, 1688, 450)) {
		mOpenLasirRaw |= 1u << mOpenLasirBit;
	}
	else {
		mOpenLasirBit = 0xff;
		return false;
	}

	mOpenLasirBit++;
	if (mOpenLasirBit != 32)
		return false;

	block = (uint8_t)mOpenLasirRaw;
	inverse = (uint8_t)(mOpenLasirRaw >> 8);
	mOpenLasirBit = 0xff;
	if ((uint8_t)(block ^ inverse) != 0xffu)
		return false;
	commandLow = (uint8_t)(mOpenLasirRaw >> 16);
	commandHigh = (uint8_t)(mOpenLasirRaw >> 24);
	/* Match the reference decoder's NEC/OpenLASIR disambiguation rule. */
	if ((uint8_t)(commandLow ^ commandHigh) == 0xffu)
		return false;
	if (frame) {
		frame->raw = mOpenLasirRaw;
		frame->blockId = block;
		frame->deviceId = commandLow;
		frame->mode = (uint8_t)((mOpenLasirRaw >> 24) & 0x1f);
		frame->data = (uint8_t)(mOpenLasirRaw >> 29);
	}
	return true;
}

bool irRemoteOpenLasirPoll(struct IrRemoteOpenLasirFrame *frame)
{
	uint32_t spaceUsec;

	while (mOpenLasirRxActive && irRemoteRxControl(IrRemoteRxControlPoll, &spaceUsec))
		if (irRemoteOpenLasirFeedSpaceUsec(spaceUsec, frame))
			return true;
	return false;
}

static void irRemotePrvOpenLasirByte(uint8_t value)
{
	for (uint_fast8_t bit = 0; bit < 8; bit++) {
		irRemoteMarkUsec(38000, 563);
		irRemoteSpaceUsec((value & (1u << bit)) ? 1688 : 563);
	}
}

void irRemoteOpenLasirSend(uint8_t blockId, uint8_t deviceId, uint8_t color)
{
	bool restartRx = mOpenLasirRxActive;
	uint8_t modeData = (uint8_t)((color & 7u) << 5);

	if (restartRx)
		irRemoteOpenLasirEndReceive();
	irRemoteBegin();
	irRemoteMarkUsec(38000, 9000);
	irRemoteSpaceUsec(4500);
	irRemotePrvOpenLasirByte(blockId);
	irRemotePrvOpenLasirByte((uint8_t)~blockId);
	irRemotePrvOpenLasirByte(deviceId);
	irRemotePrvOpenLasirByte(modeData);
	irRemoteMarkUsec(38000, 563);
	irRemoteEnd();
	if (restartRx)
		(void)irRemoteOpenLasirBeginReceive();
}
