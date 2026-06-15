/* $Header: /fridge/cvs/xscorch/sgame/sstate.c,v 1.24 2009-04-26 17:39:44 jacob Exp $ */
/*

   xscorch - sstate.c         Copyright(c) 2000-2004 Justin David Smith
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
#include <sstate.h>        /* Game state header */
#include <sconfig.h>       /* Config is dereferenced frequently */
#include <seconomy.h>      /* Need to do interest update */
#include <sgame.h>         /* Game state header */
#include <sland.h>         /* Need to drop the land after deton */
#include <sphysics.h>      /* Need to do Wind updates */
#include <splayer.h>       /* Dereferenced in death check, etc. */
#include <spreround.h>     /* For Auto Defense and The Lottery */
#include <strack.h>        /* Need for tracking, return codes */
#include <sweapon.h>       /* We construct weapon chains here */
#include <swindow.h>       /* We do much drawing from state machine */

#include <sai/sai.h>       /* We are responsible for running AI */
#include <snet/snet.h>     /* This is a comment about snet.h */
#include <ssound/ssound.h> /* Sound hooks are present, here  */
#include <sutil/srand.h>   /* Need a random number generator */



static void _sc_state_prelude_begin(sc_config *c, sc_game *g) {

   /* Start sound, if applicable */
   g->musicid = SC_MUSIC_PRELUDE;
   sc_sound_start(c->sound, g->musicid);

   /* Setup screen/status */
   sc_status_message(c->window, "Welcome to XScorch " VERSION "!");

   /* Activate the main menu and wait */
   sc_window_main_menu(c->window);
   sc_game_set_state(g, SC_STATE_PRELUDE_IDLE, SC_DELAY_PROMPT);

}



static void _sc_state_game_end(sc_config *c, sc_game *g) {

   /* Start end-game sound */
   g->musicid = SC_MUSIC_ENDGAME;
   sc_sound_start(c->sound, g->musicid);

   /* Display end-game scores */
   sc_window_paint_end_game(c->window);
   sc_game_set_state(g, SC_STATE_GAME_END_IDLE, SC_DELAY_PROMPT);

}



static void _sc_state_inventory_begin(sc_config *c, sc_game *g) {

   /* Enable inventory music */
   g->musicid = SC_MUSIC_INVENTORY;
   sc_sound_start(c->sound, g->musicid);

   /* Starting with player 0... */
   g->curplayer = 0;
   #if USE_NETWORK
      if(c->client != NULL) sc_status_message(c->window, "Waiting for other players to begin round ...");
      sc_net_client_sync(c->client, SC_CONN_SYNC_INV, c->server != NULL);
   #endif /* Network? */
   sc_game_set_state_now(c, g, SC_STATE_INVENTORY_PL_BEGIN);

}



static void _sc_state_inventory_pl_begin(sc_config *c, sc_game *g) {

   sc_player *p = c->players[g->curplayer];

   if(sc_ai_player_buy(c, p) == SC_AI_CONTINUE) {
      sc_status_message(c->window, "");
      sc_game_set_state_now(c, g, SC_STATE_INVENTORY_PL_DONE);
   } else {
      sc_status_player_message(c->window, p, "Inventory");
      sc_window_inventory(c->window, p);
      sc_game_set_state(g, SC_STATE_INVENTORY_PL_IDLE, SC_DELAY_PROMPT);
   }

}



static void _sc_state_inventory_pl_done(sc_config *c, sc_game *g) {

   #if USE_NETWORK
      sc_net_client_send_inventory(c, c->client, g->curplayer);
   #endif /* Networking? */

   ++g->curplayer;
   if(g->curplayer >= c->numplayers) {
      g->curplayer = 0;
      sc_game_set_state_now(c, g, SC_STATE_ROUND_BEGIN);
   } else {
      sc_game_set_state_now(c, g, SC_STATE_INVENTORY_PL_BEGIN);
   }

}



