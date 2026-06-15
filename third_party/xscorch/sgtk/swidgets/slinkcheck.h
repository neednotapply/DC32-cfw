/* $Header: /fridge/cvs/xscorch/sgtk/swidgets/slinkcheck.h,v 1.8 2009-04-26 17:39:54 jacob Exp $ */
/*

   xscorch - slinkcheck.h     Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Scorch checkbox widgets


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
#ifndef __slinkcheck_h_included
#define __slinkcheck_h_included


#include <sgtk.h>
#include <gtk/gtk.h>
#include <gtk/gtkcheckbutton.h>


/* typecasts */
#define  SC_LINK_CHECK(obj)          G_TYPE_CHECK_INSTANCE_CAST(obj, sc_link_check_get_type(), ScLinkCheck)
#define  SC_LINK_CHECK_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST(klass, sc_link_check_get_type(), ScLinkCheckClass)
#define  IS_SC_LINK_CHECK(obj)       G_TYPE_CHECK_INSTANCE_TYPE(obj, sc_link_check_get_type())


/* The ScLinkCheck structure */
typedef struct _ScLinkCheck {
   GtkCheckButton parent;
   bool *value;
} ScLinkCheck;


/* ScLinkCheckClass structure */
typedef struct _ScLinkCheckClass {
   GtkCheckButtonClass parent_class;
   void (*modified)(ScLinkCheck *spin, gpointer data);
} ScLinkCheckClass;


/* LinkCheck initialisation and execution */
GType sc_link_check_get_type(void);
GtkWidget *sc_link_check_new(bool *value);


#endif /* __slinkcheck_h_included */
