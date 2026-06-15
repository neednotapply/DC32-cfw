/* $Header: /fridge/cvs/xscorch/sgtk/stank-gtk.c,v 1.23 2009-04-26 17:39:50 jacob Exp $ */
/*
   
   xscorch - stank-gtk.c      Copyright(c) 2000-2004 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/
    
   Tank configuration screen
    

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

#include <sgame/saccessory.h>
#include <sgame/seconomy.h>
#include <sgame/sgame.h>
#include <sgame/sinventory.h>
#include <sgame/splayer.h>
#include <sgame/sshield.h>
#include <sgame/stankpro.h>
#include <sgame/sweapon.h>

#include <gdk/gdkkeysyms.h>
#include <libj/jstr/libjstr.h>



static gboolean _sc_window_tank_close_gtk(__libj_unused GtkWidget *widget,
                                          __libj_unused ScActiveConsoleSpot *spot,
                                          sc_window_gtk *w) {

   sc_display_console_detach_all(SC_DISPLAY(w->screen));
   sc_game_unpause(w->c, w->c->game);
   return(TRUE);

}



static gboolean _sc_window_tank_info_key_gtk(__libj_unused GtkWidget *widget,
                                             __libj_unused ScActiveConsoleSpot *spot,
                                             GdkEventKey *key, sc_window_gtk *w) {

   switch(key->keyval) {
      case GDK_Return:
      case GDK_KP_Enter:
      case GDK_Escape:
         sc_display_console_detach_all(SC_DISPLAY(w->screen));
         sc_game_unpause(w->c, w->c->game);
         return(TRUE);
   }
   return(FALSE);

}



void sc_window_tank_info_gtk(sc_window_gtk *w, sc_player *p) {

   char buf[SC_GTK_STRING_BUFFER];
   GtkWidget *console;

   console = sc_window_active_console_new(w, 8, 4, 46, 7, CONSOLE_NORMAL);
   sc_console_set_foreground(SC_CONSOLE(console), &w->colormap->pcolors[p->index]);

   sc_console_write_line(SC_CONSOLE(console), 36, 6, "< Close >");
   sc_active_console_add_spot(SC_ACTIVE_CONSOLE(console), 36, 6, 9, 1, NULL);
   g_signal_connect_after(G_OBJECT(console), "key_press_spot",
                          (GCallback)_sc_window_tank_info_key_gtk, w);
   g_signal_connect_after(G_OBJECT(console), "select-spot",
                          (GCallback)_sc_window_tank_close_gtk, w);
   
   sbprintf(buf, sizeof(buf), "%2d: %s (%s)", 
            p->index + 1, p->name, sc_ai_name(p->aitype));
   sc_console_write_line(SC_CONSOLE(console), 0, 0, buf);
   /* We must scale player tank life to display in game life units. */
   sbprintf(buf, sizeof(buf), "    Life:     %-5d     Shield:  %-5d", 
            INT_ROUND_DIV(p->life, p->tank->hardness),
            p->shield == NULL ? 0 : p->shield->life);
   sc_console_write_line(SC_CONSOLE(console), 0, 1, buf);
   sbprintf(buf, sizeof(buf), "    Wins:     %-2d        Kills:   %-2d", 
            p->numwins, p->kills);
   sc_console_write_line(SC_CONSOLE(console), 0, 2, buf);
   sbprintf(buf, sizeof(buf), "    Suicides: %d", 
            p->suicides);
   sc_console_write_line(SC_CONSOLE(console), 0, 3, buf);
   sbprintf(buf, sizeof(buf), "    Money:    $%-8d %c$%d this round", 
            p->money, p->money > p->oldmoney ? '+' : '-', p->money - p->oldmoney);
   sc_console_write_line(SC_CONSOLE(console), 0, 4, buf);
   sbprintf(buf, sizeof(buf), "    Battery:  %-2d        Trigger: %-2d",
            sc_player_battery_count(w->c, p), sc_player_contact_trigger_count(w->c, p));
   sc_console_write_line(SC_CONSOLE(console), 0, 5, buf);
   sbprintf(buf, sizeof(buf), "Press [Enter] to close");
   sc_console_write_line(SC_CONSOLE(console), 0, 6, buf);

   sc_display_console_attach(SC_DISPLAY(w->screen), SC_CONSOLE(console));

   sc_game_pause(w->c, w->c->game);

   /* focus on this tank */
   gtk_window_set_default(GTK_WINDOW(w->app), GTK_WIDGET(console));
   gtk_widget_grab_focus(GTK_WIDGET(console));
   
}



