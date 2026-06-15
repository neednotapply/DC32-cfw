/* $Header: /fridge/cvs/xscorch/ssound/ssound.h,v 1.5 2009-04-26 17:39:59 jacob Exp $ */
/*
   
   xscorch - ssound.h         Copyright(c) 2001,2000 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/
    
   xscorch sound driver
   

   This program is free software; you can redistribute it and/or modify 
   it under the terms of the GNU General Public License as published by 
   the Free Software Foundation, version 2 of the License ONLY. 

   This program is distributed in the hope that it will be useful, 
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
   General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

*/
#ifndef __ssound_h_included
#define __ssound_h_included


#include <xscorch.h>
#ifndef USE_SOUND
   #error USE_SOUND must be defined
#endif /* Sanity check */


/* Forward structure declarations */
struct MODULE;


#define SC_SOUND_BUFFER    0x1000
#define SC_SOUND_REVERB    0


typedef enum _sc_sound_music {
   SC_MUSIC_PRELUDE,
   SC_MUSIC_INVENTORY,
   SC_MUSIC_ROUND,
   SC_MUSIC_ENDROUND,
   SC_MUSIC_ENDGAME
} sc_sound_music;


typedef struct _sc_sound {
   struct MODULE *module;
   int playing;
   int hqmixer;
} sc_sound;



void sc_sound_init(void);
sc_sound *sc_sound_new(int enable, int hqmixer);
void sc_sound_config(sc_sound **s, int enable, int hqmixer);
void sc_sound_free(sc_sound **s);

void sc_sound_start(sc_sound *s, sc_sound_music id);
void sc_sound_update(sc_sound *s);
void sc_sound_stop(sc_sound *s);
void sc_sound_pause(sc_sound *s);
void sc_sound_unpause(sc_sound *s);


#endif /* __ssound_h_included */
