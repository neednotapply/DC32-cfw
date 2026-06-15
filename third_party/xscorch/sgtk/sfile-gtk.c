/* $Header: /fridge/cvs/xscorch/sgtk/sfile-gtk.c,v 1.15 2009-04-26 17:39:48 jacob Exp $ */
/*
   
   xscorch - sfile-gtk.c      Copyright(c) 2001-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/
    
   Various file saving utilities
    

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
#include <sgtk.h>
#include <sdialog.h>
#include <slabel.h>
#include <slinkentry.h>

#include <sdialog-gtk.h>
#include <ssetup-gtk.h>

#include <sgame/scffile.h>
#include <sgame/sconfig.h>

#include <gdk/gdkkeysyms.h>
#include <libj/jreg/libjreg.h>



typedef struct _sc_file_save {
   sc_config *c;
   char name[SC_FILENAME_LENGTH];
} sc_file_save;



static void _sc_config_file_save_apply_gtk(__libj_unused ScDialog *dlg, sc_file_save *save) {

   char buf[0x1000];
      
   reg_set_name(save->c->cfreg, save->name);
   if(sc_config_file_save(save->c)) {
      sbprintf(buf, sizeof(buf), "Options saved successfully to \"%s\".", save->name);
      sc_dialog_message("Options saved", buf);
   } else {
      sbprintf(buf, sizeof(buf), "An error occurred during the save of \"%s\".", save->name);
      sc_dialog_error(buf);
   }

}



void sc_config_file_save_gtk(sc_window_gtk *w) {

   sc_file_save save;
   ScDialog  *dialog;
   int row = 0;
   
   save.c = w->c;
   strcopyb(save.name, w->c->cfreg->filename, sizeof(save.name));
   
   dialog = SC_DIALOG(sc_dialog_new("Save Options As ...", NULL, SC_DIALOG_OK | SC_DIALOG_CANCEL));
   g_signal_connect(G_OBJECT(dialog), "apply",
                    (GCallback)_sc_config_file_save_apply_gtk, &save);

   attach_option_help(dialog, w, "Save options in file", 
                      "Specify the name of a file to save your user configuration to.", 
                      sc_link_entry_new(save.name, SC_FILENAME_LENGTH), &row);

   sc_dialog_run(dialog);

}
