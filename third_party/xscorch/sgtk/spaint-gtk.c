/* $Header: /fridge/cvs/xscorch/sgtk/spaint-gtk.c,v 1.18 2011-04-15 06:04:25 jacob Exp $ */
/*
   
   xscorch - spain-gtk.c      Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/
    
   Window painting code for scorch
    

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
#include <sconsole.h>
#include <sdisplay.h>

#include <scolor-gtk.h>
#include <simage-gtk.h>
#include <smenu-gtk.h>
#include <swindow-gtk.h>

#include <sgame/saccessory.h>
#include <sgame/sland.h>
#include <sgame/sphysics.h>
#include <sgame/splayer.h>
#include <sgame/sshield.h>
#include <sgame/stankpro.h>
#include <sgame/sweapon.h>



#define  USE_PIXMAP_MAX_AREA     400   /* If redraw area is less than this, draw w/ pixmap code */
#define  SC_WIND_MINIMUM_ARROW   5     /* Minimum wind arrow size for it to be drawn at all */
#define  SC_WIND_MAXIMUM_ARROW   100   /* Maximum wind arrow size - will truncate if larger */
#define  SC_WIND_ARROW_MARGIN    10    /* Padding around arrow in corner of screen */
#define  SC_WIND_ARROW_SIZE      4     /* Size of the arrowhead (in pixels) */



static inline bool _sc_lines_overlap_gtk(int a1, int a2, int b1, int b2) {
/* sc_lines_overlap_gtk */

   /* Check if line segment (a1,a2) overlaps (b1,b2) */
   return(b1 <= a2 && a1 <= b2);

}



static inline bool _sc_rects_overlap_gtk(int ax1, int ay1, int ax2, int ay2,
                                         int bx1, int by1, int bx2, int by2) {
/* sc_rects_overlap_gtk */

   /* Check if rectangle A overlaps any part of rectanble B. */
   return(_sc_lines_overlap_gtk(ax1, ax2, bx1, bx2) && 
          _sc_lines_overlap_gtk(ay1, ay2, by1, by2));

}



static void _sc_window_draw_arrow(GdkPixmap *buffer, GdkGC *gc, int x, int y,
                                  int size, bool right) {
/* sc_window_draw_arrow
   Draws an arrow on the screen whose RIGHT coordinate of the mainline
   is at (x, y), and whose mainline is <size> pixels long.  The <right>
   flag is true iff the arrow cap should be drawn on the right-hand
   side of the arrow.  The coordinates MUST be real coordinates.  */

   /* Draw the arrow mainline */
   gdk_draw_line(buffer, gc, x, y, x - size, y);

   /* Draw the arrow cap */
   if(right) {
      gdk_draw_line(buffer, gc, x, y,
                    x - SC_WIND_ARROW_SIZE, y - SC_WIND_ARROW_SIZE);
      gdk_draw_line(buffer, gc, x, y,
                    x - SC_WIND_ARROW_SIZE, y + SC_WIND_ARROW_SIZE);
   } else {
      gdk_draw_line(buffer, gc, x - size, y, 
                    x - size + SC_WIND_ARROW_SIZE, y - SC_WIND_ARROW_SIZE);
      gdk_draw_line(buffer, gc, x - size, y, 
                    x - size + SC_WIND_ARROW_SIZE, y + SC_WIND_ARROW_SIZE);
   }

}



static void _sc_window_draw_x(GdkPixmap *buffer, GdkGC *gc, int x, int y) {
/* sc_window_draw_x
   Used by arrow drawing code to draw an X whose right-center coordinate
   is at (x, y) -- this is used to indicate no wind is present.  The
   coordinates MUST be real coordiantes.  */

   /* Draw the X */
   gdk_draw_line(buffer, gc, x, y + SC_WIND_ARROW_SIZE,
                 x - SC_WIND_ARROW_SIZE - SC_WIND_ARROW_SIZE, y - SC_WIND_ARROW_SIZE);
   gdk_draw_line(buffer, gc, x, y - SC_WIND_ARROW_SIZE,
                 x - SC_WIND_ARROW_SIZE - SC_WIND_ARROW_SIZE, y + SC_WIND_ARROW_SIZE);

}



