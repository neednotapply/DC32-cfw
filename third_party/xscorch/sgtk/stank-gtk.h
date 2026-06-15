/* $Header: /fridge/cvs/xscorch/sgtk/stank-gtk.h,v 1.5 2009-04-26 17:39:51 jacob Exp $ */
/*
   
   xscorch - stank-gtk.h      Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/
    
   Tank configuration screens
    

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
#ifndef __stank_gtk_h_included
#define __stank_gtk_h_included


struct _sc_window_gtk;
struct _sc_player;


void sc_window_tank_info_gtk(struct _sc_window_gtk *w, struct _sc_player *p);
void sc_window_tank_move_gtk(struct _sc_window_gtk *w, struct _sc_player *p);


#endif /* __stank_gtk_h_included */
