#ifndef DC32_DOOM_COMPAT_I_PICOSOUND_H
#define DC32_DOOM_COMPAT_I_PICOSOUND_H

#include <stdbool.h>

typedef struct audio_buffer audio_buffer_t;

static inline bool I_PicoSoundIsInitialized(void)
{
	return false;
}

static inline void I_PicoSoundSetMusicGenerator(void (*generator)(audio_buffer_t *buffer))
{
	(void)generator;
}

static inline void I_PicoSoundFade(bool in)
{
	(void)in;
}

static inline bool I_PicoSoundFading(void)
{
	return false;
}

#endif
