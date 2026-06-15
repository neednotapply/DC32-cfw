/* $Header: /fridge/cvs/xscorch/sgtk/sexplosion-gtk.h,v 1.8 2009-04-26 17:39:48 jacob Exp $ */
/*
   
   xscorch - sexplosion-gtk.h Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/
    
   GTK interface to the explosion cache
    

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
#ifndef __sexplosion_gtk_h_included
#define __sexplosion_gtk_h_included


/* Includes */
#include <sgtk.h>
#include <sgame/sconfig.h>
#include <sgame/sexplosion.h>
#include <sgame/swindow.h>


/* Define some constants for the characteristics of the cache */
#define  SC_EXPL_CACHE_SIZE   48    /* Number of entries in the cache. */


/* Structure describing a single explosion in the cache. */
typedef struct _sc_expl_cache_entry_gtk {
   GdkPixmap *pixmap;      /* Explosion pixmap */
   sc_explosion_type type; /* Type of explosion to draw. */
   int radius;             /* Size of this explosion. */
} sc_expl_cache_entry_gtk;


/* Structure holding the explosion cache. */
typedef struct _sc_expl_cache_gtk {
   sc_expl_cache_entry_gtk cache[SC_EXPL_CACHE_SIZE]; /* Each entry */
   int cachesize;          /* Size of this cache */
   int headptr;            /* Pointer to most recent addition */
   GdkBitmap *fakebitmap;  /* Fake bitmap,needed to create GC */
   GdkGC *bitmapgc;        /* Graphic context for bitmaps */
} sc_expl_cache_gtk;


/* Functions */
sc_expl_cache_gtk *sc_expl_cache_new_gtk(void);
void sc_expl_cache_free_gtk(sc_expl_cache_gtk **cache);


#endif /* __sexplosion_gtk_h_included */
