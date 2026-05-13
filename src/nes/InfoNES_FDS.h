#ifndef NES_INFONES_FDS_STUB_H
#define NES_INFONES_FDS_STUB_H

#include "InfoNES_Types.h"

extern bool FDS_AudioEnabled;
extern BYTE *fds_wave_buffer;

static inline void fdsAutoInsertCheck(void) {}
static inline void fdsAudioReset(void) {}
static inline void fdsRenderAudio(int n) { (void)n; }
static inline void fdsHSync(void) {}

#endif