static void _sc_window_draw_wind_arrow(sc_window_gtk *w, bool drawing) {
/* sc_window_draw_wind_arrow
   Draws the wind arrow in the upper-right corner of the screen.  If
   drawing is set, we will draw an arrow; otherwise we will erase the
   last arrow that was drawn to the screen.  */

   GdkPixmap *buffer;         /* Screen buffer */
   GdkGC *gc;                 /* Screen gc */
   int size;                  /* Size of wind arrow */
   int x;                     /* Anchor X (screen coords) */
   int y;                     /* Anchor Y (screen coords) */

   /* Get the buffer and GC to use in drawing. */
   buffer = sc_display_get_buffer(SC_DISPLAY(w->screen));   
   gc = sc_display_get_gc(SC_DISPLAY(w->screen));

   /* Calculate the arrow size */
   if(drawing) {
      size = abs((int)(w->c->physics->curwind * 5000 / SC_PHYSICS_WIND_MAX));
   } else {
      size = w->windarrowsize;
   }
   x = w->c->fieldwidth - SC_WIND_ARROW_MARGIN;
   y = SC_WIND_ARROW_SIZE + SC_WIND_ARROW_MARGIN;
   if(size > SC_WIND_MAXIMUM_ARROW) size = SC_WIND_MAXIMUM_ARROW;

   /* Drawing or clearing? */
   if(drawing) {
      /* Update the current windarrow size */
      w->windarrowsize = size;

      if(size < SC_WIND_MINIMUM_ARROW) {
         size = SC_WIND_ARROW_SIZE + SC_WIND_ARROW_SIZE;

         /* Draw the SHADOW */
         gdk_gc_set_foreground(gc, &w->colormap->black);
         _sc_window_draw_x(buffer, gc, x + 1, y + 1);
         _sc_window_draw_x(buffer, gc, x + 1, y + 2);
      
         /* Draw the arrow mainline and cap */
         gdk_gc_set_foreground(gc, &w->colormap->windar);
         _sc_window_draw_x(buffer, gc, x, y);
      } else {
         /* Draw the arrow SHADOW */
         gdk_gc_set_foreground(gc, &w->colormap->black);
         _sc_window_draw_arrow(buffer, gc, x + 1, y + 1, size, w->c->physics->curwind >= 0);
         _sc_window_draw_arrow(buffer, gc, x + 1, y + 2, size, w->c->physics->curwind >= 0);
         
         /* Draw the arrow mainline and cap */
         gdk_gc_set_foreground(gc, &w->colormap->windar);
         _sc_window_draw_arrow(buffer, gc, x, y, size, w->c->physics->curwind >= 0);
      }
   } else {
      /* Clearing an existing arrow */
      if(size < SC_WIND_MINIMUM_ARROW) size = SC_WIND_ARROW_SIZE + SC_WIND_ARROW_SIZE;
      gdk_draw_drawable(buffer, gc, w->landbuffer, 
                        x - size, y - SC_WIND_ARROW_SIZE,
                        x - size, y - SC_WIND_ARROW_SIZE,
                        size + 3, SC_WIND_ARROW_SIZE + SC_WIND_ARROW_SIZE + 3);
   } /* Drawing or clearing? */
   
   /* Update the display */
   sc_display_queue_draw(SC_DISPLAY(w->screen), 
                         x - size, y - SC_WIND_ARROW_SIZE,
                         size + 3, SC_WIND_ARROW_SIZE + SC_WIND_ARROW_SIZE + 3);

}



