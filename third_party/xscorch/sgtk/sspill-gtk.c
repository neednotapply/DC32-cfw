/* $Header: /fridge/cvs/xscorch/sgtk/sspill-gtk.c,v 1.9 2009-04-26 17:39:50 jacob Exp $ */
/*

   xscorch - sspill-gtk.c     Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Window painting code, specific to "spill" explosions like napalm


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
#include <sgtk.h>
#include <sdisplay.h>

#include <scolor-gtk.h>
#include <swindow-gtk.h>

#include <sgame/sconfig.h>
#include <sgame/sland.h>
#include <sgame/swindow.h>
#include <sutil/srand.h>

#include <string.h>



#define  SC_NAPALM_FLAME_RAD     16
#define  SC_NAPALM_FLAME_PROB    0.015



static int *_sc_window_height_map_gtk(const sc_config *c, const int *xlist,
                                      const int *ylist, int size) {
/* sc_window_height_map_gtk
   Creates a heightmap for the list of coordinates, given.  If there is no
   data point for a particular value of x, the cooresponding height will be
   set to -1.  On success, a structure is returned which must be released
   manually, using free().  */

   const int *pxlist;      /* Pointer into X coord array */
   const int *pylist;      /* Pointer into Y coord array */
   int *heightmap;         /* Data structure storing hmap */
   int *pmap;              /* Pointer into the map */
   int x;                  /* X coordinate */
   int y;                  /* Y coordiante */
   int i;                  /* Iterator */

   /* Create storage for the heightmap */
   heightmap = (int *)malloc(sizeof(int) * c->fieldwidth);
   if(heightmap == NULL) return(NULL);

   /* Initialise the heightmap */
   pmap = heightmap;
   for(x = 0; x < c->fieldwidth; ++x) {
      *(pmap++) = -1;
   } /* Initialising map array */

   /* Construct datavalues for the heightmap, as needed */
   pxlist = xlist;
   pylist = ylist;
   for(i = 0; i < size; ++i) {
      /* Get the raw X,Y coordinates */
      x = *(pxlist++);
      y = *(pylist++);

      /* Translate to `proper' screen coordinates, if possible. */
      if(sc_land_translate_xy(c->land, &x, &y)) {
         /* Was this point higher than previously known height? */
         if(heightmap[x] < y) heightmap[x] = y;
      }
   }

   /* Successful return */
   return(heightmap);

}



static int *_sc_window_flame_map_gtk(const sc_config *c) {
/* sc_window_flame_map_gtk */

   double dist;
   int *flamemap;
   int maxdelta;
   int effect;
   int bigrad;
   int delta;
   int theta;
   int size;
   int rad;
   int max;
   int idx;
   int x;
   int y;

   size = SC_NAPALM_FLAME_RAD * 2 + 1;
   flamemap = (int *)malloc(sizeof(int) * SQR(size));
   if(flamemap == NULL) return(NULL);

   memset((char *)flamemap, 0xff, sizeof(int) * SQR(size));

   max = c->colors->gradsize[SC_GRAD_FLAMES] - 1;

   rad = SC_NAPALM_FLAME_RAD / 2 + sys_lrand(SC_NAPALM_FLAME_RAD / 2) + 1;
   theta = 60;

   bigrad = rad / sin(theta * M_PI / 180);
   maxdelta = (1 - cos(theta * M_PI / 180)) * bigrad;

   effect = 0;
   for(y = -rad; y < rad; ++y) {
      delta = sqrt(SQR(bigrad) - SQR(y)) - cos(theta * M_PI / 180) * bigrad;
      if(effect - delta < -SC_NAPALM_FLAME_RAD) effect = -SC_NAPALM_FLAME_RAD + delta;
      if(effect + delta >  SC_NAPALM_FLAME_RAD) effect =  SC_NAPALM_FLAME_RAD - delta;
      for(x = -delta; x < delta; ++x) {
         idx = (y + rad) * size + x + effect + SC_NAPALM_FLAME_RAD;
         dist = sqrt(SQR((double)x / maxdelta) + SQR((double)(y + rad) / (2 * rad)));
         *(flamemap + idx) = max - dist * max / M_SQRT2;
      }
      switch(sys_lrand(5)) {
         case 0:  ++effect; break;
         case 1:  --effect; break;
      }
   }

   return(flamemap);

}



