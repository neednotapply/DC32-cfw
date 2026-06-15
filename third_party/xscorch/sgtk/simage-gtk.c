/* $Header: /fridge/cvs/xscorch/sgtk/simage-gtk.c,v 1.7 2009-04-26 17:39:48 jacob Exp $ */
/*
   
   xscorch - simage-gtk.c     Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/
    
   GTK interface to image drawing
    

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
#include <simage-gtk.h>



inline gint sc_pixmap_width_gtk(GdkPixmap *pix) {

   gint width;
   
   width = 0;
   gdk_drawable_get_size(pix, &width, NULL);
   return(width);

}

 
 
inline gint sc_pixmap_height_gtk(GdkPixmap *pix) {

   gint height;
   
   height = 0;
   gdk_drawable_get_size(pix, NULL, &height);
   return(height);

}



void sc_pixmap_copy_gtk(GdkPixmap *dest, GdkGC *gc, GdkPixmap *src, GdkBitmap *mask, int dx, int dy) {

   gdk_gc_set_clip_origin(gc, dx, dy);
   gdk_gc_set_clip_mask(gc, mask);
   gdk_draw_drawable(dest, gc, src,
                     0, 0,
                     dx, dy,
                     sc_pixmap_width_gtk(src),
                     sc_pixmap_height_gtk(src));
   gdk_gc_set_clip_mask(gc, NULL);
   
}
