#include "pinoutRp2350defcon.h"
#include "timebase.h"
#include "printf.h"
#include "sleep.h"
#include "cpu.h"
#include "ui.h"

void doSleep(void)
{
	sio_hw->gpio_clr = 1 << PIN_SELF_PWR;
}

