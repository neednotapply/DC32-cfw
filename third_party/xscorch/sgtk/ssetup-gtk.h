/* $Header: /fridge/cvs/xscorch/sgtk/ssetup-gtk.h,v 1.8 2009-04-26 17:39:50 jacob Exp $ */
/*
   
   xscorch - ssetup-gtk.h     Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/
    
   Function prototypes for the various setup functions
    

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
#ifndef __ssetup_gtk_h_included
#define __ssetup_gtk_h_included


#include <swindow-gtk.h>


/* Interface functions */
void sc_player_setup_gtk(sc_window_gtk *w);
void sc_economy_setup_gtk(sc_window_gtk *w);
void sc_physics_setup_gtk(sc_window_gtk *w);
void sc_land_setup_gtk(sc_window_gtk *w);
void sc_weapons_setup_gtk(sc_window_gtk *w);
void sc_options_setup_gtk(sc_window_gtk *w);
void sc_graphics_setup_gtk(sc_window_gtk *w);
void sc_ai_controller_setup_gtk(sc_window_gtk *w);
void sc_sound_setup_gtk(sc_window_gtk *w);
void sc_config_file_save_gtk(sc_window_gtk *w);
void sc_font_gtk(sc_window_gtk *w);


#endif /* __ssetup_gtk_h_included */