static void _sc_window_draw_tank_gtk(sc_window_gtk *w, const sc_player *p) {
/* sc_window_draw_tank_gtk
   Draws the player's tank to the screen, as well as any shielding which
   might currently be equipped on the player.  */

   GdkPixmap *buffer;         /* Screen buffer */
   GdkGC *gc;                 /* Screen gc */
   sc_gradient_list grad;     /* Gradient for shields */
   unsigned char *data;       /* Profile data pointer */
   int radius;                /* Tank's radius */
   int index;                 /* Shield gradient index */
   int size;                  /* Size = radius * 2 + 1 */
   int tx1;                   /* Translate interval X1 */
   int tx2;                   /* Translate interval X2 */
   int cx;                    /* Current X coord for draw */
   int cy;                    /* Current Y coord for draw */
   int tx;                    /* Translated X coord */
   int ty;                    /* Translated Y corod */
   int x;                     /* Player X (screen coords) */
   int y;                     /* Player Y (screen coords) */

   /* Get the buffer and GC to use in drawing. */
   buffer = sc_display_get_buffer(SC_DISPLAY(w->screen));   
   gc = sc_display_get_gc(SC_DISPLAY(w->screen));

   /* Get current player coordinates. */
   x = p->x;
   y = w->c->fieldheight - p->y - 1;
   radius = p->tank->radius;
   size = radius + radius + 1;
      
   /* Draw tank shields (if available) */
   if(p->shield != NULL) {
      /* Determine which gradient to use */
      if(SC_ACCESSORY_SHIELD_IS_FORCE(p->shield->info)) {
         grad = SC_GRAD_FORCE;
      } else if(SC_ACCESSORY_SHIELD_IS_MAGNETIC(p->shield->info)) {
         grad = SC_GRAD_MAGNETIC;
      } else {
         grad = SC_GRAD_SHIELD;
      }

      /* Draw the shield */
      index = w->c->colors->gradsize[grad] * p->shield->life / (p->shield->info->shield + 1);
      gdk_gc_set_foreground(gc, &w->colormap->gradient[grad][index]);
      gdk_draw_arc(buffer, gc, FALSE, 
                   x - radius, y - radius, 
                   2 * radius + 1, 2 * radius + 1, 
                   0, 360 * 64);
      if(SC_SHIELD_IS_MEDIUM(p->shield) || SC_SHIELD_IS_STRONG(p->shield)) {
         gdk_draw_arc(buffer, gc, FALSE, 
                      x - radius - 1, y - radius - 1, 
                      2 * radius + 3, 2 * radius + 3, 
                      0, 360 * 64);
         if(SC_SHIELD_IS_STRONG(p->shield)) {
            gdk_draw_arc(buffer, gc, FALSE, 
                         x - radius - 2, y - radius - 2, 
                         2 * radius + 5, 2 * radius + 5, 
                         0, 360 * 64);
         } /* Extra shielding for strongest shields */
      } /* Extra shielding for stronger shields */
   }

   /* Set foreground pen color. */
   gdk_gc_set_foreground(gc, &w->colormap->pcolors[p->index]);

   /* Draw tank base and body. */
   data = p->tank->data;
   for(cy = -radius; cy <= 0; ++cy) {
      for(cx = -radius; cx <= radius; ++data, ++cx) {
         if(*data != SC_TANK_PROFILE_CLEAR) {
            tx = x + cx;
            ty = y + cy;
            if(sc_land_translate_x(w->c->land, &tx)) {
               gdk_draw_point(buffer, gc, tx, ty);
            } /* Is this coordinate on the screen? */
         } /* Is this point part of the tank? */
      } /* Iterate over X */
   } /* Iterate over Y */

   /* Draw tank's turret. */
   radius = p->tank->turretradius;
   gdk_draw_line(buffer, gc, 
                 x, y, 
                 (int)(x + rint(radius * cos(p->turret * M_PI / 180))), 
                 (int)(y - rint(radius * sin(p->turret * M_PI / 180))));

   /* Make sure all of this gets drawn. */
   tx1 = x - radius;
   tx2 = x + radius;
   if(sc_land_translate_x_range(w->c->land, &tx1, &tx2)) {
      sc_display_queue_draw(SC_DISPLAY(w->screen), 
                            tx1, y - radius, 
                            tx2 - tx1 + 1, size);
      if(sc_land_overlap_x(w->c->land, &tx1, &tx2)) {
         sc_display_queue_draw(SC_DISPLAY(w->screen), 
                               tx1, y - radius, 
                               tx2 - tx1 + 1, size);
      }
   }

}



