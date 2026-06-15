/* $Header: /fridge/cvs/xscorch/sgtk/sfont-gtk.c,v 1.14 2009-04-26 17:39:48 jacob Exp $ */
/*
   
   xscorch - sfont-gtk.c      Copyright(c) 2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/
    
   Various font dialogues
    

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
#include <unistd.h>

#include <sgtk.h>
#include <slabel.h>
#include <slinkentry.h>

#include <ssetup-gtk.h>

#include <sgame/sconfig.h>

#include <libj/jstr/libjstr.h>



typedef struct _sc_fontsel_data_gtk {
   GtkWidget *entry;
   GtkWidget *fontsel;
} sc_fontsel_data_gtk;



typedef struct _sc_font_data_gtk {
   sc_window_gtk *w;
   char fixed_font[SC_FONT_LENGTH];
   char italic_fixed_font[SC_FONT_LENGTH];
   char bold_fixed_font[SC_FONT_LENGTH];
   GtkWidget *fixed_entry;
   GtkWidget *italic_fixed_entry;
   GtkWidget *bold_fixed_entry;
} sc_font_data_gtk;



static void _sc_fontsel_apply_gtk(__libj_unused ScDialog *dlg, sc_fontsel_data_gtk *setup) {

   char *font;

   font = gtk_font_selection_get_font_name(GTK_FONT_SELECTION(setup->fontsel));
   sc_link_entry_set_text(SC_LINK_ENTRY(setup->entry), font);

}



static void _sc_fontsel_gtk(const char *title, GtkWidget *entry) {

   sc_fontsel_data_gtk setup;
   ScDialog *dialog;
   const char *font;

   setup.entry = entry;
   font = gtk_entry_get_text(GTK_ENTRY(entry));

   dialog = SC_DIALOG(sc_dialog_new(title, NULL, SC_DIALOG_OK | SC_DIALOG_CANCEL));
   g_signal_connect(G_OBJECT(dialog), "apply",
                    (GCallback)_sc_fontsel_apply_gtk, &setup);

   setup.fontsel = gtk_font_selection_new();
   sc_dialog_grid_attach(dialog, setup.fontsel, 0, 0);
   gtk_font_selection_set_font_name(GTK_FONT_SELECTION(setup.fontsel), font);

   /* GTK 2.0 has NO font filter abilities.  Numerous (user list) requests to
      revive this functionality appear to have gone completely unanswered.
      We will have to wait and hope for it to appear in newer GTK someday...
      TEMP: That was 2.2 ... now we're at 2.12 ... maybe it's back? -JTL */

   sc_dialog_run(dialog);

}



static void _sc_fontsel_fixed_gtk(__libj_unused GtkWidget *widget, sc_font_data_gtk *fsetup) {

   _sc_fontsel_gtk("Change Fixed Font", fsetup->fixed_entry);

}



static void _sc_fontsel_italic_fixed_gtk(__libj_unused GtkWidget *widget, sc_font_data_gtk *fsetup) {

   _sc_fontsel_gtk("Change Italic Fixed Font", fsetup->italic_fixed_entry);

}



static void _sc_fontsel_bold_fixed_gtk(__libj_unused GtkWidget *widget, sc_font_data_gtk *fsetup) {

   _sc_fontsel_gtk("Change Bold Fixed Font", fsetup->bold_fixed_entry);

}



static void _sc_font_apply_gtk(__libj_unused ScDialog *dlg, sc_font_data_gtk *setup) {

   sc_window_gtk *w = setup->w;
   sc_config *c = w->c;

   strcopyb(c->fixed_font,          setup->fixed_font,         SC_FONT_LENGTH);
   strcopyb(c->italic_fixed_font,   setup->italic_fixed_font,  SC_FONT_LENGTH);
   strcopyb(c->bold_fixed_font,     setup->bold_fixed_font,    SC_FONT_LENGTH);
   
   sc_dialog_message("Font Update: May Cause Problems!", 
                     "Font changes don't take effect immediately on all"
                     " consoles, and the status bar may not be updated correctly during"
                     " a game. Be sure to save your configuration to make any font changes"
                     " permanent.");
   
   sc_window_reload_fonts(w);

}



void sc_font_gtk(sc_window_gtk *w) {

   GtkWidget *button;
   int row = 0;

   sc_config *c = w->c;
   sc_font_data_gtk setup;
   ScDialog *dialog;

   setup.w = w;
   strcopyb(setup.fixed_font,          c->fixed_font,          SC_FONT_LENGTH);
   strcopyb(setup.italic_fixed_font,   c->italic_fixed_font,   SC_FONT_LENGTH);
   strcopyb(setup.bold_fixed_font,     c->bold_fixed_font,     SC_FONT_LENGTH);

   dialog = SC_DIALOG(sc_dialog_new("Font Selection", NULL, SC_DIALOG_APPLY | SC_DIALOG_OK | SC_DIALOG_CANCEL));
   g_signal_connect(G_OBJECT(dialog), "apply",
                    (GCallback)_sc_font_apply_gtk, &setup);

   sc_dialog_grid_attach(dialog, sc_label_new("Fixed font"), row, 0);
   setup.fixed_entry = sc_link_entry_new(setup.fixed_font, SC_FONT_LENGTH);
   sc_dialog_grid_attach(dialog, setup.fixed_entry, row, 1);
   button = gtk_button_new_with_label(" Change ");
   g_signal_connect(G_OBJECT(button), "clicked",
                    (GCallback)_sc_fontsel_fixed_gtk, &setup);
   sc_dialog_grid_attach(dialog, button, row, 2);
   ++row;
                  
   sc_dialog_grid_attach(dialog, sc_label_new("Italic fixed font"), row, 0);
   setup.italic_fixed_entry = sc_link_entry_new(setup.italic_fixed_font, SC_FONT_LENGTH);
   sc_dialog_grid_attach(dialog, setup.italic_fixed_entry, row, 1);
   button = gtk_button_new_with_label(" Change ");
   g_signal_connect(G_OBJECT(button), "clicked",
                    (GCallback)_sc_fontsel_italic_fixed_gtk, &setup);
   sc_dialog_grid_attach(dialog, button, row, 2);
   ++row;
                  
   sc_dialog_grid_attach(dialog, sc_label_new("Bold fixed font"), row, 0);
   setup.bold_fixed_entry = sc_link_entry_new(setup.bold_fixed_font, SC_FONT_LENGTH);
   sc_dialog_grid_attach(dialog, setup.bold_fixed_entry, row, 1);
   button = gtk_button_new_with_label(" Change ");
   g_signal_connect(G_OBJECT(button), "clicked",
                    (GCallback)_sc_fontsel_bold_fixed_gtk, &setup);
   sc_dialog_grid_attach(dialog, button, row, 2);
   ++row;
                  
   sc_dialog_run(dialog);

}