static void _sc_state_round_begin(sc_config *c, sc_game *g) {

   g->musicid = SC_MUSIC_ROUND;
   sc_sound_start(c->sound, g->musicid);
   sc_config_init_round(c);
   sc_window_paint(c->window, 0, 0, c->fieldwidth - 1, c->fieldheight - 1, SC_REGENERATE_LAND | SC_PAINT_EVERYTHING);
   #if USE_NETWORK
      if(c->client != NULL) sc_status_message(c->window, "Waiting for other players to close their inventory ...");
      sc_net_client_sync(c->client, SC_CONN_SYNC_ROUND, c->server != NULL);
   #endif /* Network? */
   sc_game_set_state_asap(g, SC_STATE_AUTO_DEFENSE_LOOP);

}



static void _sc_state_auto_defense(sc_config *c, sc_game *g) {
/* _sc_state_auto_defense
   Select the auto defense state, and run it.
   The window code will advance the state for us when it's done. */

   if(g->curplayer >= c->numplayers) {
      /* Send the state machine on to display the next state. */
      g->curplayer = 0;
      sc_game_set_state_now(c, g, SC_STATE_LOTTERY_DISPLAY);
   } else if(c->players[g->curplayer]->aitype != SC_AI_NETWORK) {
      /* Run an auto defense session (maybe display) for the player. */
      if(sc_preround_auto_defense(c, c->players[g->curplayer])) {
         ++g->curplayer;
         sc_game_set_state(g, SC_STATE_AUTO_DEFENSE, SC_DELAY_PROMPT);
      } else {
         ++g->curplayer;
         sc_game_set_state_now(c, g, SC_STATE_AUTO_DEFENSE_LOOP);
      }
   } else {
      /* Try the next player. */
      ++g->curplayer;
      sc_game_set_state_now(c, g, SC_STATE_AUTO_DEFENSE_LOOP);
   }

}



static void _sc_state_lottery_display(sc_config *c, sc_game *g) {
/* _sc_state_lottery_display
   Set the state for displaying The Lottery result. */

   if(c->lottery != NULL && c->lottery->winner != NULL) {
      /* Display lottery winnings, and then wait for the user. */
      if(sc_preround_lottery(c)) {
         sc_game_set_state(g, SC_STATE_LOTTERY_DISPLAY_WAIT, SC_DELAY_PROMPT);
      } else {
         sc_game_set_state_now(c, g, SC_STATE_TURN_BEGIN);
      }
   } else {
      /* Nothing to see here; advance! */
      sc_game_set_state_now(c, g, SC_STATE_TURN_BEGIN);
   }

}



static void _sc_state_round_end(sc_config *c, sc_game *g) {

   sc_game_set_victor(c);
   sc_economy_interest(c, c->economics);

   if(c->curround + 1 >= c->numrounds) {
      sc_game_set_state_now(c, g, SC_STATE_GAME_END);
   } else {
      g->musicid = SC_MUSIC_ENDROUND;
      sc_sound_start(c->sound, g->musicid);
      sc_window_paint_end_round(c->window);
      sc_game_set_state(g, SC_STATE_ROUND_END_IDLE, SC_DELAY_PROMPT);
   }

}



static void _sc_state_turn_begin(sc_config *c, sc_game *g) {

   sc_game_sync_timeout(g);
   sc_config_init_turn(c);
   sc_status_message(c->window, "");
   if(sc_game_victor(c)) {
      sc_game_set_state_now(c, g, SC_STATE_ROUND_END);
   } else {
      g->curplayer = 0;
      sc_game_set_state_now(c, g, SC_STATE_TURN_PL_BEGIN);
   }

}



static void _sc_state_turn_pl_begin(sc_config *c, sc_game *g) {

   sc_player *p = c->plorder[g->curplayer];

   if(p->dead || sc_ai_player_turn(c, p) == SC_AI_CONTINUE) {
      sc_status_message(c->window, "");
      sc_game_set_state(g, SC_STATE_TURN_PL_DONE, SC_DELAY_SHORT);
   } else {
      sc_status_update(c->window, p);
      sc_game_set_state(g, SC_STATE_TURN_PL_IDLE, SC_DELAY_PROMPT);
   }

}



