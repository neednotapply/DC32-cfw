#ifndef _FLIPPER_IR_H_
#define _FLIPPER_IR_H_

#include <stdbool.h>
#include <stdint.h>
#include "fatfs.h"

struct FlipperIrSignal {
	const uint16_t *timings;
	uint32_t numTimings;
	uint32_t frequency;
	const char *name;
	const char *protocol;
};

typedef bool (*FlipperIrSignalF)(void *userData, const struct FlipperIrSignal *sig, uint32_t index);

bool flipperIrBlastNamed(struct FatfsFil *fil, const char *wantedName, FlipperIrSignalF signalF, void *userData, uint32_t *numSentP, uint32_t *numUnsupportedP);

#endif
