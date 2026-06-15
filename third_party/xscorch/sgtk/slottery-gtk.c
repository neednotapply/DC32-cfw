/* $Header: /fridge/cvs/xscorch/sgtk/slottery-gtk.c,v 1.15 2011-08-01 00:01:42 jacob Exp $ */
/*

   xscorch - slottery-gtk.c   Copyright(c) 2001-2003 Justin David Smith
                              Copyright(c) 2001-2003 Jacob Luna Lundberg
   justins(at)chaos2.org      http://chaos2.org/
   jacob(at)gnifty.net        http://www.gnifty.net/

   Lottery results screen


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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <sgtk.h>
#include <sactiveconsole.h>
#include <sdisplay.h>

#include <scolor-gtk.h>
#include <swindow-gtk.h>

#include <sai/sai.h>
#include <sgame/sconfig.h>
#include <sgame/sgame.h>
#include <sgame/splayer.h>
#include <sgame/spreround.h>
#include <sgame/sstate.h>
#include <sgame/sweapon.h>

#include <gdk/gdkkeysyms.h>
#include <libj/jstr/libjstr.h>



static gboolean _sc_window_lottery_close_gtk(__libj_unused GtkWidget *widget,
                                             __libj_unused ScActiveConsoleSpot *spot,
                                             sc_window_gtk *w) {
/* _sc_window_lottery_close_gtk
   Advance on to the round proper. */

   sc_display_console_detach_all(SC_DISPLAY(w->screen));
   sc_game_set_state_now(w->c, w->c->game, SC_STATE_TURN_BEGIN);
   return(TRUE);

}



static gboolean _sc_window_lottery_key_gtk(__libj_unused GtkWidget *widget,
                                           __libj_unused ScActiveConsoleSpot *spot,
                                           GdkEventKey *key, sc_window_gtk *w) {
/* _sc_window_lottery_key_gtk
   Handle keystrokes.  Advance on to the round proper. */

   switch(key->keyval) {
      case GDK_Return:
      case GDK_KP_Enter:
      case GDK_Escape:
         sc_display_console_detach_all(SC_DISPLAY(w->screen));
         sc_game_set_state_now(w->c, w->c->game, SC_STATE_TURN_BEGIN);
         return(TRUE);
   }
   return(FALSE);

}



void sc_window_lottery_result(sc_window *w_, bool showstake) {
/* sc_window_lottery_result
   Paste up a window describing the result of the lottery. */

   sc_window_gtk *w = (sc_window_gtk *)w_;
   char buf[SC_GTK_STRING_BUFFER];
   GtkWidget *console;

   /* Make sure there's something to display. */
   if(w->c->lottery == NULL || w->c->lottery->winner == NULL) {
      printf("Warning:  called sc_window_lottery_result with no winners!\n");
      sc_game_set_state_now(w->c, w->c->game, SC_STATE_TURN_BEGIN);
      return;
   }

   /* Create a new console window.           x   y   w   h */
   console = sc_window_active_console_new(w, 8,  4, 36,  7, CONSOLE_NORMAL);

   /* Display the title, highlighted. */
   sc_console_write_line(SC_CONSOLE(console), 10, 0, "Lottery Winner!");
   sc_console_highlight_attach(SC_CONSOLE(console), &w->colormap->white, NULL, TRUE, 10, 0, 15, 1);

   /* Show the winner's name. */
   sbprintf(buf, sizeof(buf), "%s won the lottery!", w->c->lottery->winner->name);
   sc_console_write_line(SC_CONSOLE(console), 0, 2, buf);

   /* If we've been instructed to, show the stake. */
   if(showstake) {
      sbprintf(buf, sizeof(buf), "You just received a bundle of:");
      sc_console_write_line(SC_CONSOLE(console), 0, 3, buf);
      sbprintf(buf, sizeof(buf), w->c->lottery->stake->name);
      sc_console_write_line(SC_CONSOLE(console), 0, 4, buf);
   }

   /* Tell them how to exit this display. */
   sc_console_write_line(SC_CONSOLE(console), 0, 6, "Press [Enter] to close");
   sc_display_console_attach(SC_DISPLAY(w->screen), SC_CONSOLE(console));

   /* Display the close button. */
   sc_console_write_line(SC_CONSOLE(console), 27, 6, "< Close >");
   sc_active_console_add_spot(SC_ACTIVE_CONSOLE(console), 27, 6, 9, 1, NULL);

   /* Connect the exit methods. */
   g_signal_connect_after(G_OBJECT(console), "key_press_spot", 
                          (GCallback)_sc_window_lottery_key_gtk, w);
   g_signal_connect_after(G_OBJECT(console), "select-spot", 
                          (GCallback)_sc_window_lottery_close_gtk, w);

   /* focus on this window */
   gtk_window_set_default(GTK_WINDOW(w->app), GTK_WIDGET(console));
   gtk_widget_grab_focus(GTK_WIDGET(console));

}
