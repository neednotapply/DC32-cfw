/* $Header: /fridge/cvs/xscorch/sgtk/send-gtk.c,v 1.20 2009-04-26 17:39:47 jacob Exp $ */
/*
   
   xscorch - send-gtk.c       Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/
    
   Window painting code for end game/end round
    

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

#include <scolor-gtk.h>
#include <sdisplay.h>
#include <smenu-gtk.h>
#include <swindow-gtk.h>

#include <sgame/sgame.h>
#include <sgame/sland.h>
#include <sgame/splayer.h>
#include <sgame/sweapon.h>

#include <gdk/gdkkeysyms.h>
#include <libj/jstr/libjstr.h>



static void _sc_window_end_populate_gtk(sc_window_gtk *w, GtkWidget *console) {

   char buf[SC_GTK_STRING_BUFFER];
   sc_player *p;
   int deltamoney;
   int i;
   int j;

   for(i = 0; i < w->c->numplayers; ++i) {
      j = 3;
      p = w->c->players[i];
      deltamoney = p->money - p->oldmoney;

      sbprintf(buf, sizeof(buf), "%2d %-12s %-16s  %3d  %3d  %3d  ", 
               p->index + 1, sc_ai_name(p->aitype), p->name, 
               p->numwins, p->kills, p->suicides);
      sc_console_write_line(SC_CONSOLE(console), j, i + 2, buf);
      j += strblenn(buf, SC_GTK_STRING_BUFFER);

      if(p->killedby >= 0) {
         sbprintf(buf, sizeof(buf), "%2d  ", p->killedby + 1);
      } else {
         strcopyb(buf, "    ", sizeof(buf));
      }
      sc_console_write_line(SC_CONSOLE(console), j, i + 2, buf);
      sc_console_highlight_attach(SC_CONSOLE(console), &w->colormap->pcolors[p->index], NULL, FALSE, 3, i + 2, 2, 1);
      j += strblenn(buf, SC_GTK_STRING_BUFFER);
      
      sbprintf(buf, sizeof(buf), "$%-9d  %c$%d", p->money, (deltamoney < 0 ? '-' : ' '), abs(deltamoney));
      sc_console_write_line(SC_CONSOLE(console), j, i + 2, buf);
   }
   
}



static gboolean _sc_window_end_select_gtk(__libj_unused ScActiveConsole *widget,
                                          __libj_unused ScActiveConsoleSpot *spot,
                                          sc_window_gtk *w) {

   sc_display_console_detach_all(SC_DISPLAY(w->screen));
   sc_game_unpause(w->c, w->c->game);
   return(TRUE);

}



void sc_window_paint_end_round(sc_window *w_) {

   GtkWidget *console;
   sc_window_gtk *w = (sc_window_gtk *)w_;
   char buf[SC_GTK_STRING_BUFFER];

   console = sc_window_active_console_new(w, 4, 2, 80, w->c->numplayers + 2, CONSOLE_NORMAL);

   sbprintf(buf, sizeof(buf), "End of round %d, press [Enter] to continue.", w->c->curround + 1);
   sc_console_write_line(SC_CONSOLE(console), 0, 0, buf);
   sc_console_write_line(SC_CONSOLE(console), 3, 1, "#  Type         Name              Wins Kill Suic Xby Money      Winnings");
   _sc_window_end_populate_gtk(w, console);
   sc_display_console_attach(SC_DISPLAY(w->screen), SC_CONSOLE(console));

   sc_console_write_line(SC_CONSOLE(console), 60, 0, "< Continue >");
   sc_active_console_add_spot(SC_ACTIVE_CONSOLE(console), 60, 0, 12, 1, NULL);
   g_signal_connect(G_OBJECT(console), "select_spot", 
                    (GCallback)_sc_window_end_select_gtk, w);
   
   /* focus on this window */
   gtk_window_set_default(GTK_WINDOW(w->app), GTK_WIDGET(console));
   gtk_widget_grab_focus(GTK_WIDGET(console));

}



void sc_window_paint_end_game(sc_window *w_) {

   GtkWidget *console;
   sc_window_gtk *w = (sc_window_gtk *)w_;

   console = sc_window_active_console_new(w, 4, 2, 80, w->c->numplayers + 2, CONSOLE_NORMAL);

   sc_console_write_line(SC_CONSOLE(console), 0, 0, "End of game, press [Enter] to restart.");
   sc_console_write_line(SC_CONSOLE(console), 3, 1, "#  Type         Name              Wins Kill Suic Xby Money      Winnings");
   _sc_window_end_populate_gtk(w, console);
   sc_display_console_attach(SC_DISPLAY(w->screen), SC_CONSOLE(console));

   sc_console_write_line(SC_CONSOLE(console), 60, 0, "< Continue >");
   sc_active_console_add_spot(SC_ACTIVE_CONSOLE(console), 60, 0, 12, 1, NULL);
   g_signal_connect(G_OBJECT(console), "select_spot", 
                    (GCallback)_sc_window_end_select_gtk, w);

   /* focus on this window */
   gtk_window_set_default(GTK_WINDOW(w->app), GTK_WIDGET(console));
   gtk_widget_grab_focus(GTK_WIDGET(console));

}
