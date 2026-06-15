/* $Header: /fridge/cvs/xscorch/sgtk/swidgets/slinkcombo.h,v 1.8 2009-06-02 07:37:43 jacob Exp $ */
/*

   xscorch - slinkcombo.h     Copyright(c) 2000-2003 Justin David Smith
                              Copyright(c) 2006 Jacob Luna Lundberg
   justins(at)chaos2.org      http://chaos2.org/

   Scorch combolist widgets


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
#ifndef __slinkcombo_h_included
#define __slinkcombo_h_included


#include <sgtk.h>

#include <gtk/gtk.h>


/* typecasts */
#define  SC_LINK_COMBO(obj)          GTK_COMBO_BOX_CAST(obj, sc_link_combo_get_type(), ScLinkCombo)
#define  SC_LINK_COMBO_CLASS(klass)  GTK_COMBO_BOX_CLASS_CAST(klass, sc_link_combo_get_type(), ScLinkComboClass)
#define  IS_SC_LINK_COMBO(obj)       GTK_COMBO_BOX_TYPE(obj, sc_link_combo_get_type())


/* The ScLinkCombo structure */
typedef struct _ScLinkCombo {
   GtkComboBox parent;
} ScLinkCombo;

/* ScLinkComboClass structure */
typedef struct _ScLinkComboClass {
   GtkComboBoxClass parent_class;
   void (*modified)(ScLinkCombo *spin, gpointer data);
} ScLinkComboClass;


/* Utility data struct for GTK to pass around */
typedef struct _ScLinkComboData {
   const char **entries;
   int *value;
} ScLinkComboData;


/* LinkCombo initialisation and execution */
GType sc_link_combo_get_type(void);
GtkWidget *sc_link_combo_new(int *value, const char **entries);


#endif /* __slinkcombo_h_included */