static gboolean _sc_window_tank_move_btn_gtk(GtkWidget *widget, ScActiveConsoleSpot *spot, sc_window_gtk *w) {

   char buf[SC_GTK_STRING_BUFFER];
   int fuel;

   switch((long)spot->data) {
      case 1:
         if(sc_player_move(w->c, w->c->plorder[w->c->game->curplayer], -1)) {
            fuel = sc_player_total_fuel(w->c->accessories, w->c->plorder[w->c->game->curplayer]);
            sbprintf(buf, sizeof(buf), "%4d", fuel);
            sc_console_write_line(SC_CONSOLE(widget), 22, 0, buf);
         }
         return(TRUE);
      case 2:
         if(sc_player_move(w->c, w->c->plorder[w->c->game->curplayer], +1)) {
            fuel = sc_player_total_fuel(w->c->accessories, w->c->plorder[w->c->game->curplayer]);
            sbprintf(buf, sizeof(buf), "%4d", fuel);
            sc_console_write_line(SC_CONSOLE(widget), 22, 0, buf);
         }
         return(TRUE);
      case 3:
         sc_display_console_detach_all(SC_DISPLAY(w->screen));
         sc_game_unpause(w->c, w->c->game);
         return(TRUE);
   }
   return(FALSE);

}



static gboolean _sc_window_tank_move_key_gtk(GtkWidget *widget, 
                                             __libj_unused ScActiveConsoleSpot *spot,
                                             GdkEventKey *key, sc_window_gtk *w) {

   char buf[SC_GTK_STRING_BUFFER];
   int fuel;

   switch(key->keyval) {
      case GDK_Left:
      case GDK_KP_Left:
         if(sc_player_move(w->c, w->c->plorder[w->c->game->curplayer], -1)) {
            fuel = sc_player_total_fuel(w->c->accessories, w->c->plorder[w->c->game->curplayer]);
            sbprintf(buf, sizeof(buf), "%4d", fuel);
            sc_console_write_line(SC_CONSOLE(widget), 22, 0, buf);
         }
         return(TRUE);
      case GDK_Right:
      case GDK_KP_Right:
         if(sc_player_move(w->c, w->c->plorder[w->c->game->curplayer], +1)) {
            fuel = sc_player_total_fuel(w->c->accessories, w->c->plorder[w->c->game->curplayer]);
            sbprintf(buf, sizeof(buf), "%4d", fuel);
            sc_console_write_line(SC_CONSOLE(widget), 22, 0, buf);
         }
         return(TRUE);
      case GDK_Return:
      case GDK_KP_Enter:
      case GDK_Escape:
         sc_display_console_detach_all(SC_DISPLAY(w->screen));
         sc_game_unpause(w->c, w->c->game);
         return(TRUE);
   }
   return(FALSE);

}



void sc_window_tank_move_gtk(sc_window_gtk *w, sc_player *p) {

   char buf[SC_GTK_STRING_BUFFER];
   GtkWidget *console;
   int x;

   if(p->x < w->c->fieldwidth / 2) {
      x = w->c->fieldwidth / (2 * sc_console_get_col_width(SC_CONSOLE(w->status))) + 1;
   } else {
      x = 2;
   }
   console = sc_window_active_console_new(w, x, 2, 26, 8, CONSOLE_NORMAL);
   sc_console_set_foreground(SC_CONSOLE(console), &w->colormap->pcolors[p->index]);

   sc_console_write_line(SC_CONSOLE(console), 0, 0, p->name);
   
   sbprintf(buf, sizeof(buf), "%4d", sc_player_total_fuel(w->c->accessories, p));
   sc_console_write_line(SC_CONSOLE(console), 22, 0, buf);
   
   sc_console_write_line(SC_CONSOLE(console), 0, 1, "Right means left and left");
   sc_console_write_line(SC_CONSOLE(console), 0, 2, "means right. Go which way?");
   sc_console_write_line(SC_CONSOLE(console), 0, 3, "(arrows/buttons move tank,");
   sc_console_write_line(SC_CONSOLE(console), 0, 4, "  [Enter] closes window)");

   g_signal_connect_after(G_OBJECT(console), "key_press_spot",
                          (GCallback)_sc_window_tank_move_key_gtk, w);
   g_signal_connect_after(G_OBJECT(console), "select-spot",
                          (GCallback)_sc_window_tank_move_btn_gtk, w);

   sc_console_write_line(SC_CONSOLE(console), 2, 6, "< Left >");
   sc_active_console_add_spot(SC_ACTIVE_CONSOLE(console), 2, 6, 8, 1, (void *)1);
   
   sc_console_write_line(SC_CONSOLE(console), 15,6, "< Right >");
   sc_active_console_add_spot(SC_ACTIVE_CONSOLE(console), 15,6, 9, 1, (void *)2);
   
   sc_console_write_line(SC_CONSOLE(console), 15,7, "< Close >");
   sc_active_console_add_spot(SC_ACTIVE_CONSOLE(console), 15,7, 9, 1, (void *)3);
   
   sc_display_console_attach(SC_DISPLAY(w->screen), SC_CONSOLE(console));
   
   sc_game_pause(w->c, w->c->game);

   /* focus on this tank */
   gtk_window_set_default(GTK_WINDOW(w->app), GTK_WIDGET(console));
   gtk_widget_grab_focus(GTK_WIDGET(console));
   
}
