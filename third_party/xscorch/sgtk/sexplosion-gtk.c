/* $Header: /fridge/cvs/xscorch/sgtk/sexplosion-gtk.c,v 1.14 2009-04-26 17:39:48 jacob Exp $ */
/*
   
   xscorch - sexplosion-gtk.c Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/
    
   Window painting code, specific to the explosion cache
   I can't believe I wrote this mess...
    

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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <sgtk.h>
#include <sdisplay.h>

#include <scolor-gtk.h>
#include <sexplosion-gtk.h>
#include <smenu-gtk.h>
#include <swindow-gtk.h>

#include <sgame/sland.h>
#include <sgame/splayer.h>
#include <sgame/sweapon.h>
#include <sutil/sfractal.h>



/*

   Congratulations!
      
   You are about to be treated to some pretty repulsive-looking code! 
   You have to understand, the comments are quite intentionally misleading 
   All of them are.
      
   You really are better off reading this file, after writing a mangler
   that strips out all the comments (better keep that copyright, `tho). 
   Then you can attain "understanding"
   The comments will only hamper.
      
   Comprehension is insanity.
   First, question all boundaries.
   This code defies the laws of gravity.
   Keep your hands/feet/etc inside the vehicle at all times.
   Reading this code can be detrimintal to your health.
   Abandon all hope, ye who enter here.
   Batteries not included.
   Good luck.
      
*/



sc_expl_cache_gtk *sc_expl_cache_new_gtk(void) {
/* sc_expl_cache_new_gtk
   Creates a new explosion cache.  */
   
   sc_expl_cache_gtk *cache;  /* Newly allocated cache */
   
   /* Create the cache */
   cache = (sc_expl_cache_gtk *)malloc(sizeof(sc_expl_cache_gtk));
   if(cache == NULL) return(NULL);
   
   /* Initialise the cache */
   cache->fakebitmap = gdk_pixmap_new(NULL, 1, 1, 1);
   cache->bitmapgc = gdk_gc_new(cache->fakebitmap);
   cache->cachesize = 0;
   cache->headptr = 0;
   
   /* Return the cache */
   return(cache);
   
}



void sc_expl_cache_free_gtk(sc_expl_cache_gtk **cache) {
/* sc_expl_cache_free_gtk
   Release the cache, and associated data.  */
   
   int i;            /* Iterator variable */
   
   /* Sanity check. */
   if(cache == NULL || *cache == NULL) return;
   
   /* Release pixmaps and bitmaps in cache */
   for(i = 0; i < (*cache)->cachesize; ++i) {
      g_object_unref((*cache)->cache[(*cache)->headptr].pixmap);
      ++(*cache)->headptr;
      if((*cache)->headptr >= SC_EXPL_CACHE_SIZE) {
         (*cache)->headptr = 0;
      }
   } /* Releasing all pixmaps, bitmaps... */
   
   /* Release the GC */
   g_object_unref((*cache)->bitmapgc);
   g_object_unref((*cache)->fakebitmap);
   
   /* Release the cache memory */
   free(*cache);
   *cache = NULL;
   
}



static inline void _sc_expl_cache_draw_points_gtk(GdkImage *image, guint32 pixel, 
                                                  int dx, int dy, int radius) {
/* sc_expl_cache_draw_points_gtk
   Draw the points (cx +/- dx, cy +/- dy), using the GC's specified.  */
   
   /* Iterate through the four quadrants to update display */
   gdk_image_put_pixel(image, radius - dx, radius - dy, pixel);
   gdk_image_put_pixel(image, radius - dx, radius + dy, pixel);
   gdk_image_put_pixel(image, radius + dx, radius - dy, pixel);
   gdk_image_put_pixel(image, radius + dx, radius + dy, pixel);
   
}



