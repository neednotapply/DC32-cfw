/* $Header: /fridge/cvs/xscorch/sgtk/swidgets/sdisplay.c,v 1.18 2011-04-15 06:04:26 jacob Exp $ */
/*

   xscorch - sdisplay.c       Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Control for containing a sdisplay with an optional console


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
#include <assert.h>

#include <sdisplay.h>



static void _sc_display_init_obj(ScDisplay *dpy) {

   assert(IS_SC_DISPLAY(dpy));
   dpy->draw = NULL;
   gtk_widget_set_can_focus(GTK_WIDGET(dpy), FALSE);

}



GType sc_display_get_type(void) {

   static GType sc_display_type = 0;

   if(sc_display_type == 0) {
      static const GTypeInfo sc_display_info = {
         sizeof(ScDisplayClass),          /* Size of the class object */
         NULL,                            /* Base initializer */
         NULL,                            /* Base finalizer */
         NULL,                            /* Class initializer */
         NULL,                            /* Class finalizer */
         NULL,                            /* Class data pointer */
         sizeof(ScDisplay),               /* Size of an instance object */
         0,                               /* Number of preallocs */
         (GInstanceInitFunc)_sc_display_init_obj,
                                          /* Instance initializer */
         NULL                             /* Value table */
      };
      sc_display_type = g_type_register_static(gtk_fixed_get_type(), "ScDisplay",
                                               &sc_display_info, 0);
   }

   return(sc_display_type);

}



GtkWidget *sc_display_new(gint width, gint height) {

   ScDisplay *dpy;

   g_return_val_if_fail(width > 0 && height > 0, NULL);

   dpy = g_object_new(sc_display_get_type(), NULL);
   g_return_val_if_fail(dpy != NULL, NULL);

   dpy->draw = sc_drawbuf_new(width, height);
   gtk_fixed_put(GTK_FIXED(dpy), dpy->draw, 0, 0);
   gtk_widget_show(dpy->draw);

   return(GTK_WIDGET(dpy));

}



void sc_display_console_attach(ScDisplay *dpy, ScConsole *cons) {

   g_return_if_fail(IS_SC_DISPLAY(dpy));
   g_return_if_fail(IS_SC_CONSOLE(cons));

   gtk_fixed_put(GTK_FIXED(dpy), GTK_WIDGET(cons), cons->req_alloc.x, cons->req_alloc.y);
   gtk_widget_show(GTK_WIDGET(cons));

}



gboolean sc_display_console_detach(ScDisplay *dpy) {

   GtkFixedChild *child;
   GtkWidget *cwidget;
   GList *children;

   g_return_val_if_fail(IS_SC_DISPLAY(dpy), FALSE);

   children = GTK_FIXED(dpy)->children;

   /* The first entry is the basic drawbuf; never delete it.  */
   if(children == NULL || children->next == NULL) return(FALSE);
   while(children->next != NULL) children = children->next;
   child = children->data;
   cwidget = child->widget;
   g_return_val_if_fail(IS_SC_CONSOLE(cwidget), FALSE);

   gtk_container_remove(GTK_CONTAINER(dpy), cwidget);
   /* gtk_container_remove destroys the object on its own,
      apparently.  We do not need to call gtk_widget_destroy.  */
   return(TRUE);

}



void sc_display_console_detach_all(ScDisplay *dpy) {

   while(sc_display_console_detach(dpy)) /* Just loop */;

}



void sc_display_queue_draw(ScDisplay *dpy, gint x, gint y, gint width, gint height) {

   g_return_if_fail(IS_SC_DISPLAY(dpy));

   sc_drawbuf_queue_draw(SC_DRAWBUF(sc_display_get_drawbuf(dpy)), x, y, width, height);

}



void sc_display_console_set_fonts(ScDisplay *dpy, GdkFont *font, GdkFont *boldfont) {

   GtkFixedChild *child;
   GtkWidget *cwidget;
   ScConsole *cons;
   GList *children;

   #if SC_GTK_DEBUG_GTK
      printf("sc_display_console_set_fonts:  installing new console fonts for %p\n", (void *)dpy);
   #endif /* SC_GTK_DEBUG_GTK */
            
   g_return_if_fail(IS_SC_DISPLAY(dpy));
   g_return_if_fail(font != NULL && boldfont != NULL);

   children = GTK_FIXED(dpy)->children;

   if(children == NULL) return;
   children = children->next;
   while(children != NULL) {
      child = children->data;
      cwidget = child->widget;
      g_return_if_fail(IS_SC_CONSOLE(cwidget));
      cons = SC_CONSOLE(cwidget);
      #if SC_GTK_DEBUG_GTK
         printf("sc_display_console_set_fonts:  installing new console fonts for %p, registered console %p\n", 
                (void *)dpy, (void *)cons);
      #endif /* SC_GTK_DEBUG_GTK */

      sc_console_set_fonts(cons, font, boldfont);
      /* The font update will automatically adjust the console's 
         requested X/Y position, but we still need to tell the
         container to adjust the position.  */
      gtk_fixed_move(GTK_FIXED(dpy), cwidget, cons->req_alloc.x, cons->req_alloc.y);
      children = children->next;
   }

   #if SC_GTK_DEBUG_GTK
      printf("sc_display_console_set_fonts:  finished installing new console fonts for %p\n", (void *)dpy);
   #endif /* SC_GTK_DEBUG_GTK */

}
