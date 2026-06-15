/* $Header: /fridge/cvs/xscorch/ssound/ssound.c,v 1.9 2009-04-26 17:39:59 jacob Exp $ */
/*
   
   xscorch - ssound.c         Copyright(c) 2000-2003 Justin David Smith
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
#include <mikmod.h>

#include <ssound.h>

#include <libj/jstr/libjstr.h>



/* List of song names */
static const char *_sc_sound_names[] = {
   "prelude.xm",
   "inventory.xm",
   "round.xm",
   "endround.xm",
   "endgame.xm",
   NULL
};



static bool _sc_sound_try_load(sc_sound *s, sc_sound_music id) {
/* sc_sound_try_load
   Attempt to load the song given by the ID.  On success, true is returned
   and s->module will refer to the song.  On failure, false is returned.
   Note, Any previously loaded song must be unloaded before this function 
   is called.  */

   /* Temporary buffer to write filename into */
   char filename[SC_SOUND_BUFFER];

   /* Construct the suspected filename for the song */
   sbprintf(filename, sizeof(filename), "%s/%s/%s", SC_GLOBAL_DIR, SC_SOUND_DIR, _sc_sound_names[id]);
   
   /* Attempt to load the song */
   s->module = Player_Load(filename, 64, 0);
   if(s->module == NULL) {
      /* Failure; see what went wrong, then give up */
      /* TEMP TEMP TEMP TEMP TEMP
         fprintf(stderr, "Could not load module \"%s\": \"%s\"\n", _sc_sound_names[id], MikMod_strerror(MikMod_errno));
       */
      return(0);
   }
   
   /* Success: set module parametres and exit */
   s->module->wrap = 1;
   s->module->loop = 1;
   s->module->reppos = SC_SOUND_REVERB;
   return(!0);

}



void sc_sound_init(void) {
/* sc_sound_init
   This function must be called _exactly_ once, to initialise the sound 
   driver.  This function must be called before sound structures are
   created; for MikMod we'll just register everything.  */

   MikMod_RegisterAllDrivers();
   MikMod_RegisterAllLoaders();
   
}



static void _sc_sound_mdmode(sc_sound *s, int hqmixer) {
/* sc_sound_mdmode
   Set driver (global) options.  */

   s->hqmixer = hqmixer;
   md_mode |= DMODE_SOFT_MUSIC | DMODE_INTERP;
   if(hqmixer) {
      md_mode |= DMODE_HQMIXER;
   } else {
      md_mode &= (~DMODE_HQMIXER);
   }
   md_reverb = 0;

}



static void _sc_sound_reset(sc_sound *s) {
/* sc_sound_reset
   Reset the mikmod driver - Warning: all sound samples and modules must
   be reloaded after this call.  Otherwise, nasty segfaults will occur.  */

   /*
   if(s->playing) Player_Stop();
   if(s->module != NULL) Player_Free(s->module);
   */

   #if LIBMIKMOD_VERSION >= 0x030107
   if(MikMod_Reset("")) {
   #else /* Version is old */
   if(MikMod_Reset()) {
   #endif /* Libmikmod ok? */
      fprintf(stderr, "Could not reinitialise sound: \"%s\"\n", MikMod_strerror(MikMod_errno));
   }
   s->playing = 0;

}


   
sc_sound *sc_sound_new(int enable, int hqmixer) {

   sc_sound *s;

   if(!enable) return(NULL);

   s = (sc_sound *)malloc(sizeof(sc_sound));
   if(s == NULL) return(NULL);
   s->playing = 0;
   s->module = NULL;

   _sc_sound_mdmode(s, hqmixer);

   /* Beware the kludgery */
   #if LIBMIKMOD_VERSION >= 0x030107
   if(MikMod_Init("")) {
   #else /* Version is old */
   if(MikMod_Init()) {
   #endif /* Libmikmod ok? */
      fprintf(stderr, "Could not initialise sound: \"%s\"\n", MikMod_strerror(MikMod_errno));
      sc_sound_free(&s);
      return(NULL);
   }
   
   return(s);
   
}



void sc_sound_config(sc_sound **s, int enable, int hqmixer) {

   if(s == NULL) return;
   if(*s == NULL) {
      *s = sc_sound_new(enable, hqmixer);
   } else {
      if(enable) {
         _sc_sound_mdmode(*s, hqmixer);
         _sc_sound_reset(*s);
      } else {
         sc_sound_free(s);
      }
   }

}



void sc_sound_free(sc_sound **s) {

   if(s == NULL || *s == NULL) return;

   Player_Stop();
   if((*s)->module != NULL) {
      Player_Free((*s)->module);
   }
   MikMod_Exit();

   free(*s);
   *s = NULL;
   
}



void sc_sound_start(sc_sound *s, sc_sound_music id) {

   /*int voice;*/

   if(s == NULL) return;
   _sc_sound_reset(s);
   
   _sc_sound_try_load(s, id);
   if(s->module == NULL) return;
      
   Player_Start(s->module);
   /*
   Player_SetPosition(0);
   for(voice = 0; voice <= 99; ++voice) {
      Player_Unmute(voice);
      Voice_SetVolume(voice, 256);
      Voice_SetPanning(voice, PAN_SURROUND);
   }*/
   
   s->playing = 1;
   sc_sound_update(s);

}



void sc_sound_update(sc_sound *s) {
   
   if(s == NULL || s->module == NULL) return;
   if(s->playing) {
      MikMod_Update();
      if(!Player_Active()) {
         Player_SetPosition(0);
      }
   }

}



void sc_sound_stop(sc_sound *s) {

   if(s == NULL || s->module == NULL) return;
   Player_Stop();
   s->playing = 0;

}



void sc_sound_pause(sc_sound *s) {

   if(s == NULL || s->module == NULL) return;
   if(!Player_Paused()) Player_TogglePause();

}



void sc_sound_unpause(sc_sound *s) {

   if(s == NULL || s->module == NULL) return;
   if(Player_Paused()) Player_TogglePause();

}



