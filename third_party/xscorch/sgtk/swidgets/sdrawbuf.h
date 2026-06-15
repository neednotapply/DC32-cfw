/* $Header: /fridge/cvs/xscorch/sgtk/swidgets/sdrawbuf.h,v 1.7 2009-04-26 17:39:53 jacob Exp $ */
/*

   xscorch - drawbuf.h        Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   DrawingArea with an offscreen buffer to draw on.


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
#ifndef __sdrawbuf_h_included
#define __sdrawbuf_h_included


/* Include files */
#include <sgtk.h>
#include <gtk/gtk.h>
#include <gtk/gtkdrawingarea.h>


/* ScDrawbuf casts */
#define  SC_DRAWBUF(obj)         G_TYPE_CHECK_INSTANCE_CAST((obj), sc_drawbuf_get_type(), ScDrawbuf)
#define  SC_DRAWBUF_CLASS(klass) G_TYPE_CHECK_CLASS_CAST((klass), sc_drawbuf_get_type(), ScDrawbufClass)
#define  IS_SC_DRAWBUF(obj)      G_TYPE_CHECK_INSTANCE_TYPE((obj), sc_drawbuf_get_type())


/* ScDrawbuf structure */
typedef struct _ScDrawbuf {
   GtkDrawingArea drawing_area;     /* Parent is a drawing area */
   GdkPixmap *screen_buffer;        /* OffScreen drawable pixmap */
   GdkGC *screen_gc;                /* OffScreen drawable GC */
   gboolean style_configured;       /* True when style has been setup */
} ScDrawbuf;


/* ScDrawbufClass structure */
typedef struct _ScDrawbufClass {
   GtkDrawingAreaClass parent_class;/* Drawing area class data */
} ScDrawbufClass;


/* ScDrawbuf initialisation and basic info */
GType sc_drawbuf_get_type(void);
GtkWidget *sc_drawbuf_new(gint width, gint height);


/* Hooks to get useful information from a drawbuf */
#define sc_drawbuf_get_buffer(draw) ((draw)->screen_buffer)
#define sc_drawbuf_get_gc(draw)     ((draw)->screen_gc)


/* Redraw a drawing buffer */
void sc_drawbuf_queue_draw(ScDrawbuf *draw, gint x, gint y, gint width, gint height);


#endif /* __sdrawbuf_h_included */
