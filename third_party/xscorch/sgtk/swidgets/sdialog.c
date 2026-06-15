/* $Header: /fridge/cvs/xscorch/sgtk/swidgets/sdialog.c,v 1.32 2009-04-26 17:39:53 jacob Exp $ */
/*

   xscorch - sdialog.c        Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Scorched dialogues


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
#define  __ALLOW_DEPRECATED_GDK__

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <sdialog.h>
#include <slabel.h>

#include <gdk/gdkkeysyms.h>
#include <sutil/sgetline.h>
#include <libj/jstr/libjstr.h>



static GtkWindowClass *parent_class;



enum _ScDialogSignals {
   APPLY_SIGNAL,
   LAST_SIGNAL
};
static guint _sc_dialog_signals[LAST_SIGNAL] = { 0 };



/***  Dialog Signal Handlers  ***/



static void _sc_dialog_destroy(GtkObject *obj) {
/* sc_dialog_destroy
   Destroy a dialogue window.  If the state pointer is not NULL, then the
   state is updated to indicate the dialog has been destroyed.  The parent
   destructor will be called.  */

   ScDialog *dlg = SC_DIALOG(obj);

   /* Update state flag */
   if(dlg->state != NULL) {
      *dlg->state = SC_DIALOG_DESTROYED;
   }

   /* Call parent destructor */
   if(GTK_OBJECT_CLASS(parent_class)->destroy != NULL) {
      GTK_OBJECT_CLASS(parent_class)->destroy(obj);
   }

}



static void _sc_dialog_apply_clicked(__libj_unused GtkWidget *button, ScDialog *dlg) {
/* sc_dialog_apply_clicked
   The Apply button was clicked.  Emit the "apply" signal, causing
   any changes in the dialog window to take effect.  This button
   does not automatically close the window.  */

   assert(IS_SC_DIALOG(dlg));

   /* Emit "apply" signal, causing changes to take effect. Reference
      this dialog window while we process the signal, to ensure that
      it does not get prematurely destroyed.  */
   g_object_ref(G_OBJECT(dlg));
   g_signal_emit(GTK_OBJECT(dlg), _sc_dialog_signals[APPLY_SIGNAL], 0);
   g_object_unref(G_OBJECT(dlg));

}



static void _sc_dialog_ok_clicked(GtkWidget *button, ScDialog *dlg) {
/* sc_dialog_ok_clicked
   The OK button was clicked.  Emit the "apply" signal, then close
   out the dialog window (unless defer was selected).  */

   assert(IS_SC_DIALOG(dlg));

   /* Emit "apply" signal */
   _sc_dialog_apply_clicked(button, dlg);

   /* Update the current dialog state */
   if(dlg->state != NULL) {
      *dlg->state = SC_DIALOG_ACCEPTED;
   }
   
   /* Destroy the dialog window, unless the flags indicate we should defer. */
   if(!(dlg->flags & SC_DIALOG_DELAY_DESTROY)) {
      gtk_widget_destroy(GTK_WIDGET(dlg));
   }

}



static void _sc_dialog_cancel_clicked(__libj_unused GtkWidget *button, ScDialog *dlg) {
/* sc_dialog_cancel_clicked
   The Cancel button was clicked.  Close the dialogue window without
   emitting an "apply" signal; changes in the dialog are discarded.
   Note that Cancel is functionally equivalent to Close.  */

   assert(IS_SC_DIALOG(dlg));

   /* Update the current dialog state */
   if(dlg->state != NULL) {
      *dlg->state = SC_DIALOG_REJECTED;
   }

   /* Destroy the dialog window, unless the flags indicate we should defer. */
   if(!(dlg->flags & SC_DIALOG_DELAY_DESTROY)) {
      gtk_widget_destroy(GTK_WIDGET(dlg));
   }

}



