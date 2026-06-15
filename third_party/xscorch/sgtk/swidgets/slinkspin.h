/* $Header: /fridge/cvs/xscorch/sgtk/swidgets/slinkspin.h,v 1.7 2009-04-26 17:39:54 jacob Exp $ */
/*

   xscorch - slinkspin.h      Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Scorch spin widgets


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
#ifndef __slinkspin_h_included
#define __slinkspin_h_included


#include <sgtk.h>
#include <gtk/gtk.h>
#include <gtk/gtkspinbutton.h>


/* typecasts */
#define  SC_LINK_SPIN(obj)          G_TYPE_CHECK_INSTANCE_CAST(obj, sc_link_spin_get_type(), ScLinkSpin)
#define  SC_LINK_SPIN_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST(klass, sc_link_spin_get_type(), ScLinkSpinClass)
#define  IS_SC_LINK_SPIN(obj)       G_TYPE_CHECK_INSTANCE_TYPE(obj, sc_link_spin_get_type())

#define  SC_LINK_SPINF(obj)         G_TYPE_CHECK_INSTANCE_CAST(obj, sc_link_spinf_get_type(), ScLinkSpinF)
#define  SC_LINK_SPINF_CLASS(klass) G_TYPE_CHECK_CLASS_CAST(klass, sc_link_spinf_get_type(), ScLinkSpinFClass)
#define  IS_SC_LINK_SPINF(obj)      G_TYPE_CHECK_INSTANCE_TYPE(obj, sc_link_spinf_get_type())


/* The ScLinkSpin structure */
typedef struct _ScLinkSpin {
   GtkSpinButton parent;
   int *value;
} ScLinkSpin;


/* ScLinkSpinClass structure */
typedef struct _ScLinkSpinClass {
   GtkSpinButtonClass parent_class;
   void (*modified)(ScLinkSpin *spin, gpointer data);
} ScLinkSpinClass;


/* The ScLinkSpinF structure */
typedef struct _ScLinkSpinF {
   GtkSpinButton parent;
   double *value;
} ScLinkSpinF;


/* ScLinkSpinFClass structure */
typedef struct _ScLinkSpinFClass {
   GtkSpinButtonClass parent_class;
   void (*modified)(ScLinkSpinF *spin, gpointer data);
} ScLinkSpinFClass;


/* LinkSpin initialisation and execution */
GType sc_link_spin_get_type(void);
GType sc_link_spinf_get_type(void);
GtkWidget *sc_link_spin_new(int *value, int min, int max, int step);
GtkWidget *sc_link_spinf_new(double *value, double min, double max, double step);


#endif /* __slinkspin_h_included */
