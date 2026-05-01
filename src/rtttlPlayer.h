#ifndef _RTTTL_PLAYER_H_
#define _RTTTL_PLAYER_H_

#include "fatfs.h"
#include "mp3Player.h"

enum Mp3PlayerResult rtttlPlayerPlayFile(struct FatfsFil *fil, Mp3PlayerControlF controlF, void *userData);

#endif
