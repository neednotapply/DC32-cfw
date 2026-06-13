#ifndef DC32_DOOM_COMPAT_PICO_DIVIDER_H
#define DC32_DOOM_COMPAT_PICO_DIVIDER_H

#include <stdint.h>

static inline uint32_t hw_divider_u32_quotient_inlined(uint32_t a, uint32_t b)
{
	return b ? a / b : 0xffffffffu;
}

static inline int32_t hw_divider_s32_quotient_inlined(int32_t a, int32_t b)
{
	return b ? a / b : (a < 0 ? INT32_MIN : INT32_MAX);
}

#endif