static gint _sc_dialog_key_press(GtkWidget *widget, GdkEventKey *key) {
/* sc_dialog_key_press
   Process a keypress event in this dialog window.  */

   ScDialog *dlg = SC_DIALOG(widget);

   /* Check to see if enter or escape were pressed. */
   switch(key->keyval) {
      case GDK_Return:
      case GDK_KP_Enter:
         if(dlg->flags & SC_DIALOG_OK)          _sc_dialog_ok_clicked(widget, dlg);
         else if(dlg->flags & SC_DIALOG_YES)    _sc_dialog_ok_clicked(widget, dlg);
         else if(dlg->flags & SC_DIALOG_APPLY)  _sc_dialog_apply_clicked(widget, dlg);
         else if(dlg->flags & SC_DIALOG_CANCEL) _sc_dialog_cancel_clicked(widget, dlg);
         else if(dlg->flags & SC_DIALOG_NO)     _sc_dialog_cancel_clicked(widget, dlg);
         else if(dlg->flags & SC_DIALOG_CLOSE)  _sc_dialog_cancel_clicked(widget, dlg);
         else return(FALSE);
         return(TRUE);
      case GDK_Escape:
         if(dlg->flags & SC_DIALOG_CANCEL)      _sc_dialog_cancel_clicked(widget, dlg);
         else if(dlg->flags & SC_DIALOG_NO)     _sc_dialog_cancel_clicked(widget, dlg);
         else if(dlg->flags & SC_DIALOG_CLOSE)  _sc_dialog_cancel_clicked(widget, dlg);
         else if(dlg->flags & SC_DIALOG_OK)     _sc_dialog_ok_clicked(widget, dlg);
         else if(dlg->flags & SC_DIALOG_YES)    _sc_dialog_ok_clicked(widget, dlg);
         else if(dlg->flags & SC_DIALOG_APPLY)  _sc_dialog_apply_clicked(widget, dlg);
         else return(FALSE);
         return(TRUE);
   }

   /* Try to run parent keyhandler if we weren't able to do anything. */
   if(GTK_WIDGET_CLASS(parent_class)->key_press_event != NULL) {
      if(GTK_WIDGET_CLASS(parent_class)->key_press_event(widget, key)) {
         return(TRUE);
      }
   }

   /* We don't understand this key; defer to next handler. */
   return(FALSE);

}



/***  Dialog Class Instantiation  ***/



static void _sc_dialog_class_init(ScDialogClass *klass) {
/* sc_dialog_class_init
   Initialise the dialog class.  */

   GtkObjectClass *object_class = (GtkObjectClass *)klass;

   /* Get parent class */
   parent_class = g_type_class_peek(gtk_window_get_type());

   _sc_dialog_signals[APPLY_SIGNAL] =
      g_signal_new("apply",                        /* Signal name */
                   G_TYPE_FROM_CLASS(object_class),/* Type of object the signal applies to */
                   G_SIGNAL_RUN_LAST,              /* Signal flags; run this signal last */
                   offsetof(ScDialogClass, apply),
                                                   /* Offset to signal handler in class */
                   NULL,                           /* Signal accumulator function */
                   NULL,                           /* Signal accumulator data */
                   g_cclosure_marshal_VOID__VOID,  /* Marshal function for this signal */
                   G_TYPE_NONE,                    /* Return type for the marshaller */
                   0                               /* Number of extra parametres to pass */
                  );

   /* Setup new signals */
   klass->apply = NULL;

   /* Setup signals from parent */
   GTK_OBJECT_CLASS(klass)->destroy = _sc_dialog_destroy;
   GTK_WIDGET_CLASS(klass)->key_press_event = _sc_dialog_key_press;

}



static void _sc_dialog_init_obj(ScDialog *dlg) {
/* sc_dialog_init_obj
   Initialise an instance of a dialog window.  */

   dlg->state = NULL;

}



GType sc_dialog_get_type(void) {
/* sc_dialog_get_type
   Return the GTK type for a dialog window.  */

   static GType sc_dialog_type = 0;

   if(sc_dialog_type == 0) {
      static const GTypeInfo sc_dialog_info = {
         sizeof(ScDialogClass),           /* Size of the class object */
         NULL,                            /* Base initializer */
         NULL,                            /* Base finalizer */
         (GClassInitFunc)_sc_dialog_class_init,
                                          /* Class initializer */
         NULL,                            /* Class finalizer */
         NULL,                            /* Class data pointer */
         sizeof(ScDialog),                /* Size of an instance object */
         0,                               /* Number of preallocs */
         (GInstanceInitFunc)_sc_dialog_init_obj,
                                          /* Instance initializer */
         NULL                             /* Value table */
      };
      sc_dialog_type = g_type_register_static(gtk_window_get_type(), "ScDialog",
                                              &sc_dialog_info, 0);
   }

   return(sc_dialog_type);

}



