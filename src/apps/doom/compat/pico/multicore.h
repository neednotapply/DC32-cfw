#ifndef DC32_DOOM_COMPAT_PICO_MULTICORE_H
#define DC32_DOOM_COMPAT_PICO_MULTICORE_H

static inline void multicore_launch_core1(void (*entry)(void))
{
	(void)entry;
}

#endif
