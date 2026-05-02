#ifndef _MUSIC_PLAYER_H_
#define _MUSIC_PLAYER_H_

#include <stdbool.h>
#include <stdint.h>
#include "fatfs.h"

enum MusicPlayerControl {
	MusicPlayerControlNone,
	MusicPlayerControlPause,
	MusicPlayerControlStop,
	MusicPlayerControlPrev,
	MusicPlayerControlNext,
};

struct MusicPlayerStatus {
	uint32_t bytesPlayed;
	uint32_t fileSize;
	uint32_t sampleRate;
	bool paused;
};

typedef enum MusicPlayerControl (*MusicPlayerControlF)(void *userData, const struct MusicPlayerStatus *status);

enum MusicPlayerResult {
	MusicPlayerResultDone,
	MusicPlayerResultStopped,
	MusicPlayerResultPrev,
	MusicPlayerResultNext,
	MusicPlayerResultFileError,
	MusicPlayerResultDecodeError,
};

#endif
