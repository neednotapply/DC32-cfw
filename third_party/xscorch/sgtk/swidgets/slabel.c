/* $Header: /fridge/cvs/xscorch/sgtk/swidgets/slabel.c,v 1.13 2009-04-26 17:39:53 jacob Exp $ */
/*

   xscorch - slabel.c         Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Scorched label widgets


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

#include <slabel.h>

#include <gdk/gdkkeysyms.h>



GType sc_label_get_type(void) {

   static GType sc_label_type = 0;

   if(sc_label_type == 0) {
      static const GTypeInfo sc_label_info = {
         sizeof(ScLabelClass),            /* Size of the class object */
         NULL,                            /* Base initializer */
         NULL,                            /* Base finalizer */
         NULL,                            /* Class initializer */
         NULL,                            /* Class finalizer */
         NULL,                            /* Class data pointer */
         sizeof(ScLabel),                 /* Size of an instance object */
         0,                               /* Number of preallocs */
         NULL,                            /* Instance initializer */
         NULL                             /* Value table */
      };
      sc_label_type = g_type_register_static(gtk_label_get_type(), "ScLabel",
                                             &sc_label_info, 0);
   }

   return(sc_label_type);

}



GtkWidget *sc_label_new(const char *text) {

   ScLabel *label;

   label = g_object_new(sc_label_get_type(), NULL);
   g_return_val_if_fail(label != NULL, NULL);

   gtk_label_set_text(GTK_LABEL(label), text);
   gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

   return(GTK_WIDGET(label));

}
