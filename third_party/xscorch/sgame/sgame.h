/* $Header: /fridge/cvs/xscorch/sgame/sgame.h,v 1.9 2009-04-26 17:39:39 jacob Exp $ */
/*
   
   xscorch - sgame.h          Copyright(c) 2001,2000 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Scorched game state header
    

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
#ifndef __sgame_h_included
#define __sgame_h_included


#include <xscorch.h>
#include <ssound/ssound.h>


/* Forward structures */
struct _sc_explosion;
struct _sc_config;


typedef struct _sc_game {
   int curplayer;                /* Current player */

   /* Current audio tracks */
   sc_sound_music musicid;       /* Musical Ident */

   /* State machine parametres */
   struct timeval timeout;       /* Time until next state is run */
   int state;                    /* Next state to execute */

   /* State flags */
   int substate;                 /* Substate flag (e.g. player number) */
   bool activity;                /* In interleave: was there activity this round? */
   struct _sc_explosion *expl_init;    /* Queue holding explosions waiting to start */
   struct _sc_explosion *expl_draw;    /* Queue holding explosions currently drawing */
   struct _sc_explosion *expl_clear_init; /* .. holding explosions waitint to clear */
   struct _sc_explosion *expl_clear;   /* Queue holding explosions currently clearing */
   struct _sc_explosion *expl_done;    /* Queue holding explosions that are done */
} sc_game;


typedef bool (*sc_game_run_fn)(struct _sc_config *c, sc_game *g, struct _sc_explosion *e);


/* General game state and state machine functions */
sc_game *sc_game_new(void);
void sc_game_free(sc_game **g);
void sc_game_init(sc_game *g);
void sc_game_time(struct timeval *gametime);
void sc_game_mass_kill(struct _sc_config *c, sc_game *g);
void sc_game_set_state(sc_game *g, int state, unsigned long delay);
void sc_game_set_state_now(struct _sc_config *c, sc_game *g, int state);
void sc_game_set_state_asap(sc_game *g, int state);
void sc_game_set_state_allow_now(struct _sc_config *c, sc_game *g, int state, unsigned long delay);
void sc_game_reinstate(sc_game *g, unsigned long delay);
void sc_game_reinstate_now(struct _sc_config *c, sc_game *g);
void sc_game_reinstate_allow_now(struct _sc_config *c, sc_game *g, unsigned long delay);
void sc_game_sync_timeout(sc_game *g);
void sc_game_pause(struct _sc_config *c, sc_game *g);
void sc_game_unpause(struct _sc_config *c, sc_game *g);
int  sc_game_victor(struct _sc_config *c);
void sc_game_set_victor(struct _sc_config *c);


/* Explosion queues */
void sc_game_expl_queue_append(struct _sc_explosion **queue, struct _sc_explosion *e);
void sc_game_expl_queue_head_move(struct _sc_explosion **dst, struct _sc_explosion **src);
void sc_game_expl_queue_head_move_prepend(struct _sc_explosion **dst, struct _sc_explosion **src);
void sc_game_expl_queue_move(struct _sc_explosion **dst, struct _sc_explosion **src);
void sc_game_expl_queue_reverse(struct _sc_explosion **queue);
void sc_game_expl_queue_head_free(struct _sc_explosion **queue);
void sc_game_expl_queue_item_free(struct _sc_explosion **queue, struct _sc_explosion *e);
void sc_game_expl_queue_item_move(struct _sc_explosion **dst, struct _sc_explosion **src, struct _sc_explosion *e);
void sc_game_expl_queue_item_move_prepend(struct _sc_explosion **dst, struct _sc_explosion **src, struct _sc_explosion *e);
bool sc_game_expl_queue_do_runnable(struct _sc_config *c, struct _sc_game *g, struct _sc_explosion *queue, sc_game_run_fn f);
bool sc_game_expl_queue_run_first(struct _sc_config *c, struct _sc_game *g, struct _sc_explosion *queue, sc_game_run_fn f);
void sc_game_expl_queue_postpone(struct _sc_explosion *e, unsigned long delay);
void sc_game_expl_queue_update(struct _sc_explosion *e);


#endif /* __sgame_h_included? */
