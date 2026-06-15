/* $Header: /fridge/cvs/xscorch/sgtk/swidgets/sactiveconsole.h,v 1.18 2009-05-25 04:03:49 jacob Exp $ */
/*

   xscorch - sactiveconsole.h Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Display an active console (a console with "hotspots")


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
#ifndef __sactiveconsole_h_included
#define __sactiveconsole_h_included


#include <sgtk.h>
#include <gtk/gtk.h>
#include <sconsole.h>


/***  Forward Declaractions  ***/


struct _ScGadget;


/***  Active Consoles  ***/


#define  SC_ACTIVE_CONSOLE(obj)         G_TYPE_CHECK_INSTANCE_CAST(obj, sc_active_console_get_type(), ScActiveConsole)
#define  SC_ACTIVE_CONSOLE_CLASS(klass) G_TYPE_CHECK_CLASS_CAST(klass, sc_active_console_get_type(), ScActiveConsoleClass)
#define  IS_SC_ACTIVE_CONSOLE(obj)      G_TYPE_CHECK_INSTANCE_TYPE(obj, sc_active_console_get_type())


typedef struct _ScActiveConsoleSpot {
   gint x;
   gint y;
   gint width;
   gint height;
   struct _ScGadget *gadget;
   gpointer data;
} ScActiveConsoleSpot;


typedef struct _ScActiveConsole {
   ScConsole console;
   GList *spots;
   GList *current;
   gboolean allowkeyboard;
} ScActiveConsole;


typedef struct _ScActiveConsoleClass {
   ScConsoleClass parent_class;
   void     (*activate)(ScActiveConsole *cons);
   gboolean (*button_press_spot)(ScActiveConsole *cons, ScActiveConsoleSpot *spot, GdkEventButton *event);
   gboolean (*button_release_spot)(ScActiveConsole *cons, ScActiveConsoleSpot *spot, GdkEventButton *event);
   gboolean (*key_press_spot)(ScActiveConsole *cons, ScActiveConsoleSpot *spot, GdkEventKey *event);
   gboolean (*key_release_spot)(ScActiveConsole *cons, ScActiveConsoleSpot *spot, GdkEventKey *event);
   gboolean (*select_spot)(ScActiveConsole *cons, ScActiveConsoleSpot *spot);
   gboolean (*enter_spot)(ScActiveConsole *cons, ScActiveConsoleSpot *spot);
   gboolean (*leave_spot)(ScActiveConsole *cons, ScActiveConsoleSpot *spot);
} ScActiveConsoleClass;


/***  Gadgets  ***/


#define  SC_GADGET(obj)                 G_TYPE_CHECK_INSTANCE_CAST(obj, sc_gadget_get_type(), ScGadget)
#define  SC_GADGET_CLASS(klass)         G_TYPE_CHECK_CLASS_CAST(klass, sc_gadget_get_type(), ScGadgetClass)
#define  IS_SC_GADGET(obj)              G_TYPE_CHECK_INSTANCE_TYPE(obj, sc_gadget_get_type())

typedef struct _ScGadget {
   GtkObject object;
   ScActiveConsole *console;
   ScActiveConsoleSpot *spot;
   gint x;
   gint y;
   gint width;
   gint height;
} ScGadget;


typedef struct _ScGadgetClass {
   GtkObjectClass parent_class;
   void     (*paint)(ScGadget *gadget);
   gboolean (*button_press_spot)(ScGadget *gadget, GdkEventButton *event);
   gboolean (*button_release_spot)(ScGadget *gadget, GdkEventButton *event);
   gboolean (*key_press_spot)(ScGadget *gadget, GdkEventKey *event);
   gboolean (*key_release_spot)(ScGadget *gadget, GdkEventKey *event);
   gboolean (*select_spot)(ScGadget *gadget);
   gboolean (*enter_spot)(ScGadget *gadget);
   gboolean (*leave_spot)(ScGadget *gadget);
} ScGadgetClass;


/***  Function Prototypes  ***/


GType sc_active_console_get_type(void);
GtkWidget *sc_active_console_new(gint x, gint y, gint width, gint height, ScConsoleStyle style,
                                 GdkFont *font, GdkFont *boldfont);
void sc_active_console_init(ScActiveConsole *cons, gint x, gint y, gint width, gint height, ScConsoleStyle style,
                            GdkFont *font, GdkFont *boldfont);
void sc_active_console_set_allow_keyboard(ScActiveConsole *cons, gboolean allowkeyboard);


GType sc_gadget_get_type(void);
void sc_gadget_get_extents(ScGadget *gadget, GdkRectangle *bounds);


void sc_active_console_add_spot(ScActiveConsole *cons, gint x, gint y, gint width, gint height, gpointer data);
void sc_active_console_add_row_spot(ScActiveConsole *cons, gint row, gpointer data);
void sc_active_console_add_gadget_spot(ScActiveConsole *cons, ScGadget *gadget, gpointer data);
gboolean sc_active_console_detach_spot(ScActiveConsole *cons);
void sc_active_console_detach_all_spots(ScActiveConsole *cons);


#endif /* __sactiveconsole_h_included */
