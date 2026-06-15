/* $Header: /fridge/cvs/xscorch/sgtk/sstatus-gtk.c,v 1.38 2009-04-26 17:39:50 jacob Exp $ */
/*

   xscorch - sstatus-gtk.c    Copyright(c) 2000-2004 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   GTK top statusbar


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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <sgtk.h>
#include <sactiveconsole.h>
#include <sconsole.h>

#include <sstatus-gtk.h>
#include <scolor-gtk.h>
#include <stank-gtk.h>

#include <sgame/sgame.h>
#include <sgame/sinventory.h>
#include <sgame/sphysics.h>
#include <sgame/splayer.h>
#include <sgame/sshield.h>
#include <sgame/sstate.h>
#include <sgame/stankpro.h>
#include <sgame/sweapon.h>

#include <libj/jstr/libjstr.h>



#define  SC_TWO_ROW_BOUNDARY        90



typedef enum _sc_status_state {
   SC_STATUS_NORMAL,
   SC_STATUS_PLAYER,
   SC_STATUS_BOLD
} sc_status_state;



#define  SC_STATUS_TOGGLE_WEAPON    0x01
#define  SC_STATUS_TOGGLE_SHIELD    0x02
#define  SC_STATUS_TOGGLE_TRIGGER   0x03
#define  SC_STATUS_ACTIVATE_SHIELD  0x10
#define  SC_STATUS_ACTIVATE_BATTERY 0x11
#define  SC_STATUS_MOVE_PLAYER      0x20



static void _sc_status_reset_state(sc_window_gtk *w, gboolean sens) {

   w->statenabled = sens;
   gtk_widget_set_sensitive(GTK_WIDGET(w->status), sens);
   sc_active_console_detach_all_spots(SC_ACTIVE_CONSOLE(w->status));
   sc_console_clear(SC_CONSOLE(w->status));

}



static void _sc_status_write_line(const sc_window_gtk *w, const sc_player *p, int x, int y,
                                  const char *text, sc_status_state state) {

   switch(state) {
   case SC_STATUS_NORMAL:
      break;
   case SC_STATUS_PLAYER:
      sc_console_highlight_attach(SC_CONSOLE(w->status),
                                  &w->colormap->pcolors[p->index], NULL,
                                  FALSE, x, y, strlenn(text), 1);
      break;
   case SC_STATUS_BOLD:
      sc_console_highlight_attach(SC_CONSOLE(w->status),
                                  &w->colormap->white, NULL,
                                  TRUE, x, y, strlenn(text), 1);
      break;
   } /* what highlight mode? */
   sc_console_write_line(SC_CONSOLE(w->status), x, y, text);

}



static gboolean _sc_status_activate_item(__libj_unused GtkWidget *widget,
                                         ScActiveConsoleSpot *spot,
                                         sc_window_gtk *w) {

   sc_player *curplayer = w->c->plorder[w->c->game->curplayer];

   /* Make sure the game is enabled at this point */
   if(!SC_STATE_IS_ENABLED(w->c->game) || SC_STATE_IS_PAUSE(w->c->game)) return(FALSE);

   /* What action to perform? */
   switch((long)spot->data) {
      case SC_STATUS_TOGGLE_WEAPON:
         sc_player_advance_weapon(w->c, curplayer, 1);
         return(TRUE);
      case SC_STATUS_TOGGLE_SHIELD:
         sc_player_advance_shield(w->c, curplayer, SC_PLAYER_SHIELD_DEFAULTS);
         return(TRUE);
      case SC_STATUS_TOGGLE_TRIGGER:
         sc_player_set_contact_triggers(w->c, curplayer, !curplayer->contacttriggers);
         return(TRUE);
      case SC_STATUS_ACTIVATE_SHIELD:
         sc_player_activate_shield(w->c, curplayer);
         return(TRUE);
      case SC_STATUS_ACTIVATE_BATTERY:
         sc_player_activate_battery(w->c, curplayer);
         return(TRUE);
      case SC_STATUS_MOVE_PLAYER:
         sc_window_tank_move_gtk(w, curplayer);
         return(TRUE);
   }
   return(FALSE);

}