static void _sc_state_turn_pl_done(sc_config *c, sc_game *g) {

   sc_player *p = c->plorder[g->curplayer];

   /* Arm the player's weapon */
   if(!p->dead) {
      p->armed = true;
      sc_window_redraw_tank(c->window, p);
   }

   sc_status_message(c->window, "");

   switch(c->options.mode) {
      case SC_CONFIG_MODE_SEQUENTIAL:
         #if USE_NETWORK
            sc_net_client_send_player_state(c, c->client);
            if(c->client != NULL) sc_status_message(c->window, "Waiting for other players to turn ...");
            sc_net_client_sync(c->client, SC_CONN_SYNC_TURN, c->server != NULL);
         #endif /* Network? */
         sc_game_set_state_asap(g, SC_STATE_RUN_TALK);
         break;
      case SC_CONFIG_MODE_SYNCHRONOUS:
         sc_game_set_state_now(c, g, SC_STATE_TURN_PL_NEXT);
         break;
   }

}



static void _sc_state_turn_pl_next(sc_config *c, sc_game *g) {

   ++g->curplayer;
   if(g->curplayer >= c->numplayers) switch(c->options.mode) {
      case SC_CONFIG_MODE_SEQUENTIAL:
         sc_game_set_state_now(c, g, SC_STATE_TURN_END);
         break;
      case SC_CONFIG_MODE_SYNCHRONOUS:
         g->curplayer = 0;
         #if USE_NETWORK
            sc_net_client_send_player_state(c, c->client);
            if(c->client != NULL) sc_status_message(c->window, "Waiting for other players to turn ...");
            sc_net_client_sync(c->client, SC_CONN_SYNC_TURN, c->server != NULL);
         #endif /* Network? */
         sc_game_set_state_asap(g, SC_STATE_RUN_TALK);
         break;
   } else {
      sc_game_set_state_now(c, g, SC_STATE_TURN_PL_BEGIN);
   }

}



static void _sc_state_run_talk(sc_config *c, sc_game *g) {

   const char *msg;
   sc_player *p = c->plorder[g->curplayer];
   int nextstate = 0;

   sc_status_message(c->window, "");

   switch(c->options.mode) {
      case SC_CONFIG_MODE_SEQUENTIAL:
         nextstate = SC_STATE_RUN_CREATION;
         break;
      case SC_CONFIG_MODE_SYNCHRONOUS:
         if(g->curplayer + 1 >= c->numplayers) {
            nextstate = SC_STATE_RUN_CREATION;
         } else {
            nextstate = SC_STATE_RUN_TALK;
         }
         break;
   }

   if(nextstate == SC_STATE_RUN_TALK) ++g->curplayer;

   if(!p->dead && p->armed) {
      msg = sc_player_talk(c, p);
      if(!SC_CONFIG_GFX_FAST(c) && msg != NULL) {
         sc_status_player_message(c->window, p, msg);
         sc_game_set_state(g, nextstate, SC_DELAY_TALK);
         return;
      }
   }

   sc_game_set_state_now(c, g, nextstate);

}



static void _sc_state_run_creation(sc_config *c, sc_game *g) {

   g->activity = false;
   sc_status_message(c->window, "");
   sc_game_sync_timeout(g);

   /* If sc_weapon_create_all returns true, we have explosions to run. */
   if(sc_weapon_create_all(c, &(g->expl_init))) {
      sc_game_set_state_now(c, g, SC_STATE_RUN_EXPLOSION);
   } else {
      sc_game_set_state_now(c, g, SC_STATE_RUN_WEAPONS);
   }

}



static void _sc_state_run_weapons(sc_config *c, sc_game *g) {

   switch(sc_weapon_track_all(c, &g->expl_init)) {
      case SC_WEAPON_TRACK_NO_ACTION:
         /* Weapons have all terminated */
         if(c->options.interleave && g->activity) {
            g->activity = false;
            sc_game_set_state_now(c, g, SC_STATE_RUN_EXPLOSION);
         } else {
            sc_game_set_state_now(c, g, SC_STATE_RUN_END);
         }
         break;

      case SC_WEAPON_TRACK_NEED_RECURSE:
         /* Weapons are still flying, but no detonations yet */
         if(SC_STATE_ANY_EXPL(g)) {
            sc_game_set_state_now(c, g, SC_STATE_RUN_EXPLOSION);
         } else {
            sc_game_reinstate(g, SC_STATE_FAST(c, SC_STATE_MACHINE_INTERVAL));
         }
         break;

      case SC_WEAPON_TRACK_DETONATE:
         /* Weapons may still be flying; a detonation occurred */
         /* Move all explosions in init to the draw queue */
         sc_game_set_state_now(c, g, SC_STATE_RUN_EXPLOSION);
         break;

      default:
         /* Oops */;
   }

}