static inline void _sc_expl_cache_annihilate_column_rad2_gtk(GdkImage *image, GdkColor *gradient, 
                                                             int gsize, int dx, int dy, 
                                                             int radius, int rad2) {
/* sc_expl_cache_annihilate_column_rad2_gtk
   Annihilate a column of an explosion.  (cx,cy) is the center coordinate of
   the explosion.  (dx,dy) is the deltax and deltay that this column begins
   at -- that is, the column goes from (cx+dx,cy-dy) to (cx+dx,cy+dy).  rad2
   is the radius, squared, of the entire explosion.  Note, colors fall off 
   with the square of the radius; this makes for more "3-D"istic explosions,
   and also happens to be much faster to calculate.  */

   int dx2y2;        /* dx^2 * y^2, for current value of y */
   int tile;         /* Tile value for explosion */
   int y;            /* Current y (offset from cy, y in [0,dy]) */

   /* Iteration for all four columns */
   y = 0;            /* We are starting on the Y axis */
   dx2y2 = dx * dx;  /* This starts off as x^2, since y=0 */
   do {
      /* Calculate tile code -- any way to optimize this? */
      tile = dx2y2 * gsize / rad2;

      /* Adjust dx2y2 for next value of y */
      /* x^2 + y^2 => x^2 + y^2 + y + (y+1) 
                    = x^2 + y(y+1) + (y+1) 
                    = x^2 + (y+1)^2          */
      /* Suggested by JJP */
      dx2y2 += (y << 1) | 1;

      /* Iterate through the four quadrants to update display */
      _sc_expl_cache_draw_points_gtk(image, gradient[tile].pixel, dx, y, radius);

      /* Next value of y (one step farther from central axis) */
      ++y;
   } while(y <= dy);

}



static inline void _sc_expl_cache_annihilate_column_rad_gtk(GdkImage *image, GdkColor *gradient,
                                                            int gsize, int dx, int dy, int radius) {
/* sc_expl_cache_annihilate_column_rad_gtk
   Annihilate a column of an explosion.  (cx,cy) is the center coordinate of
   the explosion.  (dx,dy) is the deltax and deltay that this column begins
   at -- that is, the column goes from (cx+dx,cy-dy) to (cx+dx,cy+dy).  rad2
   is the radius, squared, of the entire explosion.  Note, colors fall off
   linearly with radius, which is used by funky bombs.  This is slower to
   calculate, I would not recommend using this routine with very large
   explosions. */

   /*
      Life... Dreams... Hope...
      Where'd they come from...
      And where are they headed?
      These things... I am going to destroy!
         -- Kefka, Fourth Tier, Final Fantasy VI
    */

   int dx2y2;        /* dx^2 * y^2, for current value of y */
   int tile;         /* Tile value for explosion */
   int y;            /* Current y (offset from cy, y in [0,dy]) */

   /* Iteration for all four columns */
   y = 0;            /* We are starting on the Y axis */
   dx2y2 = dx * dx;  /* This starts off as x^2, since y=0 */
   do {
      /* Calculate tile code -- any way to optimize this? */
      tile = (int)(sqrt(dx2y2) * gsize / radius);

      /* Adjust dx2y2 for next value of y */
      /* x^2 + y^2 => x^2 + y^2 + y + (y+1) 
                    = x^2 + y(y+1) + (y+1) 
                    = x^2 + (y+1)^2          */
      /* Suggested by JJP */
      dx2y2 += (y << 1) | 1;

      /* Iterate through the four quadrants to update display */
      _sc_expl_cache_draw_points_gtk(image, gradient[tile].pixel, dx, y, radius);

      /* Next value of y (one step farther from central axis) */
      ++y;
   } while(y <= dy);

}



