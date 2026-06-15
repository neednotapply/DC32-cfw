/* $Header: /fridge/cvs/xscorch/sgtk/sdialog-gtk.h,v 1.7 2011-07-31 19:05:20 jacob Exp $ */
/*
   
   xscorch - sdialog-gtk.h    Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/
    
   GTK dialogue helper functions
    

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
#ifndef __sdialog_gtk_h_included
#define __sdialog_gtk_h_included


/* The order of includes does matter! */
#include <swindow-gtk.h>
#include <sdialog.h>
#include <slabel.h>
#include <sgame/shelp.h>
#include <sgame/sconfig.h>
#include <libj/jstr/libjstr.h>


/* Tooltips and dialogue helpers */
static inline GtkWidget *tooltip(sc_window_gtk *w, const char *help, GtkWidget *widget) {

   /* TOOLTIPS changed from an ass to a mule in GTK 2.12 */
   if(w->c->options.tooltips) {
      gtk_widget_set_tooltip_text(widget, help);
      gtk_widget_set_has_tooltip(widget, TRUE);
   } else {
      gtk_widget_set_has_tooltip(widget, FALSE);
   }

   return(widget);

}


static inline void attach_label(ScDialog *dialog, sc_window_gtk *w, const char *help, const char *field, int row) {
   
   char buf[SC_GTK_STRING_BUFFER];
   sbprintf(buf, sizeof(buf), "%s:", field);
   sc_dialog_grid_attach(dialog, tooltip(w, help, sc_label_new(buf)), row, 0);
         
   
}


static inline void attach_widget(ScDialog *dialog, sc_window_gtk *w, const char *help, GtkWidget *widget, int row) {
   
   sc_dialog_grid_attach(dialog, tooltip(w, help, widget), row, 1);
         
   
}


static inline void attach_option_help(ScDialog *dialog, sc_window_gtk *w, const char *field, const char *help, GtkWidget *widget, int *row) {

   attach_label (dialog, w, help, field,  *row);
   attach_widget(dialog, w, help, widget, *row);
   ++(*row);

}


static inline void attach_option(ScDialog *dialog, sc_window_gtk *w, const char *field, GtkWidget *widget, int *row) {

   char help[SC_GTK_STRING_BUFFER];
   sc_help_text(help, sizeof(help), field);
   attach_option_help(dialog, w, field, help, widget, row);

}


#endif /* __sdialog_gtk_h_included */