/***  Dialog Interface  ***/



GtkWidget *sc_dialog_new(const char *title, const char *msgtext, guint flags) {
/* sc_dialog_new
   Create a new dialog window, with the indicated title, message text, and
   flags.  If title or msgtext are NULL, then they are omitted from the
   resultant dialog window.  */

   ScDialog *dialog;
   GtkWidget *vbox;
   GtkWidget *hbox;
   GtkWidget *msg;
   GtkWidget *btn;

   dialog = g_object_new(sc_dialog_get_type(), NULL);
   g_return_val_if_fail(dialog != NULL, NULL);

   if(title != NULL) {
      gtk_window_set_title(GTK_WINDOW(dialog), title);
   }

   vbox = gtk_vbox_new(FALSE, 5);
   gtk_container_set_border_width(GTK_CONTAINER(dialog), 10);
   gtk_container_add(GTK_CONTAINER(dialog), vbox);

   if(msgtext != NULL) {
      msg = gtk_label_new(msgtext);
      gtk_widget_set_size_request(msg, 350, -1);
      gtk_label_set_line_wrap(GTK_LABEL(msg), TRUE);
      gtk_box_pack_start(GTK_BOX(vbox), msg, TRUE, TRUE, 0);
   }
   if(flags & SC_DIALOG_NO_GRID) {
      dialog->grid = NULL;
   } else {
      dialog->grid = gtk_table_new(1, 1, FALSE);
      gtk_box_pack_start(GTK_BOX(vbox), dialog->grid, TRUE, TRUE, 0);
   }

   gtk_box_pack_start(GTK_BOX(vbox), gtk_hseparator_new(), FALSE, FALSE, 0);
   hbox = gtk_hbox_new(FALSE, 5);
   /* This box houses the command buttons and should not rescale vertically */
   gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

   if(flags & SC_DIALOG_CLOSE) {
      btn = gtk_button_new_with_label(" Close ");
      g_signal_connect(G_OBJECT(btn), "clicked", (GCallback)_sc_dialog_cancel_clicked, dialog);
      gtk_box_pack_end(GTK_BOX(hbox), btn, FALSE, FALSE, 5);
   }
   if(flags & SC_DIALOG_APPLY) {
      btn = gtk_button_new_with_label(" Apply ");
      g_signal_connect(G_OBJECT(btn), "clicked", (GCallback)_sc_dialog_apply_clicked, dialog);
      gtk_box_pack_end(GTK_BOX(hbox), btn, FALSE, FALSE, 5);
   }
   if(flags & SC_DIALOG_NO) {
      btn = gtk_button_new_with_label(" No ");
      g_signal_connect(G_OBJECT(btn), "clicked", (GCallback)_sc_dialog_cancel_clicked, dialog);
      gtk_box_pack_end(GTK_BOX(hbox), btn, FALSE, FALSE, 5);
   }
   if(flags & SC_DIALOG_CANCEL) {
      btn = gtk_button_new_with_label(" Cancel ");
      g_signal_connect(G_OBJECT(btn), "clicked", (GCallback)_sc_dialog_cancel_clicked, dialog);
      gtk_box_pack_end(GTK_BOX(hbox), btn, FALSE, FALSE, 5);
   }
   if(flags & SC_DIALOG_YES) {
      btn = gtk_button_new_with_label(" Yes ");
      g_signal_connect(G_OBJECT(btn), "clicked", (GCallback)_sc_dialog_ok_clicked, dialog);
      gtk_box_pack_end(GTK_BOX(hbox), btn, FALSE, FALSE, 5);
   }
   if(flags & SC_DIALOG_OK) {
      btn = gtk_button_new_with_label(" Ok ");
      g_signal_connect(G_OBJECT(btn), "clicked", (GCallback)_sc_dialog_ok_clicked, dialog);
      gtk_box_pack_end(GTK_BOX(hbox), btn, FALSE, FALSE, 5);
   }

   dialog->flags = flags;
   return(GTK_WIDGET(dialog));

}