static bool _sc_state_run_draw_expl(sc_config *c, sc_game *g, sc_explosion *e) {

   if(!sc_expl_annihilate(c, e)) {
      /* This explosion terminated quickly; proceed */
      sc_game_expl_queue_item_move_prepend(&g->expl_clear_init, &g->expl_init, e);
      sc_game_expl_queue_postpone(e, SC_STATE_FAST(c, SC_STATE_EXPLOSION_STEPS));
   } else {
      /* This explosion is still executing */
      sc_game_expl_queue_item_move(&g->expl_draw, &g->expl_init, e);
      sc_game_expl_queue_postpone(e, SC_STATE_FAST(c, SC_STATE_NEXTITER_STEPS));
   }
   return(true);

}



static bool _sc_state_run_draw_expl_cont(sc_config *c, sc_game *g, sc_explosion *e) {

   if(!sc_expl_annihilate_continue(c, e)) {
      /* This explosion has finally terminated */
      sc_game_expl_queue_item_move_prepend(&g->expl_clear_init, &g->expl_draw, e);
      sc_game_expl_queue_postpone(e, SC_STATE_FAST(c, SC_STATE_EXPLOSION_STEPS));
   } else {
      /* This explosion is still executing */
      sc_game_expl_queue_postpone(e, SC_STATE_FAST(c, SC_STATE_NEXTITER_STEPS));
   }
   return(true);

}



static bool _sc_state_run_clear_expl(sc_config *c, sc_game *g, sc_explosion *e) {

   /* Warning: damage routine must be called _before_ annihilate
      clear is called, as damage_all might need information that is
      deallocated on annihilate_clear().  */
   sc_player_damage_all(c, e);

   if(!sc_expl_annihilate_clear(c, e)) {
      /* This explosion was cleared quickly */
      sc_game_expl_queue_item_move(&g->expl_done, &g->expl_clear_init, e);
   } else {
      /* This explosion needs more time to clear */
      sc_game_expl_queue_item_move(&g->expl_clear, &g->expl_clear_init, e);
   }
   sc_game_expl_queue_postpone(e, SC_STATE_FAST(c, SC_STATE_NEXTITER_STEPS));
   return(true);

}



static bool _sc_state_run_clear_expl_cont(sc_config *c, sc_game *g, sc_explosion *e) {

   if(!sc_expl_annihilate_clear_continue(c, e)) {
      /* This explosion has finally cleared */
      sc_game_expl_queue_item_move(&g->expl_done, &g->expl_clear, e);
   }
   sc_game_expl_queue_postpone(e, SC_STATE_FAST(c, SC_STATE_NEXTITER_STEPS));
   return(true);

}



static bool _sc_state_run_land_fall(sc_config *c, sc_game *g, sc_explosion *e) {

   if(sc_land_drop(c, c->land, e->centerx, e->radius)) {
      sc_game_expl_queue_postpone(e, SC_STATE_FAST(c, SC_STATE_NEXTITER_STEPS));
      return(true);
   } else {
      sc_game_expl_queue_item_free(&g->expl_done, e);
      return(false);
   }

}



