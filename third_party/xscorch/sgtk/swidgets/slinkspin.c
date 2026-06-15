/* $Header: /fridge/cvs/xscorch/sgtk/swidgets/slinkspin.c,v 1.15 2009-04-26 17:39:54 jacob Exp $ */
/*

   xscorch - slinkspin.c      Copyright(c) 2000-2003 Justin David Smith
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

#include <slinkspin.h>

#include <gdk/gdkkeysyms.h>



static GtkSpinButtonClass *parent_class;



enum _ScLinkSpinSignals {
   MODIFIED_SIGNAL,
   LAST_SIGNAL
};
static guint _sc_link_spin_signals[LAST_SIGNAL] = { 0 };
static guint _sc_link_spinf_signals[LAST_SIGNAL] = { 0 };



static void _sc_link_spin_set(GtkEditable *ed) {

   ScLinkSpin *spin = SC_LINK_SPIN(ed);
   
   if(spin->value != NULL) {
      *spin->value = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ed));
   }
   g_signal_emit(GTK_OBJECT(spin), _sc_link_spin_signals[MODIFIED_SIGNAL], 0);

}



static void _sc_link_spin_class_init(ScLinkSpinClass *klass) {

   GtkObjectClass *object_class = (GtkObjectClass *)klass;

   parent_class = g_type_class_peek(gtk_spin_button_get_type());

   _sc_link_spin_signals[MODIFIED_SIGNAL] =
      g_signal_new("modified",                     /* Signal name */
                   G_TYPE_FROM_CLASS(object_class),/* Type of object the signal applies to */
                   G_SIGNAL_RUN_LAST,              /* Signal flags; run this signal last */
                   offsetof(ScLinkSpinClass, modified),
                                                   /* Offset to signal handler in class */
                   NULL,                           /* Signal accumulator function */
                   NULL,                           /* Signal accumulator data */
                   g_cclosure_marshal_VOID__VOID,  /* Marshal function for this signal */
                   G_TYPE_NONE,                    /* Return type for the marshaller */
                   0                               /* Number of extra parametres to pass */
                  );

   klass->modified = NULL;

}



static void _sc_link_spin_init_obj(ScLinkSpin *spin) {

   spin->value = NULL;
   g_signal_connect(G_OBJECT(spin), "changed", (GCallback)_sc_link_spin_set, NULL);

}



GType sc_link_spin_get_type(void) {

   static GType sc_link_spin_type = 0;

   if(sc_link_spin_type == 0) {
      static const GTypeInfo sc_link_spin_info = {
         sizeof(ScLinkSpinClass),         /* Size of the class object */
         NULL,                            /* Base initializer */
         NULL,                            /* Base finalizer */
         (GClassInitFunc)_sc_link_spin_class_init,
                                          /* Class initializer */
         NULL,                            /* Class finalizer */
         NULL,                            /* Class data pointer */
         sizeof(ScLinkSpin),              /* Size of an instance object */
         0,                               /* Number of preallocs */
         (GInstanceInitFunc)_sc_link_spin_init_obj,
                                          /* Instance initializer */
         NULL                             /* Value table */
      };
      sc_link_spin_type = g_type_register_static(gtk_spin_button_get_type(), "ScLinkSpin",
                                                 &sc_link_spin_info, 0);
   }

   return(sc_link_spin_type);

}



GtkWidget *sc_link_spin_new(int *value, int min, int max, int step) {

   GtkAdjustment *spinadjust;
   ScLinkSpin *spin;

   spin = g_object_new(sc_link_spin_get_type(), NULL);
   g_return_val_if_fail(spin != NULL, NULL);

   spinadjust = (GtkAdjustment *)gtk_adjustment_new(*value, min, max, step, step, step);
   gtk_spin_button_configure(GTK_SPIN_BUTTON(spin), spinadjust, 1, 0);
   gtk_widget_set_size_request(GTK_WIDGET(spin), 80, -1);
   gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(spin), TRUE);
   spin->value = value;

   return(GTK_WIDGET(spin));

}



static void _sc_link_spinf_set(GtkEditable *ed) {

   ScLinkSpinF *spin = SC_LINK_SPINF(ed);

   if(spin->value != NULL) {
      *spin->value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin));
   }
   g_signal_emit(GTK_OBJECT(spin), _sc_link_spinf_signals[MODIFIED_SIGNAL], 0);

}



static void _sc_link_spinf_class_init(ScLinkSpinFClass *klass) {

   GtkObjectClass *object_class = (GtkObjectClass *)klass;

   parent_class = g_type_class_peek(gtk_spin_button_get_type());

   _sc_link_spinf_signals[MODIFIED_SIGNAL] =
      g_signal_new("modified",                     /* Signal name */
                   G_TYPE_FROM_CLASS(object_class),/* Type of object the signal applies to */
                   G_SIGNAL_RUN_LAST,              /* Signal flags; run this signal last */
                   offsetof(ScLinkSpinFClass, modified),
                                                   /* Offset to signal handler in class */
                   NULL,                           /* Signal accumulator function */
                   NULL,                           /* Signal accumulator data */
                   g_cclosure_marshal_VOID__VOID,  /* Marshal function for this signal */
                   G_TYPE_NONE,                    /* Return type for the marshaller */
                   0                               /* Number of extra parametres to pass */
                  );

   klass->modified = NULL;

}



static void _sc_link_spinf_init_obj(ScLinkSpinF *spin) {

   spin->value = NULL;
   g_signal_connect(G_OBJECT(spin), "changed", (GCallback)_sc_link_spinf_set, NULL);

}



GType sc_link_spinf_get_type(void) {

   static GType sc_link_spinf_type = 0;

   if(sc_link_spinf_type == 0) {
      static const GTypeInfo sc_link_spinf_info = {
         sizeof(ScLinkSpinFClass),        /* Size of the class object */
         NULL,                            /* Base initializer */
         NULL,                            /* Base finalizer */
         (GClassInitFunc)_sc_link_spinf_class_init,
                                          /* Class initializer */
         NULL,                            /* Class finalizer */
         NULL,                            /* Class data pointer */
         sizeof(ScLinkSpinF),             /* Size of an instance object */
         0,                               /* Number of preallocs */
         (GInstanceInitFunc)_sc_link_spinf_init_obj,
                                          /* Instance initializer */
         NULL                             /* Value table */
      };
      sc_link_spinf_type = g_type_register_static(gtk_spin_button_get_type(), "ScLinkSpinF",
                                                  &sc_link_spinf_info, 0);
   }

   return(sc_link_spinf_type);

}



GtkWidget *sc_link_spinf_new(double *value, double min, double max, double step) {

   GtkAdjustment *spinadjust;
   ScLinkSpinF *spin;

   spin = g_object_new(sc_link_spinf_get_type(), NULL);
   g_return_val_if_fail(spin != NULL, NULL);

   spinadjust = (GtkAdjustment *)gtk_adjustment_new(*value, min, max, step, step, step);
   gtk_spin_button_configure(GTK_SPIN_BUTTON(spin), spinadjust, 1, 3);
   gtk_widget_set_size_request(GTK_WIDGET(spin), 80, -1);
   gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(spin), FALSE);
   spin->value = value;

   return(GTK_WIDGET(spin));

}
