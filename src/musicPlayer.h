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
	MusicPlayerControlTrackPrev,
	MusicPlayerControlTrackNext,
};

struct MusicPlayerStatus {
	uint32_t bytesPlayed;
	uint32_t fileSize;
	uint32_t bpm;
	uint16_t track;
	uint16_t trackCount;
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
	MusicPlayerResultUnsupported,
};

#endif