static inline void _sc_expl_cache_annihilate_column_plasmoid_gtk(GdkImage *image, GdkColor *gradient,
                                                                 int gsize, int dx, int dy, int radius,
                                                                 unsigned char *fractal, int fsize) {
/* sc_expl_cache_annihilate_column_plasmoid_gtk
   Annihilate a column of an explosion.  (cx,cy) is the center coordinate of
   the explosion.  (dx,dy) is the deltax and deltay that this column begins
   at -- that is, the column goes from (cx+dx,cy-dy) to (cx+dx,cy+dy).  rad2
   is the radius, squared, of the entire explosion.  Note, colors are selected
   as a plasmoid, whatever that is.  */

   int dx2y2;        /* dx^2 * y^2, for current value of y */
   int tile;         /* Tile value for explosion */
   int y;            /* Current y (offset from cy, y in [0,dy]) */
   int radminusdx;   /* Value of radius - dx */
   int radplusdx;    /* Value of radius + dx */

   /* Precompute as much as possible. */
   radminusdx = radius - dx;
   radplusdx  = radius + dx;

   /* Iteration for all four columns */
   y = 0;            /* We are starting on the Y axis */
   dx2y2 = dx * dx;  /* This starts off as x^2, since y=0 */
   do {
      /* Warning: the division must remain inlined with the rest of the
         computation; it cannot be lifted out since this is integer-level
         arithmetic. */
   
      /* Calculate tile code (quadrant 1) */
      tile = *(fractal + (radius + y) * fsize + radplusdx)  * gsize / 0x100;
      gdk_image_put_pixel(image, radplusdx,  radius + y, gradient[tile].pixel);

      /* Calculate tile code (quadrant 2) */
      tile = *(fractal + (radius + y) * fsize + radminusdx) * gsize / 0x100;
      gdk_image_put_pixel(image, radminusdx, radius + y, gradient[tile].pixel);

      /* Calculate tile code (quadrant 3) */
      tile = *(fractal + (radius - y) * fsize + radminusdx) * gsize / 0x100;
      gdk_image_put_pixel(image, radminusdx, radius - y, gradient[tile].pixel);

      /* Calculate tile code (quadrant 4) */
      tile = *(fractal + (radius - y) * fsize + radplusdx)  * gsize / 0x100;
      gdk_image_put_pixel(image, radplusdx,  radius - y, gradient[tile].pixel);
   
      /* Adjust dx2y2 for next value of y */
      /* Suggested by JJP */
      dx2y2 += (y << 1) | 1;

      /* Next value of y (one step farther from central axis) */
      ++y;
   } while(y <= dy);

}



static void _sc_expl_cache_annihilate_rad2_gtk(GdkImage *image, GdkColor *gradient, 
                                               int gsize, int radius) {
/* sc_expl_annihilate_rad2_gtk
   Annihilate a region centered at (cx,cy) for a radius r.  This function
   lets colors fall off with the square of the radius.  */

   int dx;           /* Delta X (distance away from cx) - iterator variable */
   int dy;           /* Delta Y (distance away from cy) for _edge_ of circle */
   int rad2;         /* Radius squared */
   int rad2major2;   /* Radius^2 + the major_distance^2 */
   int min2thresh;   /* Minimum threshold to avoid redrawing columns where dx>dy */

   /* DX = major axis, DY = minor axis */
   dx = 0;           /* DX starts at zero (iterator) */
   dy = radius;      /* DY is one radius away (edge of circle at cx+dx) */
   rad2 = radius * radius; /* Calculate Radius Squared */
   rad2major2 = rad2;/* Radius^2 + major^2, running total */
   min2thresh = rad2 - dy; /* Minimum threshold before need to redraw edges */
   
   /* TEMP HACK */
   /* This is a hack to prevent the orange highlights on explosions. */
   /* THIS IS A HACK -- FIXME FOR REAL. */
   rad2 += radius + radius + 1;
   
   /* Should know that, we are incrementing DX every time.  However,
      if we call the transpose method every time as well, then we will
      be filling parts of the circle multiple times.  Hence the 
      min2thresh variable. */
   do {
      _sc_expl_cache_annihilate_column_rad2_gtk(image, gradient, gsize, 
                                                dx, dy, radius, rad2);
      ++dx;
      rad2major2 -= (dx << 1) - 1;
      if(rad2major2 <= min2thresh) {
         _sc_expl_cache_annihilate_column_rad2_gtk(image, gradient, gsize, 
                                                   dy, dx, radius, rad2);
         --dy;
         min2thresh -= (dy << 1);
      }
   } while(dx <= dy);
   
}