gboolean sc_dialog_run(ScDialog *dialog) {
/* sc_dialog_run
   Runs the specified dialogue until it is closed.  This will set the
   dialog to modal if it was requested on creation, so other windows
   will be blocked while this dialog is visible.  */

   ScDialogState state;

   dialog->flags |= SC_DIALOG_DELAY_DESTROY;
   state = SC_DIALOG_WAITING;
   dialog->state = &state;

   gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
   gtk_widget_show_all(GTK_WIDGET(dialog));
   if(!(dialog->flags & SC_DIALOG_NONMODAL)) {
      gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
   }

   while(state == SC_DIALOG_WAITING) {
      do {
         gtk_main_iteration();
      } while(gtk_events_pending());
   }

   dialog->state = NULL;
   if(state != SC_DIALOG_DESTROYED) {
      gtk_widget_destroy(GTK_WIDGET(dialog));
   }
   return(state == SC_DIALOG_ACCEPTED);

}



void sc_dialog_show(ScDialog *dialog) {
/* sc_dialog_show
   Displays the dialog window.  */

   dialog->flags &= ~SC_DIALOG_DELAY_DESTROY;
   dialog->state = NULL;

   gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
   gtk_widget_show_all(GTK_WIDGET(dialog));
   if(!(dialog->flags & SC_DIALOG_NONMODAL)) {
      gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
   }

}



void sc_dialog_grid_attach(ScDialog *dlg, GtkWidget *widget, int row, int col) {
/* sc_dialog_grid_attach
   Attaches a widget to the dialog's grid.  */

   gtk_table_attach(GTK_TABLE(dlg->grid), widget, 
                    col, col + 1, row, row + 1, 
                    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 
                    2, 2);
   if(GTK_IS_MISC(widget)) {
      gtk_misc_set_alignment(GTK_MISC(widget), 0, 0.5);
   }

}



void sc_dialog_grid_attach_label(ScDialog *dlg, const char *msg, int row, int col) {
/* sc_dialog_grid_attach_label
   Attaches a text label to the dialog's grid.  */

   GtkWidget *label = sc_label_new(msg);
   gtk_table_attach(GTK_TABLE(dlg->grid), label, 
                    col, col + 1, row, row + 1, 
                    GTK_FILL, GTK_FILL, 2, 2);
   gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

}



/***  General-purpose dialogs    ***/



void sc_dialog_message(const char *title, const char *textmsg) {

   GtkWidget *dlg;

   dlg = sc_dialog_new(title, textmsg, SC_DIALOG_OK | SC_DIALOG_NO_GRID);
   sc_dialog_show(SC_DIALOG(dlg));

}



gboolean sc_dialog_query(const char *title, const char *textmsg) {

   gboolean result;
   GtkWidget *dlg;

   dlg = sc_dialog_new(title, textmsg, SC_DIALOG_YES | SC_DIALOG_NO | SC_DIALOG_NO_GRID);
   result = sc_dialog_run(SC_DIALOG(dlg));
   return(result);

}



void sc_dialog_error(const char *s) {

   char *err;
   char *buf;
   int length;

   err = strerror(errno);
   if(err == NULL) {
      sc_dialog_message("Error", s);
      return;
   }

   length = strlenn(s) + strlenn(err) + 4;

   buf = (char *)malloc(length);
   if(buf == NULL) {
      sc_dialog_message("Error", s);
      return;
   }

   sbprintf(buf, length, "%s: %s", s, err);
   sc_dialog_message("Error", buf);
   free(buf);
   return;

}