static void _sc_window_draw_all_tanks_gtk(sc_window_gtk *w, int x1, int y1, int x2, int y2) {
/* sc_window_draw_all_tanks_gtk
   Draws all player tanks to the screen which are partially or entirely
   within the bounding box given by (x1, y1)-(x2, y2).  All coordinates
   given to this function MUST be real coordinates.  */

   sc_player *p;              /* Current Player pointer */
   int radius;                /* Radius for player */
   int x;                     /* Current player X (land coords) */
   int y;                     /* Current player Y (land coords) */
   int px1;                   /* Player X range left */
   int px2;                   /* Player X range right */
   int i;

   /* Iterate through all players. */
   i = w->c->numplayers - 1;
   while(i >= 0) {
      /* Get player info; make sure they're not dead. */
      p = w->c->players[i];
      if(!p->dead) {
         /* Get player coordinates; see if any part of the tank falls in redraw field. */
         x = p->x;
         y = p->y;
         radius = p->tank->turretradius;
         px1 = x - radius;
         px2 = x + radius;
         if(sc_land_translate_x_range(w->c->land, &px1, &px2)) {
            if(_sc_rects_overlap_gtk(x1, y1, x2, y2, px1, y - radius, px2, y + radius)) {
               _sc_window_draw_tank_gtk(w, p);
            } /* Is tank in redraw range? */
            /* Check for overlap range as well */
            if(sc_land_overlap_x(w->c->land, &px1, &px2)) {
               if(_sc_rects_overlap_gtk(x1, y1, x2, y2, px1, y - radius, px2, y + radius)) {
                  _sc_window_draw_tank_gtk(w, p);
               } /* Is tank in redraw range? */
            } /* Any overlap? */
         } /* Range translation successful? */
      } /* Is player not dead? */
      --i;
   } /* Loop through players. */
   
}



static void _sc_window_draw_land_pixmap_gtk(sc_window_gtk *w, int x1, int y1, int x2, int y2) {
/* sc_window_draw_land_pixmap_gtk
   This was the original drawing code, sending pixels at a time to the X
   server.  Note this is not at all efficient in that it wastes bandwidth
   and seriously slows us down for large area updates. See the next function
   for a faster approach.  However, if very small areas are being updated
   (say, 10x10 region) this method is still faster.  The bounding box MUST
   be in real coordinates.  */

   sc_color_gtk *colormap;    /* Colormap data */
   GdkPixmap *buffer;         /* Land pixmap buffer */
   GdkColor *lcolor;          /* Last-used color */
   GdkColor *color;           /* Current color */
   GdkGC *gc;                 /* Graphics context */
   const int *lpointer;       /* Pointer into land */
   int height;                /* Plotter height */
   int x;                     /* Current X (land coords) */
   int y;                     /* Current Y (land coords) */
   int y0;                    /* Y0 of the current line */

   /* Get the colormap data, pixmap, and graphics context */
   height = w->c->fieldheight - 1;
   colormap = w->colormap;
   buffer = w->landbuffer;
   gc = sc_display_get_gc(SC_DISPLAY(w->screen));   

   /* All points must be redrawn */
   lcolor = NULL;
   for(x = x1; x <= x2; ++x) {
      /* Get pointer to bottom of current column. */
      lpointer = SC_LAND_XY(w->c->land, x, y1);
      lcolor = NULL;
      y0 = -1;
      for(y = y1; y <= y2; ++y, ++lpointer) {   
         /* Iterating along the column ... */
         switch(SC_LAND_GET_TYPE(*lpointer)) {
            case SC_LAND_GROUND:
               color = &colormap->gradient[SC_GRAD_GROUND][SC_LAND_GET_COLOR(*lpointer)];
               break;
            case SC_LAND_NIGHT_SKY:
               color = &colormap->gradient[SC_GRAD_NIGHT_SKY][SC_LAND_GET_COLOR(*lpointer)];
               break;
            case SC_LAND_FIRE_SKY:
               color = &colormap->gradient[SC_GRAD_FIRE_SKY][SC_LAND_GET_COLOR(*lpointer)];
               break;
            case SC_LAND_SMOKE:
               color = &colormap->pcolors[SC_LAND_GET_COLOR(*lpointer)];
               break;
            case SC_LAND_OBJECT:
               color = &colormap->white;
               break;
            default:
               color = &colormap->black;
         }
         /* Only set new pen color if we actually changed pens. */
         if(color != lcolor) {
            if(y0 >= 0) {
               gdk_gc_set_foreground(gc, lcolor);
               gdk_draw_line(buffer, gc, x, height - y0, x, height - y + 1);
            }
            lcolor = color;
            y0 = y;
         }
         /* Draw the point. */
         /*gdk_draw_point(buffer, gc, x, height - y);*/
      } /* Iterating in Y ... */
      if(y0 >= 0) {
         gdk_gc_set_foreground(gc, lcolor);
         gdk_draw_line(buffer, gc, x, height - y0, x, height - y2);
      }
   } /* Iterating in X ... */
   
}



