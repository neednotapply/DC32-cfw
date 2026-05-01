#ifndef _RTTTL_PLAYER_H_
#define _RTTTL_PLAYER_H_

#include "fatfs.h"
#include "musicPlayer.h"

enum MusicPlayerResult rtttlPlayerPlayFile(struct FatfsFil *fil, MusicPlayerControlF controlF, void *userData);

#endif
