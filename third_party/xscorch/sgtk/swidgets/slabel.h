/* $Header: /fridge/cvs/xscorch/sgtk/swidgets/slabel.h,v 1.7 2009-04-26 17:39:54 jacob Exp $ */
/*

   xscorch - slabel.h         Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Scorch label widget


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
#ifndef __slabel_h_included
#define __slabel_h_included


#include <sgtk.h>
#include <gtk/gtk.h>
#include <gtk/gtklabel.h>


/* typecasts */
#define  SC_LABEL(obj)           G_TYPE_CHECK_INSTANCE_CAST(obj, sc_label_get_type(), ScLabel)
#define  SC_LABEL_CLASS(klass)   G_TYPE_CHECK_CLASS_CAST(klass, sc_label_get_type(), ScLabelClass)
#define  IS_SC_LABEL(obj)        G_TYPE_CHECK_INSTANCE_TYPE(obj, sc_label_get_type())


/* The ScLabel structure */
typedef struct _ScLabel {
   GtkLabel parent;
} ScLabel;


/* ScLabelClass structure */
typedef struct _ScLabelClass {
   GtkLabelClass parent_class;
} ScLabelClass;


/* Label initialisation and execution */
GType sc_label_get_type(void);
GtkWidget *sc_label_new(const char *text);


#endif /* __slabel_h_included */