void sc_status_update(sc_window *w_, const sc_player *p) {

   int size;
   const sc_weapon_info *wi;
   const sc_accessory_info *si;
   char buf[SC_GTK_STRING_BUFFER];
   sc_window_gtk *w = (sc_window_gtk *)w_;
   int start;
   int spot_start;
   int end_line_one;

   if(w == NULL) return;
   wi = p->selweapon;
   si = p->selshield;
   if(wi == NULL || si == NULL) {
      fprintf(stderr, "warning: player is missing either shields or weapons, status NOT updated!\n");
      return;
   }

   sc_window_redraw_tank(w_, p);

   if(p->aitype == SC_AI_NETWORK) return;

   size = w->c->fieldwidth / sc_console_get_col_width(SC_CONSOLE(w->status));

   _sc_status_reset_state(w, TRUE);
   _sc_status_write_line(w, p, size - strlenn(p->name) - 2, size < SC_TWO_ROW_BOUNDARY ? 1 : 0,
                         p->name, SC_STATUS_PLAYER);

   if(size < SC_TWO_ROW_BOUNDARY) {
      /* We must scale player tank life to display in game life units. */
      sbprintf(buf, sizeof(buf), "Ang: %3d %c  Pwr: %4d  %c  Shld: %3d%%  %3d%c",
               (p->turret >= 90 ? 180 - p->turret : p->turret), p->turret > 90 ? '<' : '>',
               p->power, (p->contacttriggers ? 'T' : ' '),
               p->shield != NULL ? p->shield->life * 100 / p->shield->info->shield : 0,
               si->shield / 100, SC_ACCESSORY_SHIELD_CHAR(si));
      sc_console_write_line(SC_CONSOLE(w->status), 1, 0, buf);
      sbprintf(buf, sizeof(buf), "%2d %-16s   Wind: %4d %c",
               wi->inventories[p->index], wi->name,
               abs((int)(w->c->physics->curwind * 1000 / SC_PHYSICS_WIND_MAX)),
                   (w->c->physics->curwind >= 0 ? '>' : '<'));
      sc_console_write_line(SC_CONSOLE(w->status), 1, 1, buf);
   } else {
      /* Skip the first column */
      start = 1;

      sbprintf(buf, sizeof(buf), "[Ang: %2d %c]",
               p->turret >= 90 ? 180 - p->turret : p->turret, p->turret > 90 ? '<' : '>');
      _sc_status_write_line(w, p, start, 0, buf, SC_STATUS_NORMAL);
      start += 11;

      sbprintf(buf, sizeof(buf), "[Pwr: %4d]", p->power);
      _sc_status_write_line(w, p, start, 0, buf, SC_STATUS_NORMAL);
      start += 11;

      _sc_status_write_line(w, p, start, 0, "[Shld:           Cur:    ]", SC_STATUS_NORMAL);
      if(SC_ACCESSORY_IS_NULL(si)) {
         sbprintf(buf, sizeof(buf), "    None");
      } else {
         sbprintf(buf, sizeof(buf), "%2dx %3d%c", si->inventories[p->index],
                  si->shield / 100, SC_ACCESSORY_SHIELD_CHAR(si));
      }
      spot_start = start + 7;
      sc_active_console_add_spot(SC_ACTIVE_CONSOLE(w->status), spot_start, 0, 8, 1, (gpointer)SC_STATUS_TOGGLE_SHIELD);
      _sc_status_write_line(w, p, spot_start, 0, buf, SC_ACCESSORY_IS_NULL(si) ? SC_STATUS_BOLD : SC_STATUS_NORMAL);
      spot_start += 14;
      sc_active_console_add_spot(SC_ACTIVE_CONSOLE(w->status), spot_start, 0, 4, 1, (gpointer)SC_STATUS_ACTIVATE_SHIELD);
      sbprintf(buf, sizeof(buf), "%3d%%", p->shield != NULL ? p->shield->life * 100 / p->shield->info->shield : 0);
      _sc_status_write_line(w, p, spot_start, 0, buf, p->shield == NULL ? SC_STATUS_BOLD : SC_STATUS_NORMAL);
      start += 26;

      _sc_status_write_line(w, p, start, 0, "[                       ]", SC_STATUS_NORMAL);
      if(SC_WEAPON_IS_NULL(wi)) {
         sbprintf(buf, sizeof(buf), "   ");
      } else {
         sbprintf(buf, sizeof(buf), "%2dx", wi->inventories[p->index]);
      }
      spot_start = start + 1;
      sc_active_console_add_spot(SC_ACTIVE_CONSOLE(w->status), spot_start, 0, 23, 1, (gpointer)SC_STATUS_TOGGLE_WEAPON);
      _sc_status_write_line(w, p, spot_start, 0, buf, SC_STATUS_NORMAL);
      spot_start += 4;
      sbprintf(buf, sizeof(buf), "%-16s ", wi->name);
      _sc_status_write_line(w, p, spot_start, 0, buf, SC_WEAPON_IS_NULL(wi) ? SC_STATUS_BOLD : SC_STATUS_NORMAL);
      spot_start += 18;
      sbprintf(buf, sizeof(buf), "%c", p->contacttriggers ? 'T' : ' ');
      _sc_status_write_line(w, p, spot_start, 0, buf, SC_STATUS_NORMAL);
      start += 25;
      end_line_one = start;

      if(w->c->options.extstatus) {
         start = 1;
         sbprintf(buf, sizeof(buf), "Round %d/%d", w->c->curround + 1, w->c->numrounds);
         _sc_status_write_line(w, p, size - strlenn(buf) - 2, 1, buf, SC_STATUS_NORMAL);

         sbprintf(buf, sizeof(buf), "[Life: %4d]", INT_ROUND_DIV(p->life, p->tank->hardness));
         _sc_status_write_line(w, p, start, 1, buf, SC_STATUS_PLAYER);
         start += 12;

         sbprintf(buf, sizeof(buf), "[Batt: %2d]", sc_player_battery_count(w->c, p));
         spot_start = start + 1;
         sc_active_console_add_spot(SC_ACTIVE_CONSOLE(w->status), spot_start, 1, 8, 1, (gpointer)SC_STATUS_ACTIVATE_BATTERY);
         _sc_status_write_line(w, p, start, 1, buf, SC_STATUS_PLAYER);
         start += 10;

         sbprintf(buf, sizeof(buf), "[Trig: %2d]", sc_player_contact_trigger_count(w->c, p));
         spot_start = start + 1;
         sc_active_console_add_spot(SC_ACTIVE_CONSOLE(w->status), spot_start, 1, 8, 1, (gpointer)SC_STATUS_TOGGLE_TRIGGER);
         _sc_status_write_line(w, p, start, 1, buf, SC_STATUS_PLAYER);
         start += 10;

         sbprintf(buf, sizeof(buf), "[Fuel: %4d]", sc_player_total_fuel(w->c->accessories, p));
         spot_start = start + 1;
         sc_active_console_add_spot(SC_ACTIVE_CONSOLE(w->status), spot_start, 1, 10, 1, (gpointer)SC_STATUS_MOVE_PLAYER);
         _sc_status_write_line(w, p, start, 1, buf, SC_STATUS_PLAYER);
         start += 28;

         sbprintf(buf, sizeof(buf), "[Wind: %3d %c]",
                  abs((int)(w->c->physics->curwind * 1000 / SC_PHYSICS_WIND_MAX)),
                      (w->c->physics->curwind >= 0 ? '>' : '<'));
         _sc_status_write_line(w, p, start, 1, buf, SC_STATUS_NORMAL);
         start += 13;
         assert(end_line_one == start);
      } /* Extended status */
   }

}



