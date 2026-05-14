#ifndef _TINYUSB_COMPAT_HARDWARE_TIMER_H_
#define _TINYUSB_COMPAT_HARDWARE_TIMER_H_

#include "timebase.h"

static inline uint32_t time_us_32(void)
{
	return (uint32_t)(getTime() / (TICKS_PER_SECOND / 1000000u));
}

#endif
