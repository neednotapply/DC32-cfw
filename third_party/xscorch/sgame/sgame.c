/* $Header: /fridge/cvs/xscorch/sgame/sgame.c,v 1.17 2009-04-26 17:39:39 jacob Exp $ */
/*

   xscorch - sgame.c          Copyright(c) 2000-2004 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Scorched game state machine


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

#include <sgame.h>         /* Game state header */

#include <sconfig.h>       /* Config is dereferenced frequently */
#include <seconomy.h>      /* Need to do interest update */
#include <sland.h>         /* Need to drop the land after deton */
#include <sphysics.h>      /* Need to do Wind updates */
#include <splayer.h>       /* Dereferenced in death check, etc. */
#include <sstate.h>        /* Game state header */
#include <strack.h>        /* Need for tracking, return codes */
#include <sweapon.h>       /* We construct weapon chains here */
#include <swindow.h>       /* We do much drawing from state machine */

#include <sai/sai.h>       /* We are responsible for running AI */
#include <sutil/srand.h>   /* Need a random number generator */



sc_game *sc_game_new(void) {

   sc_game *g;

   g = (sc_game *)malloc(sizeof(sc_game));
   if(g == NULL) return(NULL);

   g->musicid = SC_MUSIC_PRELUDE;
   g->expl_init      = NULL;
   g->expl_draw      = NULL;
   g->expl_clear_init= NULL;
   g->expl_clear     = NULL;
   g->expl_done      = NULL;
   sc_game_init(g);
   return(g);

}



void sc_game_free(sc_game **g) {

   if(g == NULL || *g == NULL) return;
   sc_expl_free_chain(&(*g)->expl_init);
   sc_expl_free_chain(&(*g)->expl_draw);
   sc_expl_free_chain(&(*g)->expl_clear_init);
   sc_expl_free_chain(&(*g)->expl_clear);
   sc_expl_free_chain(&(*g)->expl_done);
   free(*g);
   *g = NULL;

}



inline void sc_game_time(struct timeval *gametime) {

   gettimeofday(gametime, NULL);

}



inline void sc_game_reinstate(sc_game *g, unsigned long delay) {

   #if SC_STATE_TIMER_DEBUG
      struct timeval curtime;
   #endif

   g->timeout.tv_usec += (delay % 1000) * 1000;
   while(g->timeout.tv_usec >= 1000000) {
      g->timeout.tv_usec -= 1000000;
      g->timeout.tv_sec++;
   }
   g->timeout.tv_sec += (delay / 1000);

   #if SC_STATE_TIMER_DEBUG
      sc_game_time(&curtime);
      printf("Next state %8x activates at %ld.%06ld (%+5ld; current %ld.%06ld)\n",
             g->state, g->timeout.tv_sec, g->timeout.tv_usec, delay, curtime.tv_sec, curtime.tv_usec);
   #endif

}



inline void sc_game_reinstate_now(sc_config *c, sc_game *g) {

   sc_game_time(&g->timeout);

   #if SC_STATE_TIMER_DEBUG
      printf("Next state %8x activates at %ld.%06ld (now)\n",
             g->state, g->timeout.tv_sec, g->timeout.tv_usec);
   #endif

   sc_state_run(c, g);

}



inline void sc_game_reinstate_asap(sc_game *g) {

   sc_game_time(&g->timeout);

   #if SC_STATE_TIMER_DEBUG
      printf("Next state %8x activates at %ld.%06ld (asap)\n",
             g->state, g->timeout.tv_sec, g->timeout.tv_usec);
   #endif

}



inline void sc_game_reinstate_allow_now(sc_config *c, sc_game *g, unsigned long delay) {

   if(delay <= 0) sc_game_reinstate_now(c, g);
   else sc_game_reinstate(g, delay);

}



inline void sc_game_set_state(sc_game *g, int state, unsigned long delay) {

   g->state    = state;
   sc_game_reinstate(g, delay);

}



inline void sc_game_set_state_now(sc_config *c, sc_game *g, int state) {

   g->state    = state;
   sc_game_reinstate_now(c, g);

}



