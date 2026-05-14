#ifndef _TINYUSB_COMPAT_HARDWARE_RESETS_H_
#define _TINYUSB_COMPAT_HARDWARE_RESETS_H_

#include "hardware/structs/resets.h"

static inline void reset_block(uint32_t bits)
{
	resets_hw->reset |= bits;
}

static inline void unreset_block_wait(uint32_t bits)
{
	resets_hw->reset &=~ bits;
	while ((resets_hw->reset_done & bits) != bits) {}
}

#endif
