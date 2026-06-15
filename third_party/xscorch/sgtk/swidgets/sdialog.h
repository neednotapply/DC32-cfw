/* $Header: /fridge/cvs/xscorch/sgtk/swidgets/sdialog.h,v 1.11 2009-04-26 17:39:53 jacob Exp $ */
/*

   xscorch - sdialog.h        Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Dialog widget header


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
#ifndef __sdialog_h_included
#define __sdialog_h_included


#include <sgtk.h>
#include <gtk/gtk.h>
#include <gtk/gtkwindow.h>


/* Dialog string buffer size */
#define  SC_DIALOG_STRING_BUFFER 0x1000


/* Dialog typecasts */
#define  SC_DIALOG(obj)          G_TYPE_CHECK_INSTANCE_CAST(obj, sc_dialog_get_type(), ScDialog)
#define  SC_DIALOG_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST(klass, sc_dialog_get_type(), ScDialogClass)
#define  IS_SC_DIALOG(obj)       G_TYPE_CHECK_INSTANCE_TYPE(obj, sc_dialog_get_type())


/* Dialog attributes */
#define  SC_DIALOG_OK            (1 << 0)
#define  SC_DIALOG_CANCEL        (1 << 1)
#define  SC_DIALOG_CLOSE         (1 << 2)
#define  SC_DIALOG_YES           (1 << 3)
#define  SC_DIALOG_NO            (1 << 4)
#define  SC_DIALOG_APPLY         (1 << 5)
#define  SC_DIALOG_NONMODAL      (1 << 6)
#define  SC_DIALOG_NO_GRID       (1 << 7)
/* The following flags should not be used by the caller */
#define  SC_DIALOG_DELAY_DESTROY (1 << 8)

typedef enum _ScDialogState {
   SC_DIALOG_WAITING = 0,
   SC_DIALOG_ACCEPTED,
   SC_DIALOG_REJECTED,
   SC_DIALOG_DESTROYED
} ScDialogState;


/* The ScDialog structure */
typedef struct _ScDialog {
   GtkWindow parent;
   GtkWidget *grid;
   ScDialogState *state;
   guint flags;
} ScDialog;


/* ScDialogClass structure */
typedef struct _ScDialogClass {
   GtkWindowClass parent_class;
   gboolean (*apply)(ScDialog *dlg);
} ScDialogClass;


/* Dialog initialisation and execution */
GType sc_dialog_get_type(void);
GtkWidget *sc_dialog_new(const char *title, const char *msgtext, guint flags);
gboolean sc_dialog_run(ScDialog *dlg);
void sc_dialog_show(ScDialog *dlg);
void sc_dialog_grid_attach(ScDialog *dlg, GtkWidget *widget, int row, int col);
void sc_dialog_grid_attach_label(ScDialog *dlg, const char *msg, int row, int col);


/* Special types of dialogs */
void sc_dialog_message(const char *title, const char *msgtext);
gboolean sc_dialog_query(const char *title, const char *msgtext);
void sc_dialog_error(const char *msgappend);
void sc_dialog_text(const char *filename, GdkFont *normal_font, GdkFont *italic_font, GdkFont *bold_font);
void sc_dialog_text_buffer(const char *title, const char *buffer, GdkFont *normal_font, GdkFont *italic_font, GdkFont *bold_font);


#endif /* __sdialog_h_included */
