/* $Header: /fridge/cvs/xscorch/sgtk/swidgets/sdrawbuf.c,v 1.20 2011-07-31 19:48:00 jacob Exp $ */
/*

   xscorch - sdrawbuf.c       Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Drawing area with an offscreen buffer associated with it


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
#include <sdrawbuf.h>



static GtkDrawingAreaClass *parent_class;



static inline void _sc_drawbuf_unref_pixmap_and_gc(ScDrawbuf *draw) {

   /* Release associated GC, pixmap */
   if(sc_drawbuf_get_buffer(draw) != NULL) {
      g_object_unref(sc_drawbuf_get_buffer(draw));
      sc_drawbuf_get_buffer(draw) = NULL;
   }
   if(sc_drawbuf_get_gc(draw) != NULL) {
      g_object_unref(sc_drawbuf_get_gc(draw));
      sc_drawbuf_get_gc(draw) = NULL;
   }

}



static void _sc_drawbuf_destroy(GtkObject *obj) {

   ScDrawbuf *draw = SC_DRAWBUF(obj);

   /* Release associated GC, pixmap */
   _sc_drawbuf_unref_pixmap_and_gc(draw);

   /* Call parent class handler? */
   if(GTK_OBJECT_CLASS(parent_class)->destroy != NULL) {
      GTK_OBJECT_CLASS(parent_class)->destroy(obj);
   } /* Call parent destroy? */

}



static gint _sc_drawbuf_configure(GtkWidget *widget, GdkEventConfigure *event) {

   ScDrawbuf *draw = SC_DRAWBUF(widget);
   gint width = widget->allocation.width;
   gint height= widget->allocation.height;
   gint pixmap_width = -1;
   gint pixmap_height= -1;
   GdkColor black;

   /* Call parent class handler? */
   if(GTK_WIDGET_CLASS(parent_class)->configure_event != NULL) {
      if(GTK_WIDGET_CLASS(parent_class)->configure_event(widget, event)) {
         /* We must halt. */
         return(TRUE);
      } /* Can we continue? */
   } /* Call parent handler? */

   /* Get current pixmap dimensions */
   if(sc_drawbuf_get_buffer(draw) != NULL) {
      gdk_drawable_get_size(sc_drawbuf_get_buffer(draw), &pixmap_width, &pixmap_height);
   }

   /* Make sure the size actually changed ... */
   if(width != pixmap_width || height != pixmap_height) {
      /* Has the style been configured? */
      if(!draw->style_configured) {
         gdk_color_black(gtk_widget_get_colormap(widget), &black);
         gdk_window_set_background(widget->window, &black);
      }

      /* Release any existing drawing buffer */
      _sc_drawbuf_unref_pixmap_and_gc(draw);

      /* Construct new offscreen-drawable, and clear it */
      draw->screen_buffer = gdk_pixmap_new(widget->window, width, height, -1);
      gdk_draw_rectangle(draw->screen_buffer,
                         widget->style->black_gc,
                         TRUE,
                         0, 0,
                         width, height);

      /* Construct a new offscreen-GC to use */
      draw->screen_gc = gdk_gc_new(widget->window);
   }

   /* Allow other events */
   return(FALSE);

}



static gint _sc_drawbuf_expose(GtkWidget *widget, GdkEventExpose *event) {

   ScDrawbuf *draw = SC_DRAWBUF(widget);
   GdkGC *fg_gc = widget->style->fg_gc[gtk_widget_get_state((GtkWidget *)draw)];

   /* Make sure buffer is allocated */
   g_return_val_if_fail(sc_drawbuf_get_buffer(draw) != NULL, TRUE);

   /* Draw from offscreen image to the screen. */
   gdk_draw_drawable(widget->window,
                     fg_gc, draw->screen_buffer,
                     event->area.x, event->area.y,
                     event->area.x, event->area.y,
                     event->area.width, event->area.height);

   /* Allow other events to process */
   return(FALSE);

}



static void _sc_drawbuf_class_init(ScDrawbufClass *klass) {

   /* Determine the parent class */
   parent_class = g_type_class_peek(gtk_drawing_area_get_type());

   /* Latch default hooks */
   GTK_WIDGET_CLASS(klass)->configure_event = _sc_drawbuf_configure;
   GTK_OBJECT_CLASS(klass)->destroy         = _sc_drawbuf_destroy;

}



static void _sc_drawbuf_init_obj(ScDrawbuf *draw) {

   /* Initialise data members */
   draw->screen_buffer = NULL;
   draw->screen_gc = NULL;
   draw->style_configured = FALSE;

   /* Setup widget, and object signals */
   gtk_widget_set_can_focus(GTK_WIDGET(draw), FALSE);
   gtk_widget_add_events(GTK_WIDGET(draw), GDK_EXPOSURE_MASK);
   gtk_widget_set_app_paintable(GTK_WIDGET(draw), TRUE);

   /* Install expose event handler. In GTK 2.0, the drawbuf is
      never drawn if this is installed as the default signal
      handler, but I do not understand why.  */
   g_signal_connect(G_OBJECT(draw), "expose_event",
                    (GCallback)_sc_drawbuf_expose, NULL);

}



GType sc_drawbuf_get_type(void) {

   static GType sc_drawbuf_type = 0;

   if(sc_drawbuf_type == 0) {
      static const GTypeInfo sc_drawbuf_info = {
         sizeof(ScDrawbufClass),          /* Size of the class object */
         NULL,                            /* Base initializer */
         NULL,                            /* Base finalizer */
         (GClassInitFunc)_sc_drawbuf_class_init,
                                          /* Class initializer */
         NULL,                            /* Class finalizer */
         NULL,                            /* Class data pointer */
         sizeof(ScDrawbuf),               /* Size of an instance object */
         0,                               /* Number of preallocs */
         (GInstanceInitFunc)_sc_drawbuf_init_obj,
                                          /* Instance initializer */
         NULL                             /* Value table */
      };
      sc_drawbuf_type = g_type_register_static(gtk_drawing_area_get_type(), "ScDrawbuf",
                                               &sc_drawbuf_info, 0);
   }

   return(sc_drawbuf_type);

}



GtkWidget *sc_drawbuf_new(gint width, gint height) {

   ScDrawbuf *draw;

   /* Allocate the object */
   draw = g_object_new(sc_drawbuf_get_type(), NULL);
   g_return_val_if_fail(draw != NULL, NULL);

   /* Set its minimum size allocation */
   gtk_widget_set_size_request(GTK_WIDGET(draw), width, height);

   /* Return the new object */
   return(GTK_WIDGET(draw));

}



void sc_drawbuf_queue_draw(ScDrawbuf *draw, gint x, gint y, gint width, gint height) {

   if(x < 0) {
      width += x;
      x = 0;
   }
   if(y < 0) {
      height += y;
      y = 0;
   }
   gtk_widget_queue_draw_area((GtkWidget *)draw, x, y, width + 1, height + 1);

}