static bool _sc_state_run_explosion_simult(sc_config *c, sc_game *g) {

   bool result;

   /* Process ``init'' queue */
   result = sc_game_expl_queue_do_runnable(c, g, g->expl_init, _sc_state_run_draw_expl);

   /* Process ``draw'' queue */
   result = sc_game_expl_queue_do_runnable(c, g, g->expl_draw, _sc_state_run_draw_expl_cont) | result;

   /* Process ``clear_init'' queue */
   result = sc_game_expl_queue_do_runnable(c, g, g->expl_clear_init, _sc_state_run_clear_expl) | result;

   /* Process ``clear'' queue */
   result = sc_game_expl_queue_do_runnable(c, g, g->expl_clear, _sc_state_run_clear_expl_cont) | result;

   /* Process ``done'' queue */
   result = sc_game_expl_queue_do_runnable(c, g, g->expl_done, _sc_state_run_land_fall) | result;

   /* Were any actions taken? */
   return(result);

}



static bool _sc_state_run_explosion_nosimult(sc_config *c, sc_game *g) {

   /* Check ``done'' queue */
   if(g->expl_done != NULL) return(sc_game_expl_queue_run_first(c, g, g->expl_done, _sc_state_run_land_fall));

   /* Check ``clear'' queue */
   if(g->expl_clear != NULL) return(sc_game_expl_queue_run_first(c, g, g->expl_clear, _sc_state_run_clear_expl_cont));

   /* Check ``clear_init'' queue */
   if(g->expl_clear_init != NULL) return(sc_game_expl_queue_run_first(c, g, g->expl_clear_init, _sc_state_run_clear_expl));

   /* Check ``draw'' queue */
   if(g->expl_draw != NULL) return(sc_game_expl_queue_run_first(c, g, g->expl_draw, _sc_state_run_draw_expl_cont));

   /* Check ``init'' queue */
   if(g->expl_init != NULL) return(sc_game_expl_queue_run_first(c, g, g->expl_init, _sc_state_run_draw_expl));

   /* No actions taken */
   return(false);

}



static void _sc_state_run_explosion(sc_config *c, sc_game *g) {

   bool result;

   #if SC_STATE_DEBUG
   printf("\nrun_explosion:  entering function\n");
   #endif /* SC_STATE_DEBUG */

   do {
      #if SC_STATE_DEBUG
      printf("run_explosion:  queues init %d  draw %d  clear_init %d  clear %d  done %d\n",
             sc_expl_count(g->expl_init),
             sc_expl_count(g->expl_draw),
             sc_expl_count(g->expl_clear_init),
             sc_expl_count(g->expl_clear),
             sc_expl_count(g->expl_done));
      #endif /* SC_STATE_DEBUG */
      if(c->options.interleave) {
         #if SC_STATE_DEBUG
         printf("run_explosion:  doing simultaneous\n");
         #endif /* SC_STATE_DEBUG */
         result = _sc_state_run_explosion_simult(c, g);
      } else {
         #if SC_STATE_DEBUG
         printf("run_explosion:  doing non-simultaneous\n");
         #endif /* SC_STATE_DEBUG */
         result = _sc_state_run_explosion_nosimult(c, g);
      }
   } while(result);

   #if SC_STATE_DEBUG
   printf("run_explosion:  done\n");
   #endif /* SC_STATE_DEBUG */

   /* Update the counters */
   sc_game_expl_queue_update(g->expl_init);
   sc_game_expl_queue_update(g->expl_draw);
   sc_game_expl_queue_update(g->expl_clear_init);
   sc_game_expl_queue_update(g->expl_clear);
   sc_game_expl_queue_update(g->expl_done);

   /* Which state do we go to next? */
   sc_game_sync_timeout(g);
   g->activity = g->activity || SC_STATE_ANY_EXPL(g);
   if(c->options.interleave) {
      sc_game_set_state_now(c, g, SC_STATE_RUN_PLAYER_DROP);
   } else {
      if(SC_STATE_ANY_EXPL(g)) {
         /* Still processing explosions... */
         sc_game_set_state_allow_now(c, g, SC_STATE_RUN_EXPLOSION, SC_STATE_FAST(c, SC_STATE_MACHINE_INTERVAL));
      } else {
         /* No more explosions to process, we may continue. */
         sc_game_set_state_now(c, g, SC_STATE_RUN_PLAYER_DROP);
      } /* Any explosions left? */
   } /* Interleaving? */

}



