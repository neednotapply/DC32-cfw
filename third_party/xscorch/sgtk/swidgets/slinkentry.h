/* $Header: /fridge/cvs/xscorch/sgtk/swidgets/slinkentry.h,v 1.9 2009-04-26 17:39:54 jacob Exp $ */
/*

   xscorch - slinkentry.h     Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Scorch entry widgets


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
#ifndef __slinkentry_h_included
#define __slinkentry_h_included


#include <sgtk.h>
#include <gtk/gtk.h>
#include <gtk/gtkentry.h>


/* typecasts */
#define  SC_LINK_ENTRY(obj)          G_TYPE_CHECK_INSTANCE_CAST(obj, sc_link_entry_get_type(), ScLinkEntry)
#define  SC_LINK_ENTRY_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST(klass, sc_link_entry_get_type(), ScLinkEntryClass)
#define  IS_SC_LINK_ENTRY(obj)       G_TYPE_CHECK_INSTANCE_TYPE(obj, sc_link_entry_get_type())


/* The ScLinkEntry structure */
typedef struct _ScLinkEntry {
   GtkEntry parent;
   char *value;
   int maxlen;
} ScLinkEntry;


/* ScLinkEntryClass structure */
typedef struct _ScLinkEntryClass {
   GtkEntryClass parent_class;
   void (*modified)(ScLinkEntry *spin, gpointer data);
} ScLinkEntryClass;


/* LinkEntry initialisation and execution */
GType sc_link_entry_get_type(void);
GtkWidget *sc_link_entry_new(char *value, int maxlen);
void sc_link_entry_set_text(ScLinkEntry *entry, const char *value);


#endif /* __slinkentry_h_included */
