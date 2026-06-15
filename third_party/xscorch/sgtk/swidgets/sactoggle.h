/* $Header: /fridge/cvs/xscorch/sgtk/swidgets/sactoggle.h,v 1.9 2011-08-01 00:01:42 jacob Exp $ */
/*

   xscorch - sactoggle.h      Copyright(c) 2001-2003 Jacob Luna Lundberg
                              Copyright(c) 2001-2003 Justin David Smith
   jacob(at)gnifty.net        http://www.gnifty.net/
   justins(at)chaos2.org      http://chaos2.org/~justins

   Header for scorched toggle button widgets.


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
#ifndef __sactoggle_h_included
#define __sactoggle_h_included


/* Some definitions are needed. */
#include <sgtk.h>
#include <sactiveconsole.h>
#include <gtk/gtk.h>


#define  SC_AC_TOGGLE(obj)         G_TYPE_CHECK_INSTANCE_CAST(obj, sc_ac_toggle_get_type(), ScACToggle)
#define  SC_AC_TOGGLE_CLASS(klass) G_TYPE_CHECK_CLASS_CAST(klass, sc_ac_toggle_get_type(), ScACToggleClass)
#define  IS_SC_AC_TOGGLE(obj)      G_TYPE_CHECK_INSTANCE_TYPE(obj, sc_ac_toggle_get_type())


typedef struct _ScACToggle {
   ScGadget gadget;
   gboolean state;
} ScACToggle;


typedef struct _ScACToggleClass {
   ScGadgetClass parent_class;
} ScACToggleClass;


/* Functions for toggle instantiation. */
GType sc_ac_toggle_get_type(void);
ScGadget *sc_ac_toggle_new(gint x, gint y, gint width, gint height);


/* Functions for toggle state. */
void sc_ac_toggle_set(ScACToggle *toggle, gboolean newstate);
gboolean sc_ac_toggle_get(const ScACToggle *toggle);


#endif /* __sactoggle_h_included */
