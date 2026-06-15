/* $Header: /fridge/cvs/xscorch/sgtk/ssystem-gtk.c,v 1.12 2009-04-26 17:39:50 jacob Exp $ */
/*

   xscorch - ssystem-gtk.c    Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   System menu dialogue


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
#include <ssystem-gtk.h>
#include <sdialog-gtk.h>
#include <ssetup-gtk.h>

#include <sdialog.h>
#include <slabel.h>
#include <slinkcheck.h>

#include <sgame/sconfig.h>
#include <sgame/sgame.h>
#include <sgame/sland.h>
#include <sgame/sstate.h>



typedef struct _sc_system_menu_data_gtk {
   sc_config *c;
   bool gfxanimate;
   bool gfxfast;
   bool gfxcompfast;
} sc_system_menu_data_gtk;



static void _sc_system_menu_apply_gtk(__libj_unused ScDialog *dlg,
                                      sc_system_menu_data_gtk *setup) {

   setup->c->graphics.gfxanimate = setup->gfxanimate;
   setup->c->graphics.gfxfast    = setup->gfxfast;
   setup->c->graphics.gfxcompfast= setup->gfxcompfast;

}



static void _sc_system_retreat_gtk(__libj_unused GtkWidget *button,
                                   __libj_unused sc_system_menu_data_gtk *setup) {

   sc_dialog_error("Sorry, not implemented");

}



static void _sc_system_resign_gtk(__libj_unused GtkWidget *button,
                                  sc_system_menu_data_gtk *setup) {

   #if USE_NETWORK
      if(setup->c->server != NULL || setup->c->client != NULL) {
         /* TEMP  this is a bug */
         sc_dialog_message("Not allowed in network", "Resign is broken in network mode, sorry.");
         return;
      }
   #endif /* Networking? */

   sc_game_pause(setup->c, setup->c->game);
   if(sc_dialog_query("Resign Game?", "Are you sure you want to RESIGN this game?")) {
      sc_config_init_game(setup->c);
   }
   sc_game_unpause(setup->c, setup->c->game);

}



static void _sc_system_mass_kill_gtk(__libj_unused GtkWidget *button,
                                     sc_system_menu_data_gtk *setup) {

   #if USE_NETWORK
      if(setup->c->server != NULL || setup->c->client != NULL) {
         sc_dialog_message("Not allowed in network", "Mass kill is not permitted in network mode.");
         return;
      }
   #endif /* Networking? */

   sc_game_pause(setup->c, setup->c->game);
   if(!SC_STATE_IS_ROUND(setup->c->game)) {
      sc_dialog_error("Cannot mass kill if not in round");
   } else if(sc_dialog_query("Mass Kill?", "Are you sure you want to kill everyone and end this round?")) {
      sc_game_mass_kill(setup->c, setup->c->game);
   }
   sc_game_unpause(setup->c, setup->c->game);

}



static void _sc_system_erase_smoke_gtk(__libj_unused GtkWidget *button,
                                       sc_system_menu_data_gtk *setup) {

   sc_land_clear_smoke(setup->c, setup->c->land);

}



static void _sc_system_sound_gtk(__libj_unused GtkWidget *button,
                                 sc_system_menu_data_gtk *setup) {

   sc_sound_setup_gtk((sc_window_gtk *)setup->c->window);

}



void sc_system_menu_gtk(sc_window_gtk *w) {

   char help[SC_GTK_STRING_BUFFER];
   sc_config *c = w->c;
   sc_system_menu_data_gtk setup;
   GtkWidget *button;
   ScDialog *dialog;
   int row = 0;

   setup.c = c;
   setup.gfxanimate     = c->graphics.gfxanimate;
   setup.gfxfast        = c->graphics.gfxfast;
   setup.gfxcompfast    = c->graphics.gfxcompfast;

   dialog = SC_DIALOG(sc_dialog_new("System Menu", NULL, SC_DIALOG_OK | SC_DIALOG_CANCEL));
   g_signal_connect(G_OBJECT(dialog), "apply",
                    (GCallback)_sc_system_menu_apply_gtk, &setup);

   attach_option(dialog, w, "Animation",           sc_link_check_new(&setup.gfxanimate), &row);
   attach_option(dialog, w, "Graphics Are Fast",   sc_link_check_new(&setup.gfxfast), &row);
   attach_option(dialog, w, "Computers Are Fast",  sc_link_check_new(&setup.gfxcompfast), &row);

   sc_help_text(help, sizeof(help), "Mass Kill");
   button = tooltip(w, help, gtk_button_new_with_label(" Mass Kill "));
   g_signal_connect(G_OBJECT(button), "clicked",
                    (GCallback)_sc_system_mass_kill_gtk, &setup);
   sc_dialog_grid_attach(dialog, button, row++, 1);

   sc_help_text(help, sizeof(help), "Erase Smoke");
   button = tooltip(w, help, gtk_button_new_with_label(" Erase Smoke "));
   g_signal_connect(G_OBJECT(button), "clicked",
                    (GCallback)_sc_system_erase_smoke_gtk, &setup);
   sc_dialog_grid_attach(dialog, button, row++, 1);

   sc_help_text(help, sizeof(help), "Retreat");
   button = tooltip(w, help, gtk_button_new_with_label(" N/A:  Retreat "));
   g_signal_connect(G_OBJECT(button), "clicked",
                    (GCallback)_sc_system_retreat_gtk, &setup);
   sc_dialog_grid_attach(dialog, button, row++, 1);

   sc_help_text(help, sizeof(help), "Resign Game");
   button = tooltip(w, help, gtk_button_new_with_label(" Resign Game "));
   g_signal_connect(G_OBJECT(button), "clicked",
                    (GCallback)_sc_system_resign_gtk, &setup);
   sc_dialog_grid_attach(dialog, button, row++, 1);

   sc_help_text(help, sizeof(help), "Sound Setup");
   button = tooltip(w, help, gtk_button_new_with_label(" Sound Setup "));
   g_signal_connect(G_OBJECT(button), "clicked",
                    (GCallback)_sc_system_sound_gtk, &setup);
   sc_dialog_grid_attach(dialog, button, row++, 1);

   sc_dialog_run(dialog);

}
