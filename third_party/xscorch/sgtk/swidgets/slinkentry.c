/* $Header: /fridge/cvs/xscorch/sgtk/swidgets/slinkentry.c,v 1.21 2009-04-26 17:39:54 jacob Exp $ */
/*

   xscorch - slinkentry.c     Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Scorched entrybox widgets


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

#include <slinkentry.h>

#include <gdk/gdkkeysyms.h>
#include <libj/jstr/libjstr.h>



static GtkEntryClass *parent_class;



enum _ScLinkEntrySignals {
   MODIFIED_SIGNAL,
   LAST_SIGNAL
};
static guint _sc_link_entry_signals[LAST_SIGNAL] = { 0 };



static void _sc_link_entry_set(GtkEditable *ed) {

   ScLinkEntry *entry = SC_LINK_ENTRY(ed);

   if(entry->value != NULL) {
      strcopyb(entry->value, gtk_entry_get_text(GTK_ENTRY(ed)), entry->maxlen);
   }
   g_signal_emit(GTK_OBJECT(entry), _sc_link_entry_signals[MODIFIED_SIGNAL], 0);

}



static void _sc_link_entry_class_init(ScLinkEntryClass *klass) {

   GtkObjectClass *object_class = (GtkObjectClass *)klass;

   parent_class = g_type_class_peek(gtk_entry_get_type());

   _sc_link_entry_signals[MODIFIED_SIGNAL] =
      g_signal_new("modified",                     /* Signal name */
                   G_TYPE_FROM_CLASS(object_class),/* Type of object the signal applies to */
                   G_SIGNAL_RUN_LAST,              /* Signal flags; run this signal last */
                   offsetof(ScLinkEntryClass, modified),
                                                   /* Offset to signal handler in class */
                   NULL,                           /* Signal accumulator function */
                   NULL,                           /* Signal accumulator data */
                   g_cclosure_marshal_VOID__VOID,  /* Marshal function for this signal */
                   G_TYPE_NONE,                    /* Return type for the marshaller */
                   0                               /* Number of extra parametres to pass */
                  );

   klass->modified = NULL;
   
}



static void _sc_link_entry_init_obj(ScLinkEntry *entry) {

   entry->value = NULL;
   entry->maxlen = 0;

   g_signal_connect(G_OBJECT(entry), "changed", (GCallback)_sc_link_entry_set, NULL);
                        
}



GType sc_link_entry_get_type(void) {

   static GType sc_link_entry_type = 0;

   if(sc_link_entry_type == 0) {
      static const GTypeInfo sc_link_entry_info = {
         sizeof(ScLinkEntryClass),        /* Size of the class object */
         NULL,                            /* Base initializer */
         NULL,                            /* Base finalizer */
         (GClassInitFunc)_sc_link_entry_class_init,
                                          /* Class initializer */
         NULL,                            /* Class finalizer */
         NULL,                            /* Class data pointer */
         sizeof(ScLinkEntry),             /* Size of an instance object */
         0,                               /* Number of preallocs */
         (GInstanceInitFunc)_sc_link_entry_init_obj,
                                          /* Instance initializer */
         NULL                             /* Value table */
      };
      sc_link_entry_type = g_type_register_static(gtk_entry_get_type(), "ScLinkEntry",
                                                  &sc_link_entry_info, 0);
   }

   return(sc_link_entry_type);

}



GtkWidget *sc_link_entry_new(char *value, int maxlen) {

   ScLinkEntry *entry;

   entry = g_object_new(sc_link_entry_get_type(), NULL);
   g_return_val_if_fail(entry != NULL, NULL);

   gtk_widget_set_size_request(GTK_WIDGET(entry), 300, -1);
   gtk_entry_set_max_length(GTK_ENTRY(entry), maxlen - 1);
   gtk_entry_set_text(GTK_ENTRY(entry), value);
   entry->value = value;
   entry->maxlen = maxlen;

   return(GTK_WIDGET(entry));

}



void sc_link_entry_set_text(ScLinkEntry *entry, const char *value) {

   strcopyb(entry->value, value, entry->maxlen);
   gtk_entry_set_text(GTK_ENTRY(entry), entry->value);

}