void sc_status_player_message(sc_window *w_, const sc_player *p, const char *msg) {

   sc_window_gtk *w = (sc_window_gtk *)w_;
   char buf[SC_GTK_STRING_BUFFER];

   if(w == NULL) return;

   sbprintf(buf, sizeof(buf), " %s:  %s", p->name, msg);

   _sc_status_reset_state(w, FALSE);
   _sc_status_write_line(w, p, 1, 0, buf, SC_STATUS_PLAYER);

}



void sc_status_message(sc_window *w_, const char *msg) {

   sc_window_gtk *w = (sc_window_gtk *)w_;

   if(w == NULL) return;

   _sc_status_reset_state(w, FALSE);
   sc_console_write_line(SC_CONSOLE(w->status), 1, 0, msg);

}



void sc_status_setup(sc_window *w_) {

   sc_window_gtk *w = (sc_window_gtk *)w_;
   int size;
   int rows;

   if(w == NULL) return;

   rows = 1;
   size = w->c->fieldwidth / sc_console_get_col_width(SC_CONSOLE(w->status));
   if(size < SC_TWO_ROW_BOUNDARY || w->c->options.extstatus) ++rows;
   sc_active_console_init(SC_ACTIVE_CONSOLE(w->status), 0, 0, size, rows, CONSOLE_BORDERLESS,
                          w->fixed_font, w->bold_fixed_font);
   gtk_widget_set_sensitive(GTK_WIDGET(w->status), FALSE);

}



void sc_status_suspend(sc_window *_w) {

   sc_window_gtk *w = (sc_window_gtk *)_w;
   gtk_widget_set_sensitive(GTK_WIDGET(w->status), FALSE);

}



void sc_status_resume(sc_window *_w) {

   sc_window_gtk *w = (sc_window_gtk *)_w;
   gtk_widget_set_sensitive(GTK_WIDGET(w->status), w->statenabled);

}



void sc_status_configure_gtk(sc_window_gtk *w) {

   g_signal_connect_after(G_OBJECT(w->status), "select-spot",
                          (GCallback)_sc_status_activate_item, w);
   sc_active_console_set_allow_keyboard(SC_ACTIVE_CONSOLE(w->status), FALSE);
   _sc_status_reset_state(w, FALSE);

}
