/* $Header: /fridge/cvs/xscorch/sgtk/scolor-gtk.c,v 1.11 2009-04-26 17:39:47 jacob Exp $ */
/*
   
   xscorch - scolor-gtk.c     Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/
    
   GTK Colormap
    

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
#include <stdlib.h>

#include <sgtk.h>
#include <scolor-gtk.h>
#include <swindow-gtk.h>



/*** Next few functions are the gradient subsystem ***/



static inline gboolean _sc_colormap_set_gtk(sc_window_gtk *w, GdkColor *c, double r, double g, double b) {

   if(r < 0) r = 0;
   if(g < 0) g = 0;
   if(b < 0) b = 0;
   if(r > 1) r = 1;
   if(g > 1) g = 1;
   if(b > 1) b = 1;
   
   c->red   = r * 0xffff;
   c->green = g * 0xffff;
   c->blue  = b * 0xffff;
   return(gdk_colormap_alloc_color(gtk_widget_get_colormap(w->app), c, FALSE, TRUE));

}



sc_color_gtk *sc_colormap_new_gtk(void) {

   return((sc_color_gtk *)malloc(sizeof(sc_color_gtk)));

}



void sc_colormap_free_gtk(sc_color_gtk **color) {

   if(color == NULL || *color == NULL) return;
   free(*color);
   *color = NULL;

}



static inline void _sc_colormap_gradient_gtk(sc_window_gtk *w, int gradidx, double r, double g, double b, double fr, double fg, double fb) {

   double dr;
   double dg;
   double db;
   int count;
   int i;
   
   dr = (fr - r) / SC_MAX_GRADIENT_SIZE;
   dg = (fg - g) / SC_MAX_GRADIENT_SIZE;
   db = (fb - b) / SC_MAX_GRADIENT_SIZE;

   for(count = 0, i = 0; i < SC_MAX_GRADIENT_SIZE; ++i) {
      if(_sc_colormap_set_gtk(w, &w->colormap->gradient[gradidx][count], r, g, b)) { 
         if(count == 0 || w->colormap->gradient[gradidx][count].pixel != w->colormap->gradient[gradidx][count - 1].pixel) {
            ++count;
         }
      }
      r += dr;
      g += dg;
      b += db;
   }
   w->c->colors->gradsize[gradidx] = count;

}



void sc_colormap_alloc_colors_gtk(sc_window_gtk *w) {

   sc_color_gtk *colormap = w->colormap;
   sc_color *colors = w->c->colors;

   /* Initialise gradient subsystem */
   _sc_colormap_gradient_gtk(w, SC_GRAD_GROUND,          0,    0.47, 0,    0,    0.72, 0);
   _sc_colormap_gradient_gtk(w, SC_GRAD_NIGHT_SKY,       0,    0,    0.05, 0,    0,    0.45);
   _sc_colormap_gradient_gtk(w, SC_GRAD_FIRE_SKY,        1.00, 0.75, 0.25, 0.37, 0,    0);
   _sc_colormap_gradient_gtk(w, SC_GRAD_EXPLOSION,       0.80, 0,    0,    0.20, 0,    0);
   _sc_colormap_gradient_gtk(w, SC_GRAD_FUNKY_EXPLOSION, 0.80, 0.40, 0,    0.50, 0.10, 0);
   _sc_colormap_gradient_gtk(w, SC_GRAD_MAGNETIC,        0,    0,    0,    1.00, 0.50, 0.00);
   _sc_colormap_gradient_gtk(w, SC_GRAD_SHIELD,          0,    0,    0,    1.00, 1.00, 1.00);
   _sc_colormap_gradient_gtk(w, SC_GRAD_FORCE,           0,    0,    0,    1.00, 0,    1.00);
   _sc_colormap_gradient_gtk(w, SC_GRAD_FLAMES,          0.40, 0.00, 0.00, 0.80, 0.40, 0.00);

   _sc_colormap_set_gtk(w, &colormap->black, 0.0, 0.0, 0.0);
   _sc_colormap_set_gtk(w, &colormap->gray,  0.5, 0.5, 0.5);
   _sc_colormap_set_gtk(w, &colormap->white, 1.0, 1.0, 1.0);
   _sc_colormap_set_gtk(w, &colormap->yellow,1.0, 1.0, 0.0);
   _sc_colormap_set_gtk(w, &colormap->napalm,1.0, 0.4, 0.1);
   _sc_colormap_set_gtk(w, &colormap->windar,0.7, 0.7, 1.0);

   _sc_colormap_set_gtk(w, &colormap->pcolors[0], 1.0, 0.1, 0.1);
   _sc_colormap_set_gtk(w, &colormap->pcolors[1], 1.0, 1.0, 0.2);
   _sc_colormap_set_gtk(w, &colormap->pcolors[2], 0.5, 0.5, 0.5);
   _sc_colormap_set_gtk(w, &colormap->pcolors[3], 0.2, 1.0, 1.0);
   _sc_colormap_set_gtk(w, &colormap->pcolors[4], 0.2, 0.2, 1.0);
   _sc_colormap_set_gtk(w, &colormap->pcolors[5], 1.0, 0.2, 1.0);
   _sc_colormap_set_gtk(w, &colormap->pcolors[6], 1.0, 1.0, 1.0);
   _sc_colormap_set_gtk(w, &colormap->pcolors[7], 0.2, 1.0, 0.2);
   _sc_colormap_set_gtk(w, &colormap->pcolors[8], 0.7, 0.7, 1.0);
   _sc_colormap_set_gtk(w, &colormap->pcolors[9], 1.0, 0.3, 0.05);

   sc_color_gradient_init(w->c, colors);

}
