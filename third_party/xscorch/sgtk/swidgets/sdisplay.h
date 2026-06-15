/* $Header: /fridge/cvs/xscorch/sgtk/swidgets/sdisplay.h,v 1.8 2009-04-26 17:39:53 jacob Exp $ */
/*

   xscorch - sdisplay.h       Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Display code; manages drawing buffers with consoles


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
#ifndef __sdisplay_h_included
#define __sdisplay_h_included


#include <sgtk.h>
#include <gtk/gtk.h>
#include <gtk/gtkfixed.h>
#include <sdrawbuf.h>
#include <sconsole.h>


#define  SC_DISPLAY(obj)         G_TYPE_CHECK_INSTANCE_CAST(obj, sc_display_get_type(), ScDisplay)
#define  SC_DISPLAY_CLASS(klass) G_TYPE_CHECK_CLASS_CAST(klass, sc_display_get_type(), ScDisplayClass)
#define  IS_SC_DISPLAY(obj)      G_TYPE_CHECK_INSTANCE_TYPE(obj, sc_display_get_type())


typedef struct _ScDisplay {
   GtkFixed fixed;
   GtkWidget *draw;
} ScDisplay;


typedef struct _ScDisplayClass {
   GtkFixedClass parent_class;
} ScDisplayClass;


GType sc_display_get_type(void);
GtkWidget *sc_display_new(gint width, gint height);


#define sc_display_get_drawbuf(dpy)    (dpy->draw)
#define sc_display_get_buffer(dpy)     sc_drawbuf_get_buffer(SC_DRAWBUF(sc_display_get_drawbuf(dpy)))
#define sc_display_get_gc(dpy)         sc_drawbuf_get_gc(SC_DRAWBUF(sc_display_get_drawbuf(dpy)))


void     sc_display_queue_draw(ScDisplay *dpy, gint x, gint y, gint width, gint height);
void     sc_display_console_attach(ScDisplay *dpy, ScConsole *cons);
gboolean sc_display_console_detach(ScDisplay *dpy);
void     sc_display_console_detach_all(ScDisplay *dpy);
void     sc_display_console_set_fonts(ScDisplay *dpy, GdkFont *font, GdkFont *boldfont);


#endif /* __sdisplay_h_included */
