#ifndef DC32_DOOM_COMPAT_PICO_STDLIB_H
#define DC32_DOOM_COMPAT_PICO_STDLIB_H

#include "pico.h"

static inline void stdio_init_all(void)
{
}

static inline void busy_wait_us(uint32_t us)
{
	(void)us;
}

#endif