static void _sc_window_draw_land_image_gtk(sc_window_gtk *w, int x1, int y1, int x2, int y2) {
/* sc_window_draw_land_image_gtk
   This code draws land using a GdkImage object (stored client-side).  I
   have found for pixel-by-pixel images, it is faster to construct a client-
   side image and send the entire image across as a single command, as
   opposed to sending a large number of draw-pixel commands to the server. 
   Note this does have some overhead associated, and in general the classic
   pixel-by-pixel approach should be used if only a small rectangular area
   is being updated.  The bounding box MUST be in real coordinates.  */

   GdkImage *image;           /* Local image to draw into */
   sc_color_gtk *colormap;    /* Colormap data */
   GdkColor *color;           /* Current color */
   const int *lpointer;       /* Pointer into land */
   int fheight;               /* Plotter field height */
   int height;                /* Image height */
   int width;                 /* Image width */
   int x;                     /* Current X offset (land coords) */
   int y;                     /* Current Y offset (land coords) */
   
   /* Get the colormap data, pixmap, and graphics context */
   fheight = w->c->fieldheight - 1;
   colormap = w->colormap;
   
   /* Create the local image to draw into */
   height= y2 - y1 + 1;
   width = x2 - x1 + 1;
   image = gdk_image_new(GDK_IMAGE_FASTEST, gtk_widget_get_visual(w->app), width, height);
   if(image == NULL) return;

   /* All points must be redrawn */
   for(x = 0; x < width; ++x) {
      /* Get pointer to bottom of current column. */
      lpointer = SC_LAND_XY(w->c->land, x1 + x, y1);
      for(y = 0; y < height; ++y, ++lpointer) {
         /* Iterating along the column ... */
         switch(SC_LAND_GET_TYPE(*lpointer)) {
            case SC_LAND_GROUND:
               color = &colormap->gradient[SC_GRAD_GROUND][SC_LAND_GET_COLOR(*lpointer)];
               break;
            case SC_LAND_NIGHT_SKY:
               color = &colormap->gradient[SC_GRAD_NIGHT_SKY][SC_LAND_GET_COLOR(*lpointer)];
               break;
            case SC_LAND_FIRE_SKY:
               color = &colormap->gradient[SC_GRAD_FIRE_SKY][SC_LAND_GET_COLOR(*lpointer)];
               break;
            case SC_LAND_SMOKE:
               color = &colormap->pcolors[SC_LAND_GET_COLOR(*lpointer)];
               break;
            case SC_LAND_OBJECT:
               color = &colormap->white;
               break;
            default:
               color = &colormap->black;
         } /* What type of land? */
         /* Draw the point. */
         gdk_image_put_pixel(image, x, height - y - 1, color->pixel);
      } /* Iterating in Y ... */
   } /* Iterating in X ... */

   /* Copy local image to the offscreen pixmap */
   gdk_draw_image(w->landbuffer, sc_display_get_gc(SC_DISPLAY(w->screen)), image, 
                  0, 0, x1, fheight - y2, width, height);
   g_object_unref(image);
   
}