void sc_dialog_text(const char *filename, GdkFont *font_normal, GdkFont *font_italic, GdkFont *font_bold) {

   ScDialog *dlg;
   char buf[SC_DIALOG_STRING_BUFFER];
   char out[SC_DIALOG_STRING_BUFFER];
   char *pin;
   char *pout;
   int  width;
   int height;
   FILE *f;
   GdkFont *font;
   GdkFont *lfont;
   GtkWidget *scroll;
   GtkWidget *message;
   GtkTextBuffer *textbuf;
   GtkTextIter iter;
   GtkTextTag *tag;

   if((f = fopen(filename, "r")) == NULL) {
      sbprintf(buf, sizeof(buf), "Cannot open file %s.\n", filename);
      sc_dialog_error(buf);
      return;
   }

   dlg = SC_DIALOG(sc_dialog_new(filename, NULL, SC_DIALOG_CLOSE | SC_DIALOG_NONMODAL));

   /* Load the needed fonts */
   if(font_normal != NULL) gdk_font_ref(font_normal);
   if(font_italic != NULL) gdk_font_ref(font_italic);
   if(font_bold   != NULL) gdk_font_ref(font_bold);
   if(font_normal == NULL) {
      width = 8;
      height = 16;
   } else {
      width = gdk_char_width(font_normal, 'W');
      height = (font_normal->ascent + font_normal->descent);
   }
   width  *= 88;
   height *= 33;

   scroll = gtk_scrolled_window_new(NULL, NULL);
   sc_dialog_grid_attach(dlg, scroll, 0, 0);
   gtk_widget_set_size_request(scroll, width, height);

   message = gtk_text_view_new();
   textbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(message));
   gtk_text_view_set_editable(GTK_TEXT_VIEW(message), FALSE);
   gtk_container_add(GTK_CONTAINER(scroll), message);

   /* Load in the text data */
   g_object_freeze_notify(G_OBJECT(message));
   while(fgets(buf, SC_DIALOG_STRING_BUFFER, f)) {
      pin   = buf;
      pout  = out;
      font  = NULL;
      lfont = font_normal;
      while(*pin != '\0') {
         if(*(pin + 1) != 0x08) {
            font = font_normal;
         } else {
            font = (*pin == '_' ? font_italic : font_bold);
            pin += 2;
         }
         if(font != lfont) {
            if(pout - out > 0) {
               if(lfont == font_italic) {
                  tag = gtk_text_buffer_create_tag(textbuf, NULL,
                                                   "weight", PANGO_WEIGHT_NORMAL,
                                                   "style",  PANGO_STYLE_ITALIC,
                                                   NULL);
               } else if(lfont == font_bold) {
                  tag = gtk_text_buffer_create_tag(textbuf, NULL,
                                                   "weight", PANGO_WEIGHT_BOLD,
                                                   "style",  PANGO_STYLE_NORMAL,
                                                   NULL);
               } else {
                  tag = gtk_text_buffer_create_tag(textbuf, NULL,
                                                   "weight", PANGO_WEIGHT_NORMAL,
                                                   "style",  PANGO_STYLE_NORMAL,
                                                   NULL);
               }
               gtk_text_buffer_get_end_iter(textbuf, &iter);
               gtk_text_buffer_insert_with_tags(textbuf, &iter, out, pout - out, tag, NULL);
            }
            pout  = out;
            lfont = font;
         }
         *(pout++) = *pin;
         if(*pin == '\n') {
            *(pout++) = ' ';
            *(pout++) = ' ';
         }
         pin++;
      }
      if(pout - out > 0) {
         if(lfont == font_italic) {
            tag = gtk_text_buffer_create_tag(textbuf, NULL,
                                             "weight", PANGO_WEIGHT_NORMAL,
                                             "style",  PANGO_STYLE_ITALIC,
                                             NULL);
         } else if(lfont == font_bold) {
            tag = gtk_text_buffer_create_tag(textbuf, NULL,
                                             "weight", PANGO_WEIGHT_BOLD,
                                             "style",  PANGO_STYLE_NORMAL,
                                             NULL);
         } else {
            tag = gtk_text_buffer_create_tag(textbuf, NULL,
                                             "weight", PANGO_WEIGHT_NORMAL,
                                             "style",  PANGO_STYLE_NORMAL,
                                             NULL);
         }
         gtk_text_buffer_get_end_iter(textbuf, &iter);
         gtk_text_buffer_insert_with_tags(textbuf, &iter, out, pout - out, tag, NULL);
      }
   }
   if(font_normal != NULL) {
      gdk_font_unref(font_normal);
   }
   if(font_italic != NULL) {
      gdk_font_unref(font_italic);
   }
   if(font_bold   != NULL) {
      gdk_font_unref(font_bold);
   }
   g_object_thaw_notify(G_OBJECT(message));
   fclose(f);

   sc_dialog_show(dlg);
   return;

}



