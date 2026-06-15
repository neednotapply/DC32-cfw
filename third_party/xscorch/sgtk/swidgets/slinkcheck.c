/* $Header: /fridge/cvs/xscorch/sgtk/swidgets/slinkcheck.c,v 1.14 2009-04-26 17:39:54 jacob Exp $ */
/*

   xscorch - slinkcheck.c     Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Scorched checkbox widgets


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

#include <slinkcheck.h>

#include <gdk/gdkkeysyms.h>



static GtkCheckButtonClass *parent_class;



enum _ScLinkCheckSignals {
   MODIFIED_SIGNAL,
   LAST_SIGNAL
};
static guint _sc_link_check_signals[LAST_SIGNAL] = { 0 };



static void _sc_link_check_set(GtkToggleButton *toggle) {

   ScLinkCheck *check = SC_LINK_CHECK(toggle);

   if(check->value != NULL) {
      *check->value = gtk_toggle_button_get_active(toggle);
   }
   g_signal_emit(GTK_OBJECT(check), _sc_link_check_signals[MODIFIED_SIGNAL], 0);

}



static void _sc_link_check_class_init(ScLinkCheckClass *klass) {

   GtkObjectClass *object_class = (GtkObjectClass *)klass;

   parent_class = g_type_class_peek(gtk_check_button_get_type());

   _sc_link_check_signals[MODIFIED_SIGNAL] =
      g_signal_new("modified",                     /* Signal name */
                   G_TYPE_FROM_CLASS(object_class),/* Type of object the signal applies to */
                   G_SIGNAL_RUN_LAST,              /* Signal flags; run this signal last */
                   offsetof(ScLinkCheckClass, modified),
                                                   /* Offset to signal handler in class */
                   NULL,                           /* Signal accumulator function */
                   NULL,                           /* Signal accumulator data */
                   g_cclosure_marshal_VOID__VOID,  /* Marshal function for this signal */
                   G_TYPE_NONE,                    /* Return type for the marshaller */
                   0                               /* Number of extra parametres to pass */
                  );

   klass->modified = NULL;
   
}



static void _sc_link_check_init_obj(ScLinkCheck *check) {

   check->value = NULL;

   g_signal_connect(G_OBJECT(check), "toggled", (GCallback)_sc_link_check_set, NULL);
                        
}



GType sc_link_check_get_type(void) {

   static GType sc_link_check_type = 0;

   if(sc_link_check_type == 0) {
      static const GTypeInfo sc_link_check_info = {
         sizeof(ScLinkCheckClass),        /* Size of the class object */
         NULL,                            /* Base initializer */
         NULL,                            /* Base finalizer */
         (GClassInitFunc)_sc_link_check_class_init,
                                          /* Class initializer */
         NULL,                            /* Class finalizer */
         NULL,                            /* Class data pointer */
         sizeof(ScLinkCheck),             /* Size of an instance object */
         0,                               /* Number of preallocs */
         (GInstanceInitFunc)_sc_link_check_init_obj,
                                          /* Instance initializer */
         NULL                             /* Value table */
      };
      sc_link_check_type = g_type_register_static(gtk_check_button_get_type(), "ScLinkCheck",
                                                  &sc_link_check_info, 0);
   }

   return(sc_link_check_type);

}



GtkWidget *sc_link_check_new(bool *value) {

   ScLinkCheck *check;

   check = g_object_new(sc_link_check_get_type(), NULL);
   g_return_val_if_fail(check != NULL, NULL);

   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), *value);
   check->value = value;

   return(GTK_WIDGET(check));

}