static void _sc_expl_cache_annihilate_rad_gtk(GdkImage *image, GdkColor *gradient, 
                                              int gsize, int radius) {
/* sc_expl_annihilate_rad_gtk
   Annihilate a region centered at (cx,cy) for a radius r.  This function
   lets colors fall off with the square of the radius.  */

   int dx;           /* Delta X (distance away from cx) - iterator variable */
   int dy;           /* Delta Y (distance away from cy) for _edge_ of circle */
   int rad2;         /* Radius squared */
   int rad2major2;   /* Radius^2 + the major_distance^2 */
   int min2thresh;   /* Minimum threshold to avoid redrawing columns where dx>dy */
   
   /* DX = major axis, DY = minor axis */
   dx = 0;           /* DX starts at zero (iterator) */
   dy = radius;      /* DY is one radius away (edge of circle at cx+dx) */
   rad2 = radius * radius; /* Calculate Radius Squared */
   rad2major2 = rad2;/* Radius^2 + major^2, running total */
   min2thresh = rad2 - dy; /* Minimum threshold before need to redraw edges */
   
   /* Should know that, we are incrementing DX every time.  However,
      if we call the transpose method every time as well, then we will
      be filling parts of the circle multiple times.  Hence the 
      min2thresh variable. */
   do {
      _sc_expl_cache_annihilate_column_rad_gtk(image, gradient, gsize, 
                                               dx, dy, radius);
      ++dx;
      rad2major2 -= (dx << 1) - 1;
      if(rad2major2 <= min2thresh) {
         _sc_expl_cache_annihilate_column_rad_gtk(image, gradient, gsize, 
                                                  dy, dx, radius);
         --dy;
         min2thresh -= (dy << 1);
      }
   } while(dx <= dy);
   
}



static void _sc_expl_cache_annihilate_plasmoid_gtk(GdkImage *image, GdkColor *gradient,
                                                   int gsize, int radius) {
/* sc_expl_annihilate_plasmoid_gtk
   Annihilate a region centered at (cx,cy) for a radius r.  This function
   selects colors to form a plasma manifold pattern, whatever that means.  */

   int dx;           /* Delta X (distance away from cx) - iterator variable */
   int dy;           /* Delta Y (distance away from cy) for _edge_ of circle */
   int rad2;         /* Radius squared */
   int rad2major2;   /* Radius^2 + the major_distance^2 */
   int min2thresh;   /* Minimum threshold to avoid redrawing columns where dx>dy */
   unsigned char *fractal;     /* */
   int fsize;

   /* Allocate a new fractal. */
   fractal = sc_fractal_create(&fsize, radius + radius + 1);
   if(fractal == NULL) {
      _sc_expl_cache_annihilate_rad2_gtk(image, gradient, gsize, radius);
      return;
   }
   
   /* DX = major axis, DY = minor axis */
   dx = 0;           /* DX starts at zero (iterator) */
   dy = radius;      /* DY is one radius away (edge of circle at cx+dx) */
   rad2 = radius * radius; /* Calculate Radius Squared */
   rad2major2 = rad2;/* Radius^2 + major^2, running total */
   min2thresh = rad2 - dy; /* Minimum threshold before need to redraw edges */
   
   /* Should know that, we are incrementing DX every time.  However,
      if we call the transpose method every time as well, then we will
      be filling parts of the circle multiple times.  Hence the 
      min2thresh variable. */
   do {
      _sc_expl_cache_annihilate_column_plasmoid_gtk(image, gradient, gsize, 
                                                    dx, dy, radius, fractal, fsize);
      ++dx;
      rad2major2 -= (dx << 1) - 1;
      if(rad2major2 <= min2thresh) {
         _sc_expl_cache_annihilate_column_plasmoid_gtk(image, gradient, gsize, 
                                                       dy, dx, radius, fractal, fsize);
         --dy;
         min2thresh -= (dy << 1);
      }
   } while(dx <= dy);
   
   free(fractal);
   
}



