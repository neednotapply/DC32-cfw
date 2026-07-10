#ifndef _MIDI_PLAYER_H_
#define _MIDI_PLAYER_H_

#include "musicPlayer.h"

/* scratch is retained in the host contract; the streaming player does not require it. */
enum MusicPlayerResult midiPlayerPlayFile(struct FatfsFil *fil, void *scratch, uint32_t scratchSize,
	MusicPlayerControlF controlF, void *userData);

#endif