static void _sc_state_run_player_drop(sc_config *c, sc_game *g) {

   if(sc_player_drop_all(c)) {
      g->activity = true;
      if(c->options.interleave) {
         sc_game_set_state(g, SC_STATE_RUN_WEAPONS, SC_STATE_FAST(c, SC_STATE_MACHINE_INTERVAL));
      } else {
         sc_game_reinstate_allow_now(c, g, SC_STATE_FAST(c, SC_STATE_MACHINE_INTERVAL));
      }
   } else {
      g->substate = 0;
      sc_game_set_state_now(c, g, SC_STATE_RUN_PLAYER_DEATH);
   }

}



static void _sc_state_run_player_death(sc_config *c, sc_game *g) {

   sc_player *p;
   const char *msg;

   sc_status_message(c->window, "");
   if(g->substate >= c->numplayers) {
      sc_game_set_state(g, SC_STATE_RUN_WEAPONS, SC_STATE_FAST(c, SC_STATE_MACHINE_INTERVAL));
   } else {
      p = c->players[g->substate];
      if(!p->dead && p->life <= 0) {
         /* This player just died */
         g->activity = true;
         msg = sc_player_death_talk(c, p);
         if(msg != NULL) {
            sc_status_player_message(c->window, p, msg);
         } else {
            sc_status_player_message(c->window, p, "");
         } /* Should player give farewell message? */
         sc_player_death(c, p, &g->expl_init);
         sc_player_died(c, p);
         sc_window_undraw_tank(c->window, p);
         if(c->options.interleave) {
            ++g->substate;
            sc_game_reinstate_now(c, g);
         } else {
            sc_game_set_state_now(c, g, SC_STATE_RUN_EXPLOSION);
            ++g->substate;
         }
      } else {
         ++g->substate;
         sc_game_reinstate_now(c, g);
      }
   }

}



static void _sc_state_run_end(sc_config *c, sc_game *g) {

   switch(c->options.mode) {
      case SC_CONFIG_MODE_SEQUENTIAL:
         sc_game_set_state_now(c, g, SC_STATE_TURN_PL_NEXT);
         break;
      case SC_CONFIG_MODE_SYNCHRONOUS:
         sc_game_set_state_now(c, g, SC_STATE_TURN_END);
         break;
   }

}



