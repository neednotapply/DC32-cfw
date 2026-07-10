#ifndef _ABC_PLAYER_H_
#define _ABC_PLAYER_H_

#include "musicPlayer.h"

enum MusicPlayerResult abcPlayerPlayFile(struct FatfsFil *fil, MusicPlayerControlF controlF,
	void *userData);

#endif