void sc_dialog_text_buffer(const char *title, const char *buffer,
                           GdkFont *font_normal, GdkFont *font_italic, GdkFont *font_bold) {

   ScDialog *dlg;
   char buf[SC_DIALOG_STRING_BUFFER];
   char out[SC_DIALOG_STRING_BUFFER];
   char *pin;
   char *pout;
   int  width;
   int height;
   int offset = 0;
   GdkFont *font;
   GdkFont *lfont;
   GtkWidget *scroll;
   GtkWidget *message;
   GtkTextBuffer *textbuf;
   GtkTextIter iter;
   GtkTextTag *tag;

   dlg = SC_DIALOG(sc_dialog_new(title, NULL, SC_DIALOG_CLOSE | SC_DIALOG_NONMODAL));

   /* Load the needed fonts */
   if(font_normal != NULL) gdk_font_ref(font_normal);
   if(font_italic != NULL) gdk_font_ref(font_italic);
   if(font_bold   != NULL) gdk_font_ref(font_bold);
   if(font_normal == NULL) {
      width = 8;
      height = 16;
   } else {
      width = gdk_char_width(font_normal, 'W');
      height = (font_normal->ascent + font_normal->descent);
   }
   width  *= 88;
   height *= 33;

   scroll = gtk_scrolled_window_new(NULL, NULL);
   sc_dialog_grid_attach(dlg, scroll, 0, 0);
   gtk_widget_set_size_request(scroll, width, height);

   message = gtk_text_view_new();
   textbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(message));
   gtk_text_view_set_editable(GTK_TEXT_VIEW(message), FALSE);
   gtk_container_add(GTK_CONTAINER(scroll), message);

   /* Load in the text data */
   g_object_freeze_notify(G_OBJECT(message));
   while(sgetline(buf, SC_DIALOG_STRING_BUFFER, buffer, &offset) != NULL) {
      pin   = buf;
      pout  = out;
      font  = NULL;
      lfont = font_normal;
      while(*pin != '\0') {
         if(*(pin + 1) != 0x08) {
            font = font_normal;
         } else {
            font = (*pin == '_' ? font_italic : font_bold);
            pin += 2;
         }
         if(font != lfont) {
            if(pout - out > 0) {
               if(lfont == font_italic) {
                  tag = gtk_text_buffer_create_tag(textbuf, NULL,
                                                   "weight", PANGO_WEIGHT_NORMAL,
                                                   "style",  PANGO_STYLE_ITALIC,
                                                   NULL);
               } else if(lfont == font_bold) {
                  tag = gtk_text_buffer_create_tag(textbuf, NULL,
                                                   "weight", PANGO_WEIGHT_BOLD,
                                                   "style",  PANGO_STYLE_NORMAL,
                                                   NULL);
               } else {
                  tag = gtk_text_buffer_create_tag(textbuf, NULL,
                                                   "weight", PANGO_WEIGHT_NORMAL,
                                                   "style",  PANGO_STYLE_NORMAL,
                                                   NULL);
               }
               gtk_text_buffer_get_end_iter(textbuf, &iter);
               gtk_text_buffer_insert_with_tags(textbuf, &iter, out, pout - out, tag, NULL);
            }
            pout  = out;
            lfont = font;
         }
         *(pout++) = *pin;
         if(*pin == '\n') {
            *(pout++) = ' ';
            *(pout++) = ' ';
         }
         pin++;
      }
      if(pout - out > 0) {
         if(lfont == font_italic) {
            tag = gtk_text_buffer_create_tag(textbuf, NULL,
                                             "weight", PANGO_WEIGHT_NORMAL,
                                             "style",  PANGO_STYLE_ITALIC,
                                             NULL);
         } else if(lfont == font_bold) {
            tag = gtk_text_buffer_create_tag(textbuf, NULL,
                                             "weight", PANGO_WEIGHT_BOLD,
                                             "style",  PANGO_STYLE_NORMAL,
                                             NULL);
         } else {
            tag = gtk_text_buffer_create_tag(textbuf, NULL,
                                             "weight", PANGO_WEIGHT_NORMAL,
                                             "style",  PANGO_STYLE_NORMAL,
                                             NULL);
         }
         gtk_text_buffer_get_end_iter(textbuf, &iter);
         gtk_text_buffer_insert_with_tags(textbuf, &iter, out, pout - out, tag, NULL);
      }
   }
   if(font_normal != NULL) {
      gdk_font_unref(font_normal);
   }
   if(font_italic != NULL) {
      gdk_font_unref(font_italic);
   }
   if(font_bold   != NULL) {
      gdk_font_unref(font_bold);
   }
   g_object_thaw_notify(G_OBJECT(message));

   sc_dialog_show(dlg);
   return;

}
