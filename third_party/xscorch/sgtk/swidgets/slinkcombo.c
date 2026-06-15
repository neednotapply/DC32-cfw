/* $Header: /fridge/cvs/xscorch/sgtk/swidgets/slinkcombo.c,v 1.19 2009-06-02 07:37:43 jacob Exp $ */
/*

   xscorch - slinkcombo.c     Copyright(c) 2000-2003 Justin David Smith
                              Copyright(c) 2006 Jacob Luna Lundberg
   justins(at)chaos2.org      http://chaos2.org/

   Scorched combolist widgets


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

#include <slinkcombo.h>

#include <gdk/gdkkeysyms.h>
#include <libj/jstr/libjstr.h>



enum _ScLinkComboSignals {
   MODIFIED_SIGNAL,
   LAST_SIGNAL
};
static guint _sc_link_combo_signals[LAST_SIGNAL] = { 0 };



static void _sc_link_combo_set(ScLinkCombo *combo) {

   ScLinkComboData *combo_data;
   gint i;

   combo_data = g_object_get_data(G_OBJECT(combo), "user_data");
   if(combo_data == NULL) return;

   i = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
   if(i >= 0) {
      if(combo_data->value != NULL) *combo_data->value = i;
      g_signal_emit(GTK_OBJECT(combo), _sc_link_combo_signals[MODIFIED_SIGNAL], 0);
   }

}



static void _sc_link_combo_class_init(ScLinkComboClass *klass) {

   GtkObjectClass *object_class = (GtkObjectClass *)klass;

   _sc_link_combo_signals[MODIFIED_SIGNAL] =
      g_signal_new("modified",                     /* Signal name */
                   G_TYPE_FROM_CLASS(object_class),/* Type of object the signal applies to */
                   G_SIGNAL_RUN_LAST,              /* Signal flags; run this signal last */
                   offsetof(ScLinkComboClass, modified),
                                                   /* Offset to signal handler in class */
                   NULL,                           /* Signal accumulator function */
                   NULL,                           /* Signal accumulator data */
                   g_cclosure_marshal_VOID__VOID,  /* Marshal function for this signal */
                   G_TYPE_NONE,                    /* Return type for the marshaller */
                   0                               /* Number of extra parametres to pass */
                  );

   klass->modified = NULL;

}



static void _sc_link_combo_init_obj(ScLinkCombo *combo) {

   g_signal_connect(G_OBJECT(combo), "changed", (GCallback)_sc_link_combo_set, NULL);

}



GType sc_link_combo_get_type(void) {

   static GType sc_link_combo_type = 0;

   if(sc_link_combo_type == 0) {
      static const GTypeInfo sc_link_combo_info = {
         sizeof(ScLinkComboClass),        /* Size of the class object */
         NULL,                            /* Base initializer */
         NULL,                            /* Base finalizer */
         (GClassInitFunc)_sc_link_combo_class_init,
                                          /* Class initializer */
         NULL,                            /* Class finalizer */
         NULL,                            /* Class data pointer */
         sizeof(ScLinkCombo),             /* Size of an instance object */
         0,                               /* Number of preallocs */
         (GInstanceInitFunc)_sc_link_combo_init_obj,
                                          /* Instance initializer */
         NULL                             /* Value table */
      };
      sc_link_combo_type = g_type_register_static(gtk_combo_box_get_type(), "ScLinkCombo",
                                                  &sc_link_combo_info, 0);
   }

   return(sc_link_combo_type);

}



GtkWidget *sc_link_combo_new(int *value, const char **entries) {

   ScLinkComboData *combo_data;
   ScLinkCombo *combo;
   GtkListStore *store;
   GtkTreeIter iter;
   GtkCellRenderer *cell;
   gint i;

   combo_data = (ScLinkComboData *)malloc(sizeof(ScLinkComboData));
   g_return_val_if_fail(combo_data != NULL, NULL);
   combo_data->entries = entries;
   combo_data->value = value;

   combo = g_object_new(sc_link_combo_get_type(), NULL);
   g_return_val_if_fail(combo != NULL, NULL);

   store = gtk_list_store_new(1, G_TYPE_STRING);
   for(i = 0; entries[i] != NULL; ++i) {
      gtk_list_store_insert_with_values(store, &iter, i, 0, (gchar *)entries[i], -1);
   }
   gtk_combo_box_set_model(GTK_COMBO_BOX(combo), GTK_TREE_MODEL(store));
   g_object_unref(G_OBJECT(store));

   cell = gtk_cell_renderer_text_new();
   gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), cell, TRUE);
   gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), cell, "text", 0, NULL);

   gtk_widget_set_size_request(GTK_WIDGET(combo), 130, -1);
   g_object_set_data(G_OBJECT(GTK_COMBO_BOX(combo)), "user_data", combo_data);
   gtk_combo_box_set_active(GTK_COMBO_BOX(combo), *value);

   return(GTK_WIDGET(combo));

}
