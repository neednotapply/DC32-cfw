/* $Header: /fridge/cvs/xscorch/sgtk/ssound-gtk.c,v 1.12 2009-04-26 17:39:50 jacob Exp $ */
/*
   
   xscorch - ssound-gtk.c     Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/
    
   Sound configuration dialogue
    

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
#include <slinkcheck.h>

#include <sdialog-gtk.h>
#include <ssetup-gtk.h>

#include <sgame/sconfig.h>
#include <sgame/sgame.h>
#include <ssound/ssound.h>



#if !USE_SOUND
void sc_sound_setup_gtk(sc_window_gtk *w) {
   
   sc_dialog_message("Sound support not enabled", "Sound support not enabled");
   
}
#else /* Sound dialogue... */


typedef struct _sc_sound_setup_data_gtk {
   sc_config *c;
   bool enablesound;
   bool usehqmixer;
} sc_sound_setup_data_gtk;



static void _sc_sound_setup_apply_gtk(__libj_unused ScDialog *dlg, sc_sound_setup_data_gtk *setup) {

   sc_config *c = setup->c;
   
   if(c->enablesound != setup->enablesound || c->usehqmixer != setup->usehqmixer) {
      c->enablesound = setup->enablesound;
      c->usehqmixer  = setup->usehqmixer;
      sc_sound_config(&c->sound, c->enablesound, c->usehqmixer);
      sc_sound_start(c->sound, c->game->musicid);
   }

}



void sc_sound_setup_gtk(sc_window_gtk *w) {

   sc_config *c = w->c;
   sc_sound_setup_data_gtk setup;
   ScDialog *dialog;
   int row = 0;

   setup.c = c;
   setup.enablesound = c->enablesound;
   setup.usehqmixer  = c->usehqmixer;

   dialog = SC_DIALOG(sc_dialog_new("Sound Setup", NULL, SC_DIALOG_OK | SC_DIALOG_CANCEL | SC_DIALOG_APPLY));
   g_signal_connect(G_OBJECT(dialog), "apply",
                    (GCallback)_sc_sound_setup_apply_gtk, &setup);

   attach_option(dialog, w, "Enable Sound", sc_link_check_new(&setup.enablesound), &row);
   attach_option(dialog, w, "Use HQ Mixer", sc_link_check_new(&setup.usehqmixer), &row);

   sc_dialog_run(dialog);

}



#endif /* Sound allowed? */