static inline void _sc_window_draw_land_gtk(sc_window_gtk *w, int x1, int y1, int x2, int y2) {
/* sc_window_draw_land_gtk
   This is a wrapper for the above two functions, which will select the
   appropriate function based on the number of pixels needing update.  This
   will also clip the input arguments to make sure they are in a valid
   range.  */

   /* Make sure X, Y are in bounds. */
   if(x1 < 0) x1 = 0;
   if(y1 < 0) y1 = 0;
   if(x2 >= w->c->fieldwidth)  x2 = w->c->fieldwidth - 1;
   if(y2 >= w->c->fieldheight) y2 = w->c->fieldheight- 1;

   /* Which subfunction to use? */
   if((y2 - y1) * (x2 - x1) <= USE_PIXMAP_MAX_AREA) { 
      _sc_window_draw_land_pixmap_gtk(w, x1, y1, x2, y2);
   } else {
      _sc_window_draw_land_image_gtk(w, x1, y1, x2, y2);
   }
   
}



void sc_window_paint(sc_window *w_, int x1, int y1, int x2, int y2, int flags) {
/* sc_window_paint
   This function updates a rectangular region of the display, using the
   coordinates (x1,y1)-(x2,y2) as a bounding box for the area needing
   update.  Flags are also given to indicate which drawing operations should
   be done -- note SC_REGENERATE_LAND is VERY expensive and should be
   avoided when the land hasn't been physically altered.  */

   sc_window_gtk *w = (sc_window_gtk *)w_;/* Window structure */
   int ox1;                   /* Overlap X1 */
   int ox2;                   /* Overlap X2 */
   int screenx;               /* Screen top-left X */
   int screeny;               /* Screen top-left Y */
   int screenw;               /* Width of area being redrawn */
   int screenh;               /* Height of area being redrawn */
   
   /* Swap boundaries if they are reversed. */
   if(x1 > x2) {
      x1 = x1 + x2;
      x2 = x1 - x2;
      x1 = x1 - x2;
   } /* X coordinates swapped? */
   if(y1 > y2) {
      y1 = y1 + y2;
      y2 = y1 - y2;
      y1 = y1 - y2;
   } /* Y coordinates swapped? */

   /* Correct X bounds */
   if(!sc_land_translate_x_range(w->c->land, &x1, &x2)) return;
   
   /* Check for overlap */
   ox1 = x1;
   ox2 = x2;
   if(sc_land_overlap_x(w->c->land, &ox1, &ox2)) {
      sc_window_paint(w_, ox1, y1, ox2, y2, flags);
   }
    
   /* Determine the screen boundaries. */
   screenx = x1;
   screeny = w->c->fieldheight - y2 - 1;
   screenw = x2 - x1 + 1;
   screenh = y2 - y1 + 1;

   /* Regeneration of land pixmap */
   if(flags & SC_REGENERATE_LAND) {
      _sc_window_draw_land_gtk(w, x1, y1, x2, y2);
   } /* Regenerate the land? */

   /* Unpaint any existing wind arrow, if clear given */
   if(flags & SC_CLEAR_WIND_ARROW || flags & SC_REDRAW_WIND_ARROW) {
      _sc_window_draw_wind_arrow(w, false);
   } /* Undraw wind arrow from screen? */

   /* Copy current land pixmap to screen. */
   if(flags & SC_REDRAW_LAND) {
      gdk_draw_drawable(sc_display_get_buffer(SC_DISPLAY(w->screen)), 
                        sc_display_get_gc(SC_DISPLAY(w->screen)),
                        w->landbuffer, 
                        screenx, screeny, screenx, screeny, screenw, screenh);
      sc_display_queue_draw(SC_DISPLAY(w->screen), screenx, screeny, screenw, screenh);
   } /* Copy land to screen? */

   /* Draw any tanks in redraw area to the screen. */
   if(flags & SC_REDRAW_TANKS) {
      _sc_window_draw_all_tanks_gtk(w, x1, y1, x2, y2);
   } /* Draw tanks to screen? */

   /* Draw wind arrow if requested */
   if(flags & SC_REDRAW_WIND_ARROW) {
      _sc_window_draw_wind_arrow(w, true);
   } /* Draw wind arrow to screen? */

}



