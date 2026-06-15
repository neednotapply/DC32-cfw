/* $Header: /fridge/cvs/xscorch/sgame/swindow.h,v 1.9 2009-04-26 17:39:45 jacob Exp $ */
/*
   
   xscorch - swindow.h        Copyright(c) 2000 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Scorched generic window interface
    

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
#ifndef __swindow_h_included
#define __swindow_h_included


#include <xscorch.h>
#include <sgame/sexplosion.h>


/* Forward declarations */
struct _sc_trajectory;
struct _sc_config;
struct _sc_player;
struct _sc_weapon;
struct _sc_color;


#define  SC_REGENERATE_LAND      0x0001
#define  SC_REDRAW_LAND          0x0002
#define  SC_REDRAW_TANKS         0x0004
#define  SC_REDRAW_WIND_ARROW    0x0008
#define  SC_CLEAR_WIND_ARROW     0x0010
#define  SC_PAINT_EVERYTHING     (~SC_REGENERATE_LAND)


typedef struct _sc_window {
   void *data;       /* Not allowed to have an empty struct */
} sc_window;


/* Basic window manipulation calls */
sc_window *sc_window_new(struct _sc_config *c, int argc, char **argv);
void sc_window_free(sc_window **w);
void sc_window_run(sc_window *w);
void sc_window_idle(sc_window *w);
void sc_window_update(sc_window *w);
void sc_window_message(sc_window *w, const char *title,  const char *msg);


/* Calls related to the status bar */
void sc_status_update(sc_window *w, const struct _sc_player *p);
void sc_status_message(sc_window *w, const char *msg);
void sc_status_player_message(sc_window *w, const struct _sc_player *p, const char *msg);
void sc_status_setup(sc_window *w);
void sc_status_suspend(sc_window *w);
void sc_status_resume(sc_window *w);


/* Calls for drawing the game screen */
void sc_window_paint(sc_window *w, int x1, int y1, int x2, int y2, int flag);
void sc_window_paint_circular(sc_window *w, int centerx, int centery, int rad, int flag);
void sc_window_resize(sc_window *w);


/* Calls for drawing the tanks */
void sc_window_draw_tank(sc_window *w, const struct _sc_player *p);
void sc_window_undraw_tank(sc_window *w, const struct _sc_player *p);
void sc_window_redraw_tank(sc_window *w, const struct _sc_player *p);


/* Calls for drawing the weapons */
void sc_window_draw_weapon(sc_window *w, const struct _sc_weapon *wp);
void sc_window_undraw_weapon(sc_window *w, const struct _sc_weapon *wp);


/* Calls for painting other types of screens */
void sc_window_main_menu(sc_window *w);
void sc_window_paint_blank(sc_window *w);


/* End of round/end of game */
void sc_window_paint_end_round(sc_window *w);
void sc_window_paint_end_game(sc_window *w);


/* Inventory windows */
void sc_window_inventory(sc_window *w, struct _sc_player *p);
void sc_window_auto_defense(sc_window *w, struct _sc_player *p);
void sc_window_lottery_result(sc_window *w, bool showstake);


/* Explosion cache */
int sc_expl_cache_new(sc_window *w, int radius, enum _sc_explosion_type type);
void sc_expl_cache_draw(sc_window *w, int ptr, int centerx, int centery, int cacheid);


/* Arc drawing */
void sc_window_draw_arc(sc_window *w, struct _sc_trajectory *tr, int playerid);
void sc_window_clear_arc(sc_window *w, struct _sc_trajectory *tr);


/* Spill functions */
void sc_window_draw_napalm_frame(sc_window *w, const int *xlist, const int *ylist, int size);
void sc_window_draw_napalm_final(sc_window *w, const int *xlist, const int *ylist, int totalsize);
void sc_window_clear_napalm(sc_window *w, const int *xlist, const int *ylist, int totalsize);


/* Interface timer */
/*void sc_window_timer_enable(sc_window *w);*/
/*void sc_window_timer_disable(sc_window *w);*/


/* Network status update functions */
void sc_chat_window_update(sc_window *w, const char *msg);


#endif /* __swindow_h_included */