static void _sc_window_draw_flames_gtk(sc_window_gtk *w, const int *xlist,
                                       const int *ylist, int size) {
/* sc_window_draw_flames_gtk */

   GdkPixmap *buffer;
   GdkGC *gc;
   int *flamemap;
   int *heights;
   int *heightp;
   int boundx1;
   int boundy1;
   int boundx2;
   int boundy2;
   int height;
   int px;
   int py;
   int x;
   int y;

   heights = _sc_window_height_map_gtk(w->c, xlist, ylist, size);
   if(heights == NULL) return;

   height = w->c->fieldheight;
   buffer = sc_display_get_buffer(SC_DISPLAY(w->screen));
   gc = sc_display_get_gc(SC_DISPLAY(w->screen));
   gdk_gc_set_foreground(gc, &w->colormap->napalm);

   boundx1 = w->c->fieldwidth;
   boundx2 = w->c->fieldheight;
   boundy1 = 0;
   boundy2 = 0;

   for(heightp = heights, x = 0; x < w->c->fieldwidth; ++heightp, ++x) {
      y = *heightp;
      if(y >= 0 && sys_drand() < SC_NAPALM_FLAME_PROB) {
         flamemap = _sc_window_flame_map_gtk(w->c);
         if(flamemap != NULL) {
            for(py = 0; py < SC_NAPALM_FLAME_RAD * 2 + 1; ++py) {
               for(px = 0; px < SC_NAPALM_FLAME_RAD * 2 + 1; ++px) {
                  if(*(flamemap + py * (SC_NAPALM_FLAME_RAD * 2 + 1) + px) >= 0) {
                     gdk_gc_set_foreground(gc, &w->colormap->gradient[SC_GRAD_FLAMES][*(flamemap + py * (SC_NAPALM_FLAME_RAD * 2 + 1) + px)]);
                     gdk_draw_point(buffer, gc, x + px - SC_NAPALM_FLAME_RAD, height - y - py);
                  }
               }
            }
            free(flamemap);
         }
         if(x - SC_NAPALM_FLAME_RAD < boundx1) boundx1 = x - SC_NAPALM_FLAME_RAD;
         if(x + SC_NAPALM_FLAME_RAD > boundx2) boundx2 = x + SC_NAPALM_FLAME_RAD;
         if(y                           < boundy1) boundy1 = y;
         if(y + 2 * SC_NAPALM_FLAME_RAD > boundy2) boundy2 = y + 2 * SC_NAPALM_FLAME_RAD;
      }
   }

   free(heights);

   sc_display_queue_draw(SC_DISPLAY(w->screen),
                         boundx1, height - boundy2 - 1,
                         boundx2 - boundx1 + 1, boundy2 - boundy1 + 1);

}



void sc_window_draw_napalm_frame(sc_window *w_, const int *xlist,
                                 const int *ylist, int size) {
/* sc_window_draw_napalm_frame */

   sc_window_gtk *w = (sc_window_gtk *)w_;
   GdkPixmap *buffer;
   GdkGC *gc;
   int boundx1;
   int boundy1;
   int boundx2;
   int boundy2;
   int height;
   int x;
   int y;

   if(xlist == NULL || ylist == NULL || w_ == NULL) return;

   boundx1 = w->c->fieldwidth;
   boundx2 = w->c->fieldheight;
   boundy1 = 0;
   boundy2 = 0;

   height = w->c->fieldheight;
   buffer = sc_display_get_buffer(SC_DISPLAY(w->screen));
   gc = sc_display_get_gc(SC_DISPLAY(w->screen));
   gdk_gc_set_foreground(gc, &w->colormap->napalm);

   while(size > 0) {
      x = *xlist;
      y = *ylist;
      if(sc_land_translate_xy(w->c->land, &x, &y)) {
         gdk_draw_point(buffer, gc, x, height - y - 1);
         if(*xlist < boundx1) boundx1 = x;
         if(*xlist > boundx2) boundx2 = x;
         if(*ylist < boundy1) boundy1 = y;
         if(*ylist > boundy2) boundy2 = y;
      }
      ++xlist;
      ++ylist;
      --size;
   }

   sc_display_queue_draw(SC_DISPLAY(w->screen),
                         boundx1, height - boundy2 - 1,
                         boundx2 - boundx1 + 1, boundy2 - boundy1 + 1);

}



void sc_window_draw_napalm_final(sc_window *w_, const int *xlist,
                                 const int *ylist, int totalsize) {
/* sc_window_draw_napalm_final */

   if(xlist == NULL || ylist == NULL || w_ == NULL) return;
   _sc_window_draw_flames_gtk((sc_window_gtk *)w_, xlist, ylist, totalsize);

}



void sc_window_clear_napalm(sc_window *w_, const int *xlist,
                            const int *ylist, int totalsize) {
/* sc_window_clear_napalm */

   sc_window_gtk *w = (sc_window_gtk *)w_;
   int boundx1;
   int boundy1;
   int boundx2;
   int boundy2;

   if(xlist == NULL || ylist == NULL || w_ == NULL) return;

   boundx1 = w->c->fieldwidth;
   boundx2 = w->c->fieldheight;
   boundy1 = 0;
   boundy2 = 0;

   while(totalsize > 0) {
      if(*xlist < boundx1) boundx1 = *xlist;
      if(*xlist > boundx2) boundx2 = *xlist;
      if(*ylist < boundy1) boundy1 = *ylist;
      if(*ylist > boundy2) boundy2 = *ylist;
      ++xlist;
      ++ylist;
      --totalsize;
   }

   boundx1 -= SC_NAPALM_FLAME_RAD;
   boundx2 += SC_NAPALM_FLAME_RAD;
   boundy2 += 2 * SC_NAPALM_FLAME_RAD;

   sc_window_paint(w_, boundx1, boundy1, boundx2, boundy2,
                   SC_REDRAW_LAND | SC_REDRAW_TANKS);

}
