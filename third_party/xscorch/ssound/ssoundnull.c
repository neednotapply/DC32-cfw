/* $Header: /fridge/cvs/xscorch/ssound/ssoundnull.c,v 1.6 2009-04-26 17:39:59 jacob Exp $ */
/*
   
   xscorch - ssoundnull.c     Copyright(c) 2001,2000 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/
    
   xscorch null sound driver
   

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
#include <ssound.h>



void sc_sound_init(void) {
/* sc_sound_init */
   
}



sc_sound *sc_sound_new(__libj_unused int enable, __libj_unused int hqmixer) {

   return(NULL);
   
}



void sc_sound_config(__libj_unused sc_sound **s, __libj_unused int enable, __libj_unused int hqmixer) {

}



void sc_sound_free(sc_sound **s) {

   if(s == NULL || *s == NULL) return;
   free(*s);
   *s = NULL;
   
}



void sc_sound_start(__libj_unused sc_sound *s, __libj_unused sc_sound_music id) {

}



void sc_sound_update(__libj_unused sc_sound *s) {
   
}



void sc_sound_stop(__libj_unused sc_sound *s) {

}



void sc_sound_pause(__libj_unused sc_sound *s) {

}



void sc_sound_unpause(__libj_unused sc_sound *s) {

}



