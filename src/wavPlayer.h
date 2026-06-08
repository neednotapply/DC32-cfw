#ifndef _WAV_PLAYER_H_
#define _WAV_PLAYER_H_

#include "musicPlayer.h"

enum MusicPlayerResult wavPlayerPlayFile(struct FatfsFil *fil, MusicPlayerControlF controlF, void *userData);

#endif