inline void sc_game_set_state_asap(sc_game *g, int state) {
/* sc_game_set_state_asap
   API calls which want to start the state machine up again on next timer
   should call this, rather than sc_game_set_state_now; otherwise the
   state machine may run for a long time (processing AI commands) before
   control returns to the original signal that advanced the state machine
   in GTK (and effectively blocked other GTK signals).  This is subtle:
   if inventory interface calls now instead of asap, then we will end
   up returning to an inventory state errantly during player turns.  */

   g->state    = state;
   sc_game_reinstate_asap(g);

}



inline void sc_game_set_state_allow_now(sc_config *c, sc_game *g, int state, unsigned long delay) {

   if(delay <= 0) sc_game_set_state_now(c, g, state);
   else sc_game_set_state(g, state, delay);

}



inline void sc_game_sync_timeout(sc_game *g) {

   sc_game_time(&g->timeout);

}



void sc_game_mass_kill(sc_config *c, sc_game *g) {

   int i;

   for(i = 0; i < c->numplayers; ++i) {
      /* Since we are about to deallocate everything anyway,
         we can just set the death flag directly here.  */
      c->players[i]->dead = true;
   }
   sc_game_set_state(g, SC_STATE_ROUND_END, 0);

}



void sc_game_init(sc_game *g) {

   g->substate = 0;
   g->curplayer = 0;
   sc_game_sync_timeout(g);
   sc_game_set_state(g, SC_STATE_PRELUDE_BEGIN, 0);
   sc_expl_free_chain(&g->expl_init);
   sc_expl_free_chain(&g->expl_draw);
   sc_expl_free_chain(&g->expl_clear_init);
   sc_expl_free_chain(&g->expl_clear);
   sc_expl_free_chain(&g->expl_done);

}



int sc_game_victor(sc_config *c) {

   int i;
   int j;

   i = c->numplayers - 1;
   j = 0;
   while(i >= 0) {
      if(SC_PLAYER_IS_ALIVE(c->players[i])) ++j;
      --i;
   }
   return(j <= 1);

}



void sc_game_set_victor(sc_config *c) {

   int i;

   i = c->numplayers - 1;
   while(i >= 0) {
      if(SC_PLAYER_IS_ALIVE(c->players[i])) sc_player_inc_wins(c, c->players[i]);
      --i;
   }

}



void sc_game_pause(sc_config *c, sc_game *g) {

   g->state = g->state | SC_STATE_PAUSE_FLAG;
   sc_status_suspend(c->window);

}



void sc_game_unpause(sc_config *c, sc_game *g) {

   g->state = g->state & ~SC_STATE_PAUSE_FLAG;
   sc_game_sync_timeout(g);
   sc_status_resume(c->window);

}



void sc_game_expl_queue_append(sc_explosion **queue, sc_explosion *e) {
/* sc_game_expl_queue_append
   Appends explosion chain in e to queue given.  */

   sc_expl_add(queue, e);

}



void sc_game_expl_queue_head_move(sc_explosion **dst, sc_explosion **src) {
/* sc_game_expl_queue_head_move
   Moves the HEAD of src only to the end of dst. */

   sc_explosion *e;

   if(src == NULL || dst == NULL) return;
   if(*src == NULL) return;

   e = *src;
   *src = e->chain;
   e->chain = NULL;
   sc_game_expl_queue_append(dst, e);

}



void sc_game_expl_queue_head_move_prepend(sc_explosion **dst, sc_explosion **src) {
/* sc_game_expl_queue_head_move_prepend
   Moves the HEAD of src only to the beginning of dst. */

   sc_explosion *e;

   if(src == NULL || dst == NULL) return;
   if(*src == NULL) return;

   e = *src;
   *src = e->chain;
   e->chain = *dst;
   *dst = e;

}



void sc_game_expl_queue_move(sc_explosion **dst, sc_explosion **src) {
/* sc_game_expl_queue_move
   Moves the entire queue in src to dst.  */

   if(src == NULL || dst == NULL) return;
   sc_game_expl_queue_append(dst, *src);
   *src = NULL;

}



