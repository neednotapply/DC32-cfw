#ifndef _MP3_PLAYER_H_
#define _MP3_PLAYER_H_

#include <stdbool.h>
#include <stdint.h>
#include "fatfs.h"

enum Mp3PlayerControl {
	Mp3PlayerControlNone,
	Mp3PlayerControlPause,
	Mp3PlayerControlStop,
	Mp3PlayerControlPrev,
	Mp3PlayerControlNext,
};

struct Mp3PlayerStatus {
	uint32_t bytesPlayed;
	uint32_t fileSize;
	uint32_t sampleRate;
	bool paused;
};

typedef enum Mp3PlayerControl (*Mp3PlayerControlF)(void *userData, const struct Mp3PlayerStatus *status);

enum Mp3PlayerResult {
	Mp3PlayerResultDone,
	Mp3PlayerResultStopped,
	Mp3PlayerResultPrev,
	Mp3PlayerResultNext,
	Mp3PlayerResultFileError,
	Mp3PlayerResultDecodeError,
};

enum Mp3PlayerResult mp3PlayerPlayFile(struct FatfsFil *fil, Mp3PlayerControlF controlF, void *userData);

#endif
