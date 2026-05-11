#ifndef NES_INFONES_NSF_STUB_H
#define NES_INFONES_NSF_STUB_H

#include "InfoNES_Types.h"

extern bool IsNSF;
extern bool NsfIsPlaying;
extern BYTE *NsfBank4K[8];

static inline void nsfSetupCpuState(void) {}

#endif