static inline void _sc_expl_cache_annihilate_happy_gtk(sc_window_gtk *w, GdkPixmap *pixmap, 
                                                       int radius) {
/* sc_expl_cache_annihilate_happy_gtk
   It's not a bug... it's a ``feature''...  I wonder if anyone has actually
   found this egg in practice.  It's quite fun once it's activated :)  */

   int size = radius + radius + 1;
   int eyesize = (radius >> 2) + (radius >> 4);
   int eyecoff = radius >> 2;
   GdkGC *gc = gdk_gc_new(pixmap);

   gdk_gc_set_foreground(gc, &w->colormap->yellow);
   gdk_draw_arc(pixmap, gc, TRUE, 0, 0, size, size, 0, 360 * 64);
   gdk_gc_set_foreground(gc, &w->colormap->black);
   gdk_gc_set_line_attributes(gc, 3, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
   gdk_draw_arc(pixmap, gc, FALSE, 0, 0, size, size, 0, 360 * 64);
   gdk_draw_arc(pixmap, gc, TRUE, 
                radius - eyecoff - eyesize, radius - eyecoff - eyesize, 
                eyesize, eyesize, 0 * 64, 360 * 64);
   gdk_draw_arc(pixmap, gc, TRUE, 
                radius + eyecoff, radius - eyecoff - eyesize, 
                eyesize, eyesize, 0 * 64, 360 * 64);
   gdk_draw_arc(pixmap, gc, FALSE,
                radius - eyecoff - eyesize, radius + (eyecoff >> 1),
                (eyecoff + eyesize) << 1, eyesize << 1, 200 * 64, 140 * 64);
   g_object_unref(gc);

}



static inline void _sc_expl_cache_annihilate_gtk(sc_window_gtk *w, GdkPixmap *pixmap, 
                                                 int radius, sc_explosion_type type) {
/* sc_expl_annihilate_rad_gtk
   Annihilate a region centered at (cx,cy) for a radius r.  This function
   uses the type parameter to dispatch to the appropriate drawing
   subfunction.  */

   GdkImage *image;
   int size;

   /* ...wonder why this is here... :) */
   if(w->state < 4) {
      /* Create images */
      size = radius + radius + 1;
      image = gdk_image_new(GDK_IMAGE_FASTEST, gtk_widget_get_visual(w->app), size, size);
      if(image == NULL) return;
      
      /* Draw the appropriate type of explosion. */
      switch(type) {
         case SC_EXPLOSION_NORMAL:
            _sc_expl_cache_annihilate_rad2_gtk(image, w->colormap->gradient[SC_GRAD_EXPLOSION], 
                                               w->c->colors->gradsize[SC_GRAD_EXPLOSION], radius);
            break;
         case SC_EXPLOSION_PLASMA:
            _sc_expl_cache_annihilate_plasmoid_gtk(image, w->colormap->gradient[SC_GRAD_EXPLOSION], 
                                                   w->c->colors->gradsize[SC_GRAD_EXPLOSION], radius);
            break;
         case SC_EXPLOSION_SPIDER:
            _sc_expl_cache_annihilate_rad_gtk(image, w->colormap->gradient[SC_GRAD_FUNKY_EXPLOSION],
                                              w->c->colors->gradsize[SC_GRAD_FUNKY_EXPLOSION], radius);
            break;
         default:
            /* do nothing */;
      } /* Which type of explosion to draw? */

      /* Commit data to pixmap/bitmap buffers */
      gdk_draw_image(pixmap, sc_display_get_gc(SC_DISPLAY(w->screen)), 
                     image, 0, 0, 0, 0, size, size);
      
      /* Destroy the local images */
      g_object_unref(image);
   } else {
      _sc_expl_cache_annihilate_happy_gtk(w, pixmap, radius);
   }

}



static int _sc_expl_cache_lookup_gtk(sc_window_gtk *w, int radius, sc_explosion_type type) {
/* sc_expl_cache_lookup_gtk
   Lookup an entry in the cache matching this description.
   If no such entry is found, -1 is returned.  */

   sc_expl_cache_gtk *cache = w->explcache;  /* Cache */
   int size;         /* Size of cache */
   int ptr;          /* Current ptr into cache */

   /* Determine size and starting pointer */   
   size = cache->cachesize;
   ptr = cache->headptr;
   
   /* Try to find an explosion matching these characteristics */
   while(size > 0) {
      /* Does this explosion match? */
      if(cache->cache[ptr].radius == radius && cache->cache[ptr].type == type) {
         /* Match found; return its cacheid */
         return(ptr);
      } /* Found a match? */
      
      /* Not a match; advance to next entry in the cache. */
      ++ptr;
      if(ptr >= SC_EXPL_CACHE_SIZE) ptr = 0;
      --size;
   } /* Searching for a match... */

   /* No matching explosion was found */
   return(-1);

}



int sc_expl_cache_new(sc_window *w_, int radius, sc_explosion_type type) {
/* sc_expl_cache_new
   Creates a new explosion with these characteristics, and returns its cache
   ID.  Note, if an explosion of these characteristics already exists in the
   cache, then its cache ID will be returned, and no new explosion will be
   created.  */

   sc_window_gtk *w = (sc_window_gtk *)w_;   /* Window structure */
   sc_expl_cache_gtk *cache =  w->explcache; /* Cache structure */
   sc_expl_cache_entry_gtk *centry; /* An entry in the cache. */
   int cacheid;      /* Cache ID of a matching explosion */
   int size;         /* Size (wid/hei) of the pixmap to create. */

   /* Try to find this explosion in the cache. */      
   cacheid = _sc_expl_cache_lookup_gtk(w, radius, type);
   if(cacheid < 0) {
      /* No matching explosion found; overwrite the oldest
         entry in the cache with our new explosion.  */
      /* TEMP BUG:  We really shouldn't overwrite the OLDEST
         entry; we'd get better performance if we overwrite
         the LEAST-RECENTLY-USED entry.  Need to fix this... */
      --cache->headptr;
      if(cache->headptr < 0) cache->headptr = SC_EXPL_CACHE_SIZE - 1;
      cacheid = cache->headptr;

      /* Get the entry for the explosion we are overwriting */
      centry = &cache->cache[cacheid];
      if(cache->cachesize < SC_EXPL_CACHE_SIZE) {
         /* Cache wasn't full; this entry is vacant. */
         ++cache->cachesize;
      } else {
         /* Cache was full; need to release the old pixmaps */
         g_object_unref(centry->pixmap);
      } /* Was cache already full? */

      /* Create the new pixmap and bitmap, and set characteristics */
      size = radius + radius + 1;   
      centry->pixmap = gdk_pixmap_new(w->app->window, size, size, -1);
      centry->radius = radius;
      centry->type   = type;

      /* Draw an explosion into the new pixmap/bitmap. */
      _sc_expl_cache_annihilate_gtk(w, centry->pixmap, radius, type);
   } /* Did explosion already exist in cache? */

   /* Return the cache ID of our new explosion */   
   return(cacheid);

}



void sc_expl_cache_draw(sc_window *w_, int ptr, int centerx, int centery, int rad) {
/* sc_expl_cache_draw
   Draws the explosion to the screen buffer.  */

   GdkColor black = { 0, 0x0000, 0x0000, 0x0000 };
   GdkColor white = { 1, 0xffff, 0xffff, 0xffff };
   sc_window_gtk *w = (sc_window_gtk *)w_;/* Window structure */
   sc_expl_cache_entry_gtk *centry = &(w->explcache->cache[ptr]);
   GdkPixmap *mask;                       /* Mask to use */
   int radius;                            /* Radius of explosion */
   int size;                              /* Size (wid/hei) of expl */

   /* Calculate the screen center coordinates */
   radius = centry->radius;
   centerx = centerx;
   centery = w->c->fieldheight - centery - 1;
   if(rad > radius) rad = radius;
   size = rad + rad + 1;
   
   /* Create a new temporary mask */
   mask = gdk_pixmap_new(NULL, size, size, 1);
   if(mask != NULL) {
      gdk_gc_set_foreground(w->explcache->bitmapgc, &black);
      gdk_draw_rectangle(mask, w->explcache->bitmapgc, TRUE, 
                         0, 0, size, size);
      gdk_gc_set_foreground(w->explcache->bitmapgc, &white);
      gdk_draw_arc(mask, w->explcache->bitmapgc, TRUE, 
                   0, 0, size, size, 0, 360 * 64);
   }
         
   /* Draw the explosion to the screen buffer */
   gdk_gc_set_clip_mask(sc_display_get_gc(SC_DISPLAY(w->screen)), mask);
   gdk_gc_set_clip_origin(sc_display_get_gc(SC_DISPLAY(w->screen)), 
                          centerx - rad, centery - rad);
   gdk_draw_drawable(sc_display_get_buffer(SC_DISPLAY(w->screen)),
                     sc_display_get_gc(SC_DISPLAY(w->screen)),
                     centry->pixmap,
                     radius - rad, radius - rad,
                     centerx - rad, centery - rad,
                     size, size);
   gdk_gc_set_clip_mask(sc_display_get_gc(SC_DISPLAY(w->screen)), NULL);

   /* Update drawing */   
   sc_display_queue_draw(SC_DISPLAY(w->screen),
                         centerx - rad, centery - rad,
                         size, size);
   
   /* Uninstall the bitmap */
   if(mask != NULL) {
      g_object_unref(mask);
   }
   
}