void sc_window_paint_circular(sc_window *w_, int centerx, int centery, int rad, int flags) {
/* sc_window_paint_circular
   Like the above function, but this updates a circular region of the
   physical display (it will still update a rectangular region of the
   INTERNAL data structures, however).  (centerx, centery) are specified
   in the usual virtual coordinates. 
   
   It is NOT advised that you call this during a SC_REGENERATE_LAND. */
   
   /* TEMP:  I don't play the wraparound game, yet. */
   /* TEMP:  I don't play with wind arrows yet. */

   GdkGC *gc = NULL;
   GdkGC *displaygc;
   GdkPixmap *mask = NULL;
   GdkColor black = { 0, 0x0000, 0x0000, 0x0000 };
   GdkColor white = { 1, 0xffff, 0xffff, 0xffff };
   sc_window_gtk *w = (sc_window_gtk *)w_;/* Window structure */
   int size;
   int minx;
   int miny;
   
   /* Make sure radius is sane. */
   if(rad < 0) return;
   
   /* Construct the clipping mask.  This code is modelled from the code in
      sc_expl_cache_draw().  In fact, it pretty much is the same code... */

   /* Get the display GC */
   displaygc = sc_display_get_gc(SC_DISPLAY(w->screen));

   /* Calculate the screen center coordinates */
   size = rad + rad + 1;
   minx = centerx - rad;
   miny = (w->c->fieldheight - centery - 1) - rad;
   
   /* Create a new temporary mask */
   mask   = gdk_pixmap_new(NULL, size, size, 1);
   if(mask != NULL) gc = gdk_gc_new(mask);
   if(mask != NULL && gc != NULL) {
      gdk_gc_set_foreground(gc, &black);
      gdk_draw_rectangle(mask, gc, TRUE, 0, 0, size, size);
      gdk_gc_set_foreground(gc, &white);
      gdk_draw_arc(mask, gc, TRUE, 0, 0, size, size, 0, 360 * 64);
      gdk_gc_set_clip_mask(displaygc, mask);
      gdk_gc_set_clip_origin(displaygc, minx, miny);
   }

   /* Call the main painter */
   sc_window_paint(w_, centerx - rad, centery - rad, centerx + rad, centery + rad, flags);
   
   /* Uninstall the clipping mask */
   gdk_gc_set_clip_mask(displaygc, NULL);
   if(mask != NULL) g_object_unref(mask);
   if(gc != NULL) g_object_unref(gc);
   
}



void sc_window_undraw_tank(sc_window *w_, const sc_player *p) {
/* sc_window_undraw_tank
   Erases the indicated tank from the player field (useful, f.e., if the
   tank is about to move).  Uses p's coordinates to determine the current
   tank location.  */

   int radius = p->tank->turretradius;
   sc_window_paint(w_, p->x - radius, p->y - radius, p->x + radius, p->y + radius, SC_REDRAW_LAND);

}



void sc_window_draw_tank(sc_window *w_, const sc_player *p) {
/* sc_window_draw_tank
   Draws the indicated tank from the player field (useful, f.e., if the
   tank has just moved).  Uses p's coordinates to determine the current
   tank location.  */

   int radius = p->tank->turretradius;
   sc_window_paint(w_, p->x - radius, p->y - radius, p->x + radius, p->y + radius, SC_REDRAW_TANKS);

}



void sc_window_redraw_tank(sc_window *w_, const sc_player *p) {
/* sc_window_redraw_tank
   Redraws the indicated tank to the screen - useful when the tank has
   just changed configuration, e.g. shield or turret-angle changes.  */

   int radius = p->tank->turretradius;
   sc_window_paint(w_, p->x - radius, p->y - radius, p->x + radius, p->y + radius, SC_REDRAW_LAND | SC_REDRAW_TANKS);

}



void sc_window_undraw_weapon(sc_window *w_, const sc_weapon *wp) {
/* sc_window_undraw_weapon
   Erase the indicated weapon from the screen.  */

   sc_window_gtk *w = (sc_window_gtk *)w_;
   int size = w->c->weapons->bombiconsize;
   int x = rint(wp->tr->curx) - (size >> 1);
   int y = rint(wp->tr->cury) - (size >> 1);

   sc_window_paint(w_, x, y - size + 1, x + size - 1, y, SC_REDRAW_LAND | SC_REDRAW_TANKS);

}



