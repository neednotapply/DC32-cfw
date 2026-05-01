#ifndef _WAV_PLAYER_H_
#define _WAV_PLAYER_H_

#include "fatfs.h"
#include "mp3Player.h"

enum Mp3PlayerResult wavPlayerPlayFile(struct FatfsFil *fil, Mp3PlayerControlF controlF, void *userData);

#endif
