/* $Header: /fridge/cvs/xscorch/sconsole/swindow-console.c,v 1.14 2009-04-26 17:39:36 jacob Exp $ */
/*
   
   xscorch - swindow-console.c   Copyright(c) 2001-2003 Justin David Smith
   justins(at)chaos2.org         http://chaos2.org/
    
   Console interface to xscorch (server)
    

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
#define  _BSD_SOURCE          /* Needed for usleep */

#include <unistd.h>

#include <sgame/sconfig.h>
#include <sgame/splayer.h>
#include <sgame/sstate.h>
#include <swindow-console.h>



sc_window *sc_window_new(struct _sc_config *c, __libj_unused int argc, __libj_unused char **argv) {
/* sc_window_new */

   sc_window_console *w;      /* Window data structure */
   
   /* TEMP WARNING */
   fprintf(stderr, "xscorch-server: This program doesn't actually do anything (yet).\n");
   fprintf(stderr, "                Eventually it will allow for xscorch metaservers and\n");
   fprintf(stderr, "                for running a server without the graphical client.\n");
   
   /* Allocate the window data structure */
   w = (sc_window_console *)malloc(sizeof(sc_window_console));
   if(w == NULL) {
      fprintf(stderr, "Malloc error creating console window.\n");
      return(NULL);
   }
   
   /* Initialise the window data structure */
   w->c = c;
   
   /* That's all! */
   return((sc_window *)w);

}



void sc_window_free(sc_window **w) {
/* sc_window_free */

   if(w == NULL || *w == NULL) return;
   free(*w);
   *w = NULL;

}



void sc_window_run(sc_window *w_) {
/* sc_window_run */

   sc_window_console *w = (sc_window_console *)w_;
   if(w == NULL) return;
   while(true) {
      sc_state_run(w->c, w->c->game);
      usleep(SC_SLEEP_TIME * 1000);
   } /* Execution loop */

}



void sc_window_idle(__libj_unused sc_window *w) {
/* sc_window_idle */

}



void sc_window_update(__libj_unused sc_window *w) {
/* sc_winodw_update */

}



void sc_window_message(__libj_unused sc_window *w, const char *title,  const char *msg) {
/* sc_winodw_message */

   printf("%s: %s\n", title, msg);
   
}



void sc_status_update(__libj_unused sc_window *w, const sc_player *p) {
/* sc_status_update */

   printf("status: Player \"%s\" update.\n", p->name);

}



void sc_status_message(__libj_unused sc_window *w, const char *msg) {
/* sc_status_message */

   printf("status: %s\n", msg);
   
}



void sc_status_player_message(__libj_unused sc_window *w, const struct _sc_player *p, const char *msg) {
/* sc_status_player_message */

   printf("status: %s: %s\n", p->name, msg);

}



void sc_status_setup(__libj_unused sc_window *w) {
/* sc_status_setup */

}



void sc_status_suspend(__libj_unused sc_window *w) {
/* sc_status_suspend */

}



void sc_status_resume(__libj_unused sc_window *w) {
/* sc_status_resume */

}



void sc_window_paint(__libj_unused sc_window *w,
                     __libj_unused int x1, __libj_unused int y1,
                     __libj_unused int x2, __libj_unused int y2,
                     __libj_unused int flag) {
/* sc_window_paint */

}



void sc_window_paint_circular(__libj_unused sc_window *w, __libj_unused int centerx, __libj_unused int centery,
                              __libj_unused int rad, __libj_unused int flag) {
/* sc_window_paint_circular */

}



void sc_window_resize(__libj_unused sc_window *w) {
/* sc_window_resize */

}



void sc_window_draw_tank(__libj_unused sc_window *w, __libj_unused const struct _sc_player *p) {
/* sc_window_draw_tank */

}



void sc_window_undraw_tank(__libj_unused sc_window *w, __libj_unused const struct _sc_player *p) {
/* sc_window_undraw_tank */

}



void sc_window_redraw_tank(__libj_unused sc_window *w, __libj_unused const struct _sc_player *p) {
/* sc_window_redraw_tank */

}



void sc_window_draw_weapon(__libj_unused sc_window *w, __libj_unused const struct _sc_weapon *wp) {
/* sc_window_draw_weapon */

}



void sc_window_undraw_weapon(__libj_unused sc_window *w, __libj_unused const struct _sc_weapon *wp) {
/* sc_window_undraw_weapon */

}



void sc_window_main_menu(__libj_unused sc_window *w) {
/* sc_window_main_menu */

}



void sc_window_paint_blank(__libj_unused sc_window *w) {
/* sc_window_paint_blank */

}



void sc_window_paint_end_round(__libj_unused sc_window *w) {
/* sc_window_paint_end_round */

   printf("End of round.\n");

}



void sc_window_paint_end_game(__libj_unused sc_window *w) {
/* sc_window_paint_end_game */

   printf("End of game.\n");

}



void sc_window_inventory(__libj_unused sc_window *w, __libj_unused struct _sc_player *p) {
/* sc_window_inventory */

}



void sc_window_auto_defense(__libj_unused sc_window *w, __libj_unused struct _sc_player *p) {
/* sc_window_auto_defense */

}



void sc_window_lottery_result(__libj_unused sc_window *w, __libj_unused bool showstake) {
/* sc_window_lottery_result */

}



int sc_expl_cache_new(__libj_unused sc_window *w, __libj_unused int radius,
                      __libj_unused enum _sc_explosion_type type) {
/* sc_expl_cache_new */

   return(0);
   
}



void sc_expl_cache_draw(__libj_unused sc_window *w, __libj_unused int ptr,
                        __libj_unused int centerx, __libj_unused int centery, __libj_unused int cacheid) {
/* sc_expl_cache_draw */

}



void sc_window_draw_arc(__libj_unused sc_window *w, __libj_unused struct _sc_trajectory *tr, __libj_unused int playerid) {
/* sc_window_draw_arc */

}



void sc_window_clear_arc(__libj_unused sc_window *w, __libj_unused struct _sc_trajectory *tr) {
/* sc_window_clear_arc */

}



void sc_window_draw_napalm_frame(__libj_unused sc_window *w, __libj_unused const int *xlist,
                                 __libj_unused const int *ylist, __libj_unused int size) {
/* sc_window_draw_napalm_frame */

}



void sc_window_draw_napalm_final(__libj_unused sc_window *w, __libj_unused const int *xlist,
                                 __libj_unused const int *ylist, __libj_unused int totalsize) {
/* sc_window_draw_napalm_final */

}



void sc_window_clear_napalm(__libj_unused sc_window *w, __libj_unused const int *xlist,
                            __libj_unused const int *ylist, __libj_unused int totalsize) {
/* sc_window_clear_napalm */

}



void sc_chat_window_update(__libj_unused sc_window *w, const char *msg) {
/* sc_chat_window_update */

   printf("chat: %s\n", msg);
   
}