void sc_game_expl_queue_reverse(sc_explosion **queue) {
/* sc_game_expl_queue_reverse
   Reverses the items stored in this queue.  */

   sc_explosion *new = NULL;
   sc_explosion *cur = NULL;

   if(queue == NULL) return;

   while(*queue != NULL) {
      cur = *queue;
      *queue = cur->chain;
      cur->chain = new;
      new = cur;
   }
   *queue = new;

}



void sc_game_expl_queue_head_free(sc_explosion **queue) {
/* sc_game_expl_queue_head_free
   Releases the top explosion on the queue...  */

   sc_expl_free(queue);

}



void sc_game_expl_queue_item_free(sc_explosion **queue, sc_explosion *e) {
/* sc_game_expl_queue_item_free
   Removes the explosion e from within the queue given.  */

   if(queue == NULL || e == NULL) return;

   while(*queue != NULL) {
      if(*queue == e) {
         /* Remove this item */
         sc_game_expl_queue_head_free(queue);
         return;
      }
      queue = &(*queue)->chain;
   }

}



void sc_game_expl_queue_item_move(sc_explosion **dst, sc_explosion **src, sc_explosion *e) {
/* sc_game_expl_queue_item_move
   Moves the item indicated in src to the end of the queue in dst.  */

   if(src == NULL || dst == NULL || e == NULL) return;

   while(*src != NULL) {
      if(*src == e) {
         /* Remove this item */
         *src = e->chain;
         e->chain = NULL;
         sc_game_expl_queue_head_move(dst, &e);
         return;
      }
      src = &(*src)->chain;
   }

}



void sc_game_expl_queue_item_move_prepend(sc_explosion **dst, sc_explosion **src, sc_explosion *e) {
/* sc_game_expl_queue_item_move_prepend
   Moves the item indicated in src to the beginning of the queue in dst.  */

   if(src == NULL || dst == NULL || e == NULL) return;

   while(*src != NULL) {
      if(*src == e) {
         /* Remove this item */
         *src = e->chain;
         e->chain = NULL;
         sc_game_expl_queue_head_move_prepend(dst, &e);
         return;
      }
      src = &(*src)->chain;
   }

}



bool sc_game_expl_queue_do_runnable(sc_config *c, sc_game *g, sc_explosion *queue, sc_game_run_fn action) {
/* sc_game_expl_queue_do_runnable
   For each item in the queue whose timestamp is at most the current game
   time, invoke the procedure given exactly once.  Action may remove e
   from the queue but must not take any action that would be destructive
   to the rest of the queue.  */

   sc_explosion *cur;
   bool acted = false;

   if(c == NULL || g == NULL || action == NULL) return(false);

   while(queue != NULL) {
      cur = queue;
      queue = cur->chain;
      if(cur->counter == 0) {
         /* Run this explosion through the action method */
         acted = action(c, g, cur) | acted;
      }
   }

   return(acted);

}



bool sc_game_expl_queue_run_first(sc_config *c, sc_game *g, sc_explosion *queue, sc_game_run_fn action) {
/* sc_game_expl_queue_run_first
   Find the first item in the queue whose timestamp is at most the current game
   time, invoking the procedure given exactly once on that item.  Action may
   remove e from the queue but must not take any action that would be destructive
   to the rest of the queue.  */

   sc_explosion *cur;

   if(c == NULL || g == NULL || action == NULL) return(false);

   while(queue != NULL) {
      cur = queue;
      queue = cur->chain;
      if(cur->counter == 0) {
         /* Run this explosion through the action method */
         return(action(c, g, cur));
      }
   }

   return(false);

}



void sc_game_expl_queue_postpone(sc_explosion *e, unsigned long delay) {
/* sc_game_expl_queue_postpone */

   e->counter = delay;

}



void sc_game_expl_queue_update(sc_explosion *e) {
/* sc_game_expl_queue_update */

   while(e != NULL) {
      if(e->counter > 0) --e->counter;
      e = e->chain;
   }

}



