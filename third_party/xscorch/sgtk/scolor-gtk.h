/* $Header: /fridge/cvs/xscorch/sgtk/scolor-gtk.h,v 1.8 2009-04-26 17:39:47 jacob Exp $ */
/*
   
   xscorch - scolor-gtk.h     Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/
    
   GTK colormap
    

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
#ifndef __scolor_gtk_h_included
#define __scolor_gtk_h_included


#include <xscorch.h>
#include <gtk/gtk.h>
#include <sgame/sconfig.h>
#include <sgame/scolor.h>


/* Forward structure definitions */
struct _sc_window_gtk;
struct _sc_color;


/* Structure for storing allocated colors */
typedef struct _sc_color_gtk {
   GdkColor gradient[SC_NUM_GRADIENTS][SC_MAX_GRADIENT_SIZE];
   GdkColor pcolors[SC_MAX_PLAYERS];
   GdkColor black;
   GdkColor gray;
   GdkColor white;
   GdkColor yellow;
   GdkColor napalm;
   GdkColor windar;
} sc_color_gtk;


/* Functions */
sc_color_gtk *sc_colormap_new_gtk(void);
void sc_colormap_free_gtk(sc_color_gtk **color);
void sc_colormap_alloc_colors_gtk(struct _sc_window_gtk *w);


#endif /* __scolor_gtk_h_included */