void sc_state_run(sc_config *c, sc_game *g) {

   struct timeval curtime;

   /* Check network activity */
   #if USE_NETWORK
      sc_net_client_run(c, c->client);
      sc_net_server_run(c, c->server);
      if(sc_net_client_death(&c->client)) {
         sc_window_message(c->window, "Client Disconnected", "Client was disconnected from the network");
         sc_window_update(c->window);
      }
      if(c->client != NULL && !SC_CONN_IS_OKAY(c->client->server)) {
         /* We deferred this cycle */
         return;
      }
   #endif /* Network activity? */

   sc_game_time(&curtime);
   if(curtime.tv_sec > g->timeout.tv_sec ||
     (curtime.tv_sec == g->timeout.tv_sec && curtime.tv_usec >= g->timeout.tv_usec)) {

      #if SC_STATE_TIMER_DEBUG
         printf("...  state %8x accepted %ld.%06ld\n", g->state, curtime.tv_sec, curtime.tv_usec);
      #endif /* SC_STATE_TIMER_DEBUG */

      #if SC_STATE_DEBUG
         printf("State %8x    ", g->state);
         SC_PROFILE_BEGIN("game_run")
      #endif /* SC_STATE_DEBUG */

      switch(g->state) {



         /***  PRELUDE  ***/


         /* PRELUDE_BEGIN
               -> PRELUDE_IDLE
            Initialise for a new game; display main menu.
         */
         case SC_STATE_PRELUDE_BEGIN:
         case SC_STATE_GAME_END_DONE:
            _sc_state_prelude_begin(c, g);
            break;



         /***  GAME  ***/


         /* GAME_BEGIN
               -> INVENTORY_BEGIN   (immediate)
            When user first presses Enter on the main menu.  Initialize
            configuration: Initialize config, player game data, and just
            about everything else relevant for game state.
         */
         case SC_STATE_GAME_BEGIN:
            /* Initialize configuration for a new GAME */
            sc_config_init_game(c);
            sc_status_message(c->window, "Starting a new game ...");
            #if USE_NETWORK
               if(c->client != NULL) sc_status_message(c->window, "Waiting for other players to begin game ...");
               sc_net_client_sync(c->client, SC_CONN_SYNC_GAME, c->server != NULL);
            #endif /* Network? */
            sc_game_set_state_asap(g, SC_STATE_INVENTORY_BEGIN);
            break;


         /* GAME_END
               -> GAME_END_IDLE
            All rounds have been played, display the final winners.  Pause
            for user input.
         */
         case SC_STATE_GAME_END:
            _sc_state_game_end(c, g);
            break;



         /***  INVENTORY   ***/


         /* INVENTORY_BEGIN
               -> INVENTORY_PL_BEGIN   (immediate)
            Begin inventory; prepare to take inventory for one player.
         */
         case SC_STATE_INVENTORY_BEGIN:
         case SC_STATE_ROUND_END_DONE:
            _sc_state_inventory_begin(c, g);
            break;


         /* INVENTORY_PL_BEGIN
             Human:
               -> INVENTORY_PL_IDLE
             AI:
               -> INVENTORY_PL_DONE
            If this state is entered, then curplayer must be a valid player.
            Purchases for the player indicated.  Loops back to self until all
            players have bought items.  If this is an AI, purchases are made
            now.
         */
         case SC_STATE_INVENTORY_PL_BEGIN:
            _sc_state_inventory_pl_begin(c, g);
            break;


         /* INVENTORY_PL_DONE
               -> INVENTORY_PL_BEGIN   (loop)
               t> AUTO_DEFENSE_LOOP    (terminating condition)
         */
         case SC_STATE_INVENTORY_PL_DONE:
            _sc_state_inventory_pl_done(c, g);
            break;


         /***  PRE-ROUND   ***/


         /* AUTO DEFENSE LOOP */
         case SC_STATE_AUTO_DEFENSE_LOOP:
            _sc_state_auto_defense(c, g);
            break;


         /* LOTTERY_DISPLAY */
         case SC_STATE_LOTTERY_DISPLAY:
            _sc_state_lottery_display(c, g);
            break;


         /***  ROUND    ***/


         /* ROUND_BEGIN
               -> TURN_BEGIN     (immediate)
            Begin a new round:  Inventory has been taken; generate the land,
            place tanks as appropriate, and prepare for Scorch.
         */
         case SC_STATE_ROUND_BEGIN:
            _sc_state_round_begin(c, g);
            break;


         /* ROUND_END
               -> ROUND_END_IDLE (end of round, will loop)
               t> GAME_END       (end of all rounds)
            Round has ended; advance round, and go to inventory or final
            winnings (latter occurs only if all rounds have been played).
            In former, we identify the victor, do economics and final
            winning calculations, and then notify the user who won.
         */
         case SC_STATE_ROUND_END:
            _sc_state_round_end(c, g);
            break;



         /***  TURNS    ***/


         /* TURN_BEGIN
               -> TURN_PL_BEGIN  (normal progression)
               t> ROUND_END      (no one/1 person alive)
            Player turn begin: Setup for each player to take a turn.  Make
            sure to check if the game has already ended `tho (is there a
            victor?)  Make sure times are in sync, and init the turn.
         */
         case SC_STATE_TURN_BEGIN:
         case SC_STATE_TURN_END:
            _sc_state_turn_begin(c, g);
            break;


         /* TURN_PL_BEGIN
             Dead:
               -> TURN_PL_DONE   (cannot play)
             Human:
               -> TURN_PL_IDLE   (take orders)
             AI:
               -> TURN_PL_DONE   (AI actions taken)
            If we get here, we must have a valid curplayer value (it may be
            a dead player, however).  If dead, then this cycle is basically
            skipped; otherwise, humans are given the opportunity to enter
            orders, and AI actions are taken now.
         */
         case SC_STATE_TURN_PL_BEGIN:
            _sc_state_turn_pl_begin(c, g);
            break;


         /* TURN_PL_DONE

            ==Sequential Mode==
               -> RUN_TALK
            This player has given orders; execute those orders if the player
            is living.  If the player is dead, RUN_TALK will fall through
            and proceed to the next player.  The player's weapon is armed
            here.

            ==Synchronous Mode==
               -> TURN_PL_NEXT
            This player has given orders; take orders from the next player.
            If no more players, the next state will fall to RUN_TALK loop.
            The player's weapon is armed here.
         */
         case SC_STATE_TURN_PL_DONE:
            _sc_state_turn_pl_done(c, g);
            break;


         /* TURN_PL_NEXT

            ==Sequential Mode==
               -> TURN_PL_BEGIN     (immediate)
               t> TURN_END          (terminal, immediate)

            ==Synchronous Mode==
               -> TURN_PL_BEGIN     (immediate)
               t> RUN_TALK          (terminal, immediate)
         */
         case SC_STATE_TURN_PL_NEXT:
            _sc_state_turn_pl_next(c, g);
            break;



         /***  RUN   ***/


         /* RUN_TALK

            ==Sequential Mode==
               -> RUN_CREATION      (might be 2s delay)
            This state DOES NOT LOOP.  If this player is not dead and armed,
            then we might allow them to speak (2s delay) or might launch
            their weapon now (immediate).

            ==Synchronous Mode==
               -> RUN_TALK          (might delay, loops)
               t> RUN_CREATION      (out of players)
            This state loops through all players.  If the player is not dead
            and is armed, they may speak at this time (2s delay) or might
            not (loop is immediate).  Once complete, this will yield to the
            weapon creation state.
         */
         case SC_STATE_RUN_TALK:
            _sc_state_run_talk(c, g);
            break;


         /* RUN_CREATION
               -> RUN_WEAPONS          (immediate; no explosions yet)
               -> RUN_EXPLOSION        (immediate; explosions waiting)
            Create all player weapons.  If the weapon creation created
            an explosion immediately, then process the explosion before
            sending any weapons into flight.
         */
         case SC_STATE_RUN_CREATION:
            _sc_state_run_creation(c, g);
            break;


         /* RUN_WEAPONS (TEMP OBS DOC)
               -> RUN_WEAPONS          (weapons still flying)
               -> RUN_EXPLOSION        (might be flying; detonation)
               t> RUN_END              (terminal case; no weapons)
            Simulate the weapons in-flight.
         */
         case SC_STATE_RUN_WEAPONS:
            _sc_state_run_weapons(c, g);
            break;


         /* RUN_EXPLOSION (TEMP OBS DOC)
               -> RUN_EXPLOSION        (this explosion chain not done; wait)
               t> RUN_PLAYER_DROP      (immediate)
            Run an explosion in progress.  The weapon explosion might opt to
            end at this point, or it may decide to continue its trend of
            annihilation for a few more cycles.  This loop terminates when
            there are no explosions remaining to be drawn; then we will
            start clearing explosions after a respectable pause.
         */
         case SC_STATE_RUN_EXPLOSION:
            _sc_state_run_explosion(c, g);
            break;


         /* RUN_PLAYER_DROP (TEMP OBS DOC)
               -> RUN_PLAYER_DROP   (loop)
               t> RUN_PLAYER_DEATH  (terminal)
         */
         case SC_STATE_RUN_PLAYER_DROP:
            _sc_state_run_player_drop(c, g);
            break;


         /* RUN_PLAYER_DEATH (TEMP OBS DOC)
               -> RUN_PLAYER_DEATH
               t> RUN_END
            Check if any players just died.
         */
         case SC_STATE_RUN_PLAYER_DEATH:
            _sc_state_run_player_death(c, g);
            break;


         /* RUN_END

            ==Sequential Mode==
               -> TURN_PL_NEXT      (immediate)

            ==Synchronous Mode==
               -> TURN_END          (immediate)
         */
         case SC_STATE_RUN_END:
            _sc_state_run_end(c, g);
            break;


         /*    End of (known) game states    */


         default:
            sc_game_reinstate(g, SC_DELAY_LONG);
            break;

      }

      #if SC_STATE_DEBUG
         SC_PROFILE_END
      #endif /* SC_STATE_DEBUG */

   }

}