void sc_window_draw_weapon(sc_window *w_, const sc_weapon *wp) {
/* sc_window_draw_weapon
   Draw the indicated weapon to the screen.  */

   sc_window_gtk *w = (sc_window_gtk *)w_;
   int size = w->c->weapons->bombiconsize;
   GdkPixmap *buffer;
   GdkGC *gc;
   int x;
   int y;
   
   x = rint(wp->tr->curx) - (size >> 1);
   y = rint(wp->tr->cury) - (size >> 1);
   if(!sc_land_translate_xy(w->c->land, &x, &y)) return;
   x = x;
   y = w->c->fieldheight - y - 1;

   buffer = sc_display_get_buffer(SC_DISPLAY(w->screen));   
   gc = sc_display_get_gc(SC_DISPLAY(w->screen));
   gdk_gc_set_foreground(gc, &w->colormap->white);
   gdk_draw_rectangle(buffer, gc, TRUE, x, y, size, size);
   sc_display_queue_draw(SC_DISPLAY(w->screen), x, y, size, size);

}



void sc_window_paint_blank(sc_window *w_) {
/* sc_window_paint_blank
   Erase the entire screen and fill it in with black.  */

   sc_window_gtk *w = (sc_window_gtk *)w_;

   gdk_gc_set_foreground(sc_display_get_gc(SC_DISPLAY(w->screen)),
                         &w->colormap->black);
   gdk_draw_rectangle(sc_display_get_buffer(SC_DISPLAY(w->screen)),
                      sc_display_get_gc(SC_DISPLAY(w->screen)),
                      TRUE,
                      0, 0,
                      w->screen->allocation.width, w->screen->allocation.height);
   sc_display_queue_draw(SC_DISPLAY(w->screen),
                         0, 0,
                         w->screen->allocation.width, w->screen->allocation.height);

}



static sc_trajectory_result _sc_window_draw_arc_gtk(sc_config *c, sc_trajectory *tr, void *data) {
/* sc_window_draw_arc_gtk */

   sc_window_gtk *w = (sc_window_gtk *)data;
   GdkPixmap *buffer;
   GdkGC *gc;
   int x;
   int y;

   x = rint(tr->curx);
   y = rint(tr->cury);
   if(!sc_land_translate_xy(w->c->land, &x, &y)) {
      return(SC_TRAJ_CONTINUE);
   }
   
   buffer = sc_display_get_buffer(SC_DISPLAY(w->screen));   
   gc = sc_display_get_gc(SC_DISPLAY(w->screen));
   gdk_gc_set_foreground(gc, &w->colormap->pcolors[tr->victim]);
   gdk_draw_rectangle(buffer, gc, TRUE, x, c->fieldheight - y - 1, 1, 1);
   sc_display_queue_draw(SC_DISPLAY(w->screen), x, c->fieldheight - y - 1, 1, 1);
   return(SC_TRAJ_CONTINUE);

}



static sc_trajectory_result _sc_window_clear_arc_gtk(__libj_unused sc_config *c, sc_trajectory *tr, void *data) {
/* sc_window_clear_arc_gtk */

   sc_window_gtk *w = (sc_window_gtk *)data;
   int x = rint(tr->curx);
   int y = rint(tr->cury);

   sc_window_paint((sc_window *)w, x, y, x, y, SC_REDRAW_LAND | SC_REDRAW_TANKS);
   return(SC_TRAJ_CONTINUE);

}



void sc_window_draw_arc(sc_window *w_, sc_trajectory *tr, int playerid) {
/* sc_window_draw_arc */

   sc_window_gtk *w = (sc_window_gtk *)w_;

   sc_traj_reinitialize(tr);
   tr->victim = playerid;
   sc_traj_run(w->c, tr, SC_TRAJ_IGNORE_TANK | SC_TRAJ_IGNORE_LAND, _sc_window_draw_arc_gtk, w);

}



void sc_window_clear_arc(sc_window *w_, sc_trajectory *tr) {
/* sc_window_clear_arc */

   sc_window_gtk *w = (sc_window_gtk *)w_;

   sc_traj_reinitialize(tr);
   sc_traj_run(w->c, tr, SC_TRAJ_IGNORE_TANK | SC_TRAJ_IGNORE_LAND, _sc_window_clear_arc_gtk, w);

}
