/* $Header: /fridge/cvs/xscorch/sgame/spreround.c,v 1.7 2011-08-01 00:01:41 jacob Exp $ */
/*

   xscorch - spreround.c      Copyright(c) 2001 Jacob Luna Lundberg
   jacob(at)gnifty.net        http://www.gnifty.net/

   Scorched pre round setup called from state machine


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
#include <spreround.h>

#include <sconfig.h>
#include <sgame.h>
#include <sinventory.h>
#include <splayer.h>
#include <sshield.h>
#include <sstate.h>
#include <sweapon.h>
#include <swindow.h>

#include <sai/sai.h>
#include <sutil/srand.h>



bool sc_autodef_activate(const sc_config *c, sc_player *p, const sc_auto_def_set *ads) {
/* sc_player_activate_auto_def
   Activates player auto-defense, if available and if any shields are currently
   available.  This is for the interactive GUI screen (see ...auto_shield). */

   /* Sanity checks. */
   if(c == NULL || p == NULL || ads == NULL) return(false);

   /* Check for auto defense capability. */
   if(!(p->ac_state & SC_ACCESSORY_STATE_AUTO_DEF)) return(false);

   /*
    * Perform the requested auto defense actions...
    */

   /* Select the given tracking accessory. */
   /* TEMP - unimplemented - ads->auto_guidance - JL */

   /* Select and activate the given shield. */
   sc_player_set_shield(c, p, ads->auto_shield);
   sc_player_activate_shield(c, p);

   /* Activate parachutes, with the given pixel threshold. */
   /* TEMP - unimplemented - ads->chute_height - JL */

   /* Turn on or off Contact Triggers, as requested. */
   sc_player_set_contact_triggers(c, p, ads->triggers);

   /* Success! */
   return(true);

}



bool sc_autodef_ai_activate(const sc_config *c, sc_player *p) {
/* sc_autodef_ai_activate
   Activate an AI's Auto Defense system.
   Do NOT call game_rand from this function!  You have been warned! */

   sc_auto_def_set ads;

   /* Sanity checks. */
   if(c == NULL || p == NULL) return(false);

   /* Check for auto defense capability. */
   if(!(p->ac_state & SC_ACCESSORY_STATE_AUTO_DEF)) return(false);

   switch(p->aitype) {
      case SC_AI_ANNIHILATER:
      case SC_AI_CALCULATER:
      case SC_AI_CHOOSER:
      case SC_AI_INSANITY:
      case SC_AI_MORON:
      case SC_AI_SHOOTER:
      case SC_AI_SPREADER:
         /*
          * TEMP - current issues - JL
          * (1) All AIs use the same settings currently.
          * (2) Should this perhaps be in sai or splayer instead?
          * (3) The tracking stuff is unimplemented currently.
          * (4) Same with parachutes, and is 8 pixels good?
          */
         ads.auto_guidance = NULL;
         ads.auto_shield   = sc_shield_find_best(c, p);
         ads.chute_height  = 8;
         ads.triggers      = true;
         return(sc_autodef_activate(c, p, &ads));
      case SC_AI_HUMAN:
      case SC_AI_NETWORK:
      default:
         /* Non local AI players do not use this function. */
         return(false);
   }

}



sc_lottery *sc_lottery_new(void) {
/* sc_lottery_new
   Create a new lottery */

   sc_lottery *lottery = (sc_lottery *)malloc(sizeof(sc_lottery));
   if(lottery == NULL) return(NULL);

   lottery->displayed = false;
   lottery->winner    = NULL;
   lottery->stake     = NULL;

   return(lottery);

}



void sc_lottery_free(sc_lottery **lottery) {
/* sc_lottery_free
   Obliterate a lottery */

   if(lottery == NULL) return;

   free(*lottery);
   *lottery = NULL;

}



void sc_lottery_run(sc_config *c) {
/* sc_lottery_run
   Give a random player a random weapon. */

   sc_weapon_info *info;
   int player = game_lrand(c->numplayers);
   int weapon = game_lrand(sc_weapon_count(c->weapons, SC_WEAPON_LIMIT_ALL));

   if(c == NULL) return;

   /* Get the first weapon. */
   info = sc_weapon_first(c->weapons, SC_WEAPON_LIMIT_ALL);

   /* Count up to the weapon specified. */
   while(info != NULL && weapon-- >= 0)
      info = sc_weapon_next(c->weapons, info, SC_WEAPON_LIMIT_ALL);

   /* Find the first non-useless, non-infinite weapon. */
   weapon = sc_weapon_count(c->weapons, SC_WEAPON_LIMIT_ALL);
   while(info != NULL && (SC_WEAPON_IS_USELESS(info) || SC_WEAPON_IS_INFINITE(info)) && --weapon > 0)
      info = sc_weapon_next(c->weapons, info, SC_WEAPON_LIMIT_ALL);

   /* Tell the user and bail if we just can't do it. */
   if(info == NULL || weapon == 0) {
      printf("lottery: unable to award a weapon because all are useless\n");
      return;
   }

   /* Give the player his award, if we can. */
   c->lottery->displayed = false;
   if(sc_inventory_award_weapon(info, player)) {
      /* Record the award for posterity. */
      c->lottery->winner = c->players[player];
      c->lottery->stake  = info;
   } else {
      /* No award to announce. */
      c->lottery->winner = NULL;
      c->lottery->stake  = NULL;
   }

   return;

}



bool sc_preround_auto_defense(struct _sc_config *c, struct _sc_player *p) {
/* sc_preround_auto_defense
   Run the auto defense screen in pre-round mode.
   Will return true if a wait is expected. */

   /* Sanity checks. */
   if(c == NULL || p == NULL) return(false);

   /* Check for auto defense capability. */
   if(!(p->ac_state & SC_ACCESSORY_STATE_AUTO_DEF)) return(false);

   switch(p->aitype) {
      case SC_AI_NETWORK:
         /* We do not perform any action for network controlled players. */
         return(false);
      case SC_AI_HUMAN:
         /* Humans get a nice screen they can set stuff on. */
         sc_window_auto_defense(c->window, p);
         return(true);
      default:
         /* Local AIs set their auto defense parameters in realtime. */
         sc_autodef_ai_activate(c, p);
         return(false);
   }

}



bool sc_preround_lottery(struct _sc_config *c) {
/* sc_preround_lottery
   Run the pre-round display of lottery results.
   Will return true if a wait is expected. */

   bool showstake = false;
   int playerid;

   /* Sanity checks. */
   if(c == NULL || c->lottery == NULL || c->lottery->winner == NULL) return(false);

   /* Decide whether or not to show the stake (local human player). */
   for(playerid = 0; playerid < c->numplayers; ++playerid)
      if(c->players[playerid]->aitype == SC_AI_HUMAN && c->lottery->winner->index == playerid) {
         showstake = true;
         break;
      }

   /* Paste up the window. */
   sc_window_lottery_result(c->window, showstake);
   return(true);

}
