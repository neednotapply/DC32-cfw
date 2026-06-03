#include <stdbool.h>
#include "pinoutRp2350defcon.h"
#include "pioIrdaSIR.h"
#include "timebase.h"
#include "2350.h"
#include "irRemote.h"

static bool mIrRemoteActive;

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
