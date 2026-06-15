/* $Header: /fridge/cvs/xscorch/sgtk/smainmenu-gtk.c,v 1.23 2009-04-26 17:39:49 jacob Exp $ */
/*
   
   xscorch - smainmenu-gtk.c  Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/
    
   Main menu console for scorch
    

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
#include <sactiveconsole.h>
#include <sdialog.h>
#include <sdisplay.h>

#include <scolor-gtk.h>
#include <simage-gtk.h>
#include <ssetup-gtk.h>
#include <swindow-gtk.h>

#include <sgame/scffile.h>
#include <sgame/sconfig.h>
#include <sgame/sgame.h>
#include <sgame/sstate.h>

#include <gdk/gdkkeysyms.h>



/* Various actions we may take on main menu */
#define  SC_ACTION_PLAYER     1
#define  SC_ACTION_ECONOMY    2
#define  SC_ACTION_PHYSICS    3
#define  SC_ACTION_LAND       4
#define  SC_ACTION_WEAPONS    5
#define  SC_ACTION_GRAPHICS   6
#define  SC_ACTION_OPTIONS    7
#define  SC_ACTION_AI         8
#define  SC_ACTION_SOUND      9
#define  SC_ACTION_SAVE       10
#define  SC_ACTION_BEGIN      11
#define  SC_ACTION_QUIT       12



static gboolean _sc_main_menu_action_gtk(sc_window_gtk *w, gint row) {

   switch(row) {
      case SC_ACTION_PLAYER:
         sc_player_setup_gtk(w);
         return(TRUE);
   
      case SC_ACTION_ECONOMY:
         sc_economy_setup_gtk(w);
         return(TRUE);
   
      case SC_ACTION_PHYSICS:
         sc_physics_setup_gtk(w);
         return(TRUE);
   
      case SC_ACTION_LAND:
         sc_land_setup_gtk(w);
         return(TRUE);
   
      case SC_ACTION_WEAPONS:
         sc_weapons_setup_gtk(w);
         return(TRUE);
   
      case SC_ACTION_OPTIONS:
         sc_options_setup_gtk(w);
         return(TRUE);
   
      case SC_ACTION_GRAPHICS:
         sc_graphics_setup_gtk(w);
         return(TRUE);

      case SC_ACTION_AI:
         sc_ai_controller_setup_gtk(w);
         return(TRUE);

      case SC_ACTION_SOUND:
         sc_sound_setup_gtk(w);
         return(TRUE);

      case SC_ACTION_SAVE:
         sc_config_file_save_gtk(w);
         return(TRUE);
   
      case SC_ACTION_BEGIN:
         if(sc_config_okay_to_begin(w->c)) {
            sc_display_console_detach_all(SC_DISPLAY(w->screen));
            sc_game_set_state_now(w->c, w->c->game, SC_STATE_GAME_BEGIN);
         }
         return(TRUE);
   
      case SC_ACTION_QUIT:
         gtk_main_quit();
         return(TRUE);
         
      default:
         return(FALSE);
   }
   
}



static gboolean _sc_main_menu_select_gtk(__libj_unused ScActiveConsole *cons,
                                         ScActiveConsoleSpot *spot,
                                         sc_window_gtk *w) {

   return(_sc_main_menu_action_gtk(w, spot->y));

}



void sc_window_main_menu(sc_window *w_) {

   sc_window_gtk *w = (sc_window_gtk *)w_;
   static const char *mainmenu[] = {
      "XScorch Options:",
      "   Setup Players/Rounds",
      "   Setup Economics",
      "   Setup Physics",
      "   Setup Landscape",
      "   Setup Weapons",
      "   Setup Graphics",
      "   Setup Gameplay Options",
      "   Setup AI Controller",
      "   Setup Sound Support",
      "   Save Options",
      "   Begin Game",
      "   Exit XScorch",
      "You may use mouse or arrow",
      "keys to select menu options.",
      NULL
   };
   const char **current;
   GtkWidget *console;

   /* Detach any currently visible consoles */
   sc_display_console_detach_all(SC_DISPLAY(w->screen));

   /* Setup the background image */   
   sc_window_paint_blank(w_);
   sc_window_paint(w_, 0, 0, w->c->fieldwidth, w->c->fieldheight,
                   SC_REGENERATE_LAND | SC_REDRAW_LAND);
   sc_pixmap_copy_gtk(sc_display_get_buffer(SC_DISPLAY(w->screen)),
                      sc_display_get_gc(SC_DISPLAY(w->screen)),
                      w->logo, w->logo_m,
                      w->c->fieldwidth - sc_pixmap_width_gtk(w->logo),
                      w->c->fieldheight - sc_pixmap_height_gtk(w->logo));

   /* Put strings into menu; activate the relevant rows */
   console = sc_window_active_console_new(w, 4, 2, 30, 15, CONSOLE_NORMAL);
   for(current = mainmenu; *current != NULL; ++current) {
      sc_console_write_line(SC_CONSOLE(console), 0, current - mainmenu, *current);
      if(**current == ' ') {
         /* We assume this line is a menu entry, if indented */
         sc_active_console_add_row_spot(SC_ACTIVE_CONSOLE(console), current - mainmenu, NULL);
      }
   }

   /* Must be connected after default handler is run */
   g_signal_connect_after(G_OBJECT(console), "select_spot",
                          (GCallback)_sc_main_menu_select_gtk, w);
   g_object_set_data(G_OBJECT(console), "user_data", w);
   sc_display_console_attach(SC_DISPLAY(w->screen), SC_CONSOLE(console));

   /* Setup the general interface */
   gtk_window_set_default(GTK_WINDOW(w->app), GTK_WIDGET(console));
   gtk_widget_grab_focus(GTK_WIDGET(console));

}
