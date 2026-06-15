/* $Header: /fridge/cvs/xscorch/sai/saiturn.c,v 1.33 2011-08-01 00:01:40 jacob Exp $ */
/*

   xscorch - saiturn.c        Copyright(c) 2000-2003 Justin David Smith
                              Copyright(c) 2002-2003 Jacob Luna Lundberg
   justins(at)chaos2.org      http://chaos2.org/
   jacob(at)gnifty.net        http://www.gnifty.net/

   AI turn code


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

#include <saiint.h>              /* Main internal AI header */

#include <sgame/sconfig.h>       /* Dereference config for ph ptr */
#include <sgame/sinventory.h>    /* Need inv count (weapon selection) */
#include <sgame/sland.h>         /* Land's line-of-sight function */
#include <sgame/splayer.h>       /* Need access to player functions */
#include <sgame/sshield.h>       /* Raising/lowering shields */
#include <sgame/stankpro.h>      /* Need tank radius data */
#include <sgame/sweapon.h>       /* How many weapons are there? */
#include <sgame/swindow.h>       /* Needed for the status display */
#include <sutil/srand.h>         /* Needed for random trajectory */


/*

   Not you again.  
   *sigh*

   Ed:     Do you own a video camera?
   Renee:  No, Fred hates them.
   Fred:   I like to remember things my own way.
   Ed:     What do you mean by that?
   Fred:   How I remember them, not necessarily the way they happened.

   The whole key is right there.  It's just a matter of applying it

*/



typedef bool (*trajfn)(const struct _sc_config *c, sc_player *p, const sc_player *vp);



static void _sc_ai_select_last_weapon(const sc_config *c, sc_player *p) {
/* sc_ai_select_last_weapon
   Selects the last (highest-index) weapon available to the AI.  The naive
   AI's make the assumption that weapons further down the inventory list
   must clearly be better.  */

   sc_weapon_info *info;
   int count;

   assert(c != NULL && p != NULL);

   count = sc_weapon_count(c->weapons, SC_WEAPON_LIMIT_ALL);
   info = sc_weapon_first(c->weapons, SC_WEAPON_LIMIT_ALL | SC_WEAPON_SCAN_REVERSE);

   for(/* No initializer */; count > 0; --count) {
      if(info->inventories[p->index] > 0) {
         /* We found a weapon.. */
         if(SC_AI_DEBUG_SELECT) {
            printf("   AI %s selected weapon \"%s\"\n", sc_ai_name(p->aitype), info->name);
         }
         sc_player_set_weapon(c, p, info);
         return;
      } /* Do we have this weapon? */
   } /* Looking for a weapon... */

   /* Well, heh, if we get here we have no weapons -- should be impossible. */
   if(SC_AI_DEBUG_SELECT) {
      printf("   AI %s has no weapons to fire!\n", sc_ai_name(p->aitype));
   }

}



static void _sc_ai_select_weapon_by_score(const sc_config *c, sc_player *p) {
/* sc_ai_select_weapon_by_score
   Selects the highest-ranked weapon available to the AI.  This is slightly
   better, but we might end up selecting a very powerful weapon to kill only
   a single tank.  */

   sc_weapon_info *info;        /* Weapon tracking */
   sc_weapon_info *maxinfo;     /* Weapon index of best weapon */
   sc_weapon_info *nextinfo;    /* Weapon index of second-best */
   int maxscore;     /* Score of best weapon so far */
   int nextscore;    /* Score of second best weapon */
   int score;        /* Score of current weapon */
   int count;        /* Iterator */

   assert(c != NULL && p != NULL);

   /* Setup and iterate */   
   count = sc_weapon_count(c->weapons, SC_WEAPON_LIMIT_ALL);
   info = sc_weapon_first(c->weapons, SC_WEAPON_LIMIT_ALL | SC_WEAPON_SCAN_REVERSE);
   maxinfo = NULL;
   nextinfo= NULL;
   maxscore = 0;
   nextscore= -1;

   for(/* No initializer */; count > 0; --count) {
      if(info->inventories[p->index] > 0) {
         /* We have some of this weapon; is it ideal? */
         score = sc_weapon_statistic(c->weapons, info, p, SC_WEAPON_STAT_YIELD);
         if(score > maxscore) {
            /* This weapon is ideal (so far) */
            nextinfo = maxinfo;
            nextscore= maxscore;
            maxinfo  = info;
            maxscore = score;
         } else if(score > nextscore) {
            /* weapon is second-best (so far) */
            nextinfo = info;
            nextscore= score;
         } /* Does this weapon have a better score? */
      } /* Do we have this weapon in inventory? */
      info = sc_weapon_next(c->weapons, info, SC_WEAPON_LIMIT_ALL | SC_WEAPON_SCAN_REVERSE);
   } /* Loop through all weapons. */

   /* Set new weapon to one of the "best" found. */
   if(nextinfo != NULL && game_drand() < (double)nextscore / (nextscore + maxscore)) {
      /* Probabilistically select "next best" */
      maxinfo = nextinfo;
   }

   /* We have only one option now */
   if(maxinfo != NULL) {
      sc_player_set_weapon(c, p, maxinfo);
      if(SC_AI_DEBUG_SELECT) {
         printf("   AI %s selected weapon \"%s\"\n", sc_ai_name(p->aitype), maxinfo->name);
      }
   } else if(SC_AI_DEBUG_SELECT) {
      printf("   AI %s has no weapons to fire!\n", sc_ai_name(p->aitype));
   }

}



static void _sc_ai_select_shield_sappers(const sc_config *c, sc_player *p, const sc_player *target) {
/* sc_ai_select_shield_sappers
   If we are not selecting offset targetting and the target is
   shielded, then we will fire shield sappers if we have them.  */

   int count;
   sc_weapon_info *info;
   sc_weapon_info *sapper;

   assert(c != NULL && p != NULL);
   assert(target == NULL || SC_PLAYER_IS_ALIVE(target));
   assert(target == NULL || p->index != target->index);

   /* TEMP - We cannot use shield sappers because AI purchasing economics are
             currently too fucked up.  Also most AIs couldn't hit a mountain
             if they were in the middle of it!  */
   return;

   /* We only use the Shield Sappers if the AI intends to hit dead on. */
   if(target == NULL || SC_AI_WILL_OFFSET(c, target))
      return;

   /* We only use Sappers if the target is shielded. */
   if(target->shield == NULL)
      return;

   /* We don't use Sappers if our primary weapon choice is a spread weapon. */
   if(SC_WEAPON_SCATTERING(p->selweapon))
      return;

   /* Search for a shield sapping weapon in AI inventory. */
   count = sc_weapon_count(c->weapons, SC_WEAPON_LIMIT_ALL);
   info = sc_weapon_first(c->weapons, SC_WEAPON_LIMIT_ALL | SC_WEAPON_SCAN_REVERSE);
   sapper = NULL;
   for(; count > 0; --count) {
      if(SC_WEAPON_IS_SAPPER(info) && info->inventories[p->index] > 0) {
         sapper = info;
         break;
      } else {
         info = sc_weapon_next(c->weapons, info, SC_WEAPON_LIMIT_ALL | SC_WEAPON_SCAN_REVERSE);
      }
   }

   /* Set the weapon, if we found one. */
   if(sapper != NULL) {
      sc_player_set_weapon(c, p, sapper);
      if(SC_AI_DEBUG_SELECT) {
         printf("   AI %s using Shield Sappers (special select)\n", sc_ai_name(p->aitype));
      }
   } else if(SC_AI_DEBUG_SELECT) {
      printf("   AI %s wanted Shield Sappers but has none\n", sc_ai_name(p->aitype));
   }

}



static const sc_player *_sc_ai_random_fire(const sc_config *c, sc_player *p) {
/* sc_ai_random_fire
   Fires a random trajectory and power.  This is the simplest form of AI;
   currently, MORON uses it exclusively, and other AI's may use this
   mechanism as a starter or a fallback targetting system.  */

   int min_power;
   int max_power;
   int power_range;
   int power;
   
   assert(c != NULL && p != NULL);

   /* Mark Anderson corrected this in 20011029015201.A3951@www.muking.org */
   min_power   = max(SC_AI_MORON_MIN_POWER, p->power - SC_AI_POWER_DELTA_MAX);
   max_power   = max(SC_AI_MORON_MIN_POWER, p->power + SC_AI_POWER_DELTA_MAX);
   power_range = max_power - min_power;
   power       = min_power + game_lrand(power_range + 1);

   /* Set a new angle and power level. */
   sc_player_advance_turret(c, p, game_lrand(SC_AI_ANGLE_DELTA_MAX * 2 + 1) - SC_AI_ANGLE_DELTA_MAX);
   sc_player_set_power(c, p, power);

   /* No victim used */
   p->ai->victim = NULL;

   if(SC_AI_DEBUG_TRAJECTORY) {
      printf("AI_trajectory:   %s, %s random fire.\n", sc_ai_name(p->aitype), p->name);
   }

   return(NULL);

}



static inline int _sc_ai_line_of_sight(const sc_config *c, sc_player *p, const sc_player *vp, trajfn line, trajfn noline) {
/* sc_ai_line_of_sight
   Call the appropriate trajectory function.  */

   assert(c != NULL && p != NULL && vp != NULL);
   assert(SC_PLAYER_IS_ALIVE(vp));
   assert(p->index != vp->index);

   if(sc_land_line_of_sight(c,c->land, p->x - p->tank->radius, p->y, vp->x - p->tank->radius, vp->y) ||
     sc_land_line_of_sight(c, c->land, p->x - p->tank->radius, p->y, vp->x + p->tank->radius, vp->y) ||
     sc_land_line_of_sight(c, c->land, p->x + p->tank->radius, p->y, vp->x - p->tank->radius, vp->y) ||
     sc_land_line_of_sight(c, c->land, p->x + p->tank->radius, p->y, vp->x + p->tank->radius, vp->y)) {
      return(line(c, p, vp));
   } else {
      return(noline(c, p, vp));
   }

}



static void _sc_ai_target_practice(const sc_config *c, sc_player **playerlist) {
/* sc_ai_target_practice
   I feel kind of sorry for the poor, poor humans...
   Actually, no, I don't :)  */

   sc_player *tmp;
   int i;
   int j;

   assert(c != NULL && playerlist != NULL);

   if(!c->aicontrol->humantargets) return;

   for(i = 0; i < c->numplayers; ++i) {
      if(playerlist[i]->aitype == SC_AI_HUMAN) {
         tmp = playerlist[i];
         for(j = 0; j < i && tmp != NULL; ++j) {
            if(playerlist[j]->aitype != SC_AI_HUMAN) {
               playerlist[i] = playerlist[j];
               playerlist[j] = tmp;
               tmp = NULL;
            }
         }
      }
   }

}



static const sc_player *_sc_ai_calculated_fire(const sc_config *c, sc_player *p) {
/* sc_ai_calculated_fire
   See if we can shoot anyone with the above two trajectory mechanisms.  We
   check players in random order, and if we can reach any of them, then fire
   we go.  Otherwise, we fall back on random firing scheme.  This mechanism
   only considers gravity, which is sufficient for up to CHOOSER.  */

   const sc_player *playerlist[SC_MAX_PLAYERS]; /* List of players, random order */
   const sc_player *vp; /* Current "victim" */
   int victim;          /* Index to "victim" */

   assert(c != NULL && p != NULL);

   /* Get a random ordering of players */
   sc_player_random_order((sc_config *)c, (sc_player **)playerlist);
   _sc_ai_target_practice(c, (sc_player **)playerlist);

   /* Try to find a victim. */   
   victim = 0;
   while(victim < c->numplayers) {
      vp = playerlist[victim];
      if(p->index != vp->index && SC_PLAYER_IS_ALIVE(vp)) {
         /* "Victim" isn't ourself (that would suck) */
         if(_sc_ai_line_of_sight(c, p, vp, sc_ai_trajectory_line, sc_ai_trajectory)) {
            /* Victim selected */
            return(vp);
         }
      } /* Make sure victim isn't self */
      ++victim;   /* Next please */
   } /* Looking for a victim ... */

   /* No suitable victim; just fire at random. */
   return(_sc_ai_random_fire(c, p));

}



static const sc_player *_sc_ai_calculated_fire_wind(const sc_config *c, sc_player *p) {
/* sc_ai_calculated_fire_wind
   See if we can shoot anyone with the above two trajectory mechanisms.  We
   check players in random order, and if we can reach any of them, then fire
   we go.  Otherwise, we fall back on random firing scheme.  This mechanism
   considers gravity and wind, which CALCULATER needs.  */

   const sc_player *playerlist[SC_MAX_PLAYERS]; /* List of players, random order */
   const sc_player *vp; /* Current "victim" */
   int victim;          /* Index to "victim" */

   assert(c != NULL && p != NULL);

   /* Get a random ordering of players */
   sc_player_random_order((sc_config *)c, (sc_player **)playerlist);
   _sc_ai_target_practice(c, (sc_player **)playerlist);

   /* Try to find a victim. */   
   victim = 0;
   while(victim < c->numplayers) {
      vp = playerlist[victim];
      if(p->index != vp->index && SC_PLAYER_IS_ALIVE(vp)) {
         /* "Victim" isn't ourself (that would suck) */
         if(_sc_ai_line_of_sight(c, p, vp, sc_ai_trajectory_line_wind, sc_ai_trajectory_wind)) {
            /* Victim selected */
            return(vp);
         }
      } /* Make sure victim isn't self */
      ++victim;   /* Next please */
   } /* Looking for a victim ... */

   /* No suitable victim; just fire at random. */
   return(_sc_ai_random_fire(c, p));

}



static const sc_player *_sc_ai_fire_at_victim(const sc_config *c, sc_player *p) {
/* sc_ai_fire_at_victim
   Attack victim in AI state.  If NULL or they are dead, then behave like
   calculated_fire.  This is the "big bully" behaviour; once a victim is
   chosen, well, they're pretty much dead.  */

   const sc_player *vp;

   assert(c != NULL && p != NULL);

   /* Is the victim still alive? */
   vp = p->ai->victim;
   if(vp != NULL && SC_PLAYER_IS_ALIVE(vp)) {
      /* "Victim" isn't dead; can we shoot them? */
      if(_sc_ai_line_of_sight(c, p, vp, sc_ai_trajectory_line, sc_ai_trajectory)) {
         /* I AM A RETURN STATEMENT!  FEAR MY WRATH! */
         return(vp);
      }
   } /* Was victim alive? */

   /* If at this point, we have no victim; find another. */
   return(_sc_ai_calculated_fire(c, p));

}



static const sc_player *_sc_ai_fire_at_victim_ruthlessly(const sc_config *c, sc_player *p) {
/* sc_ai_fire_at_victim_ruthlessly
   Attack victim in AI state.  If NULL or they are dead, then behave like
   calculated_fire.  This is the "big bully" behaviour; once a victim is
   chosen, well, they're pretty much dead.  This is insane, it compensates
   for wind.  */

   const sc_player *vp;

   assert(c != NULL && p != NULL);

   /* Is the victim still alive? */
   vp = p->ai->victim;
   if(vp != NULL && SC_PLAYER_IS_ALIVE(vp)) {
      /* "Victim" isn't dead; can we shoot them? */
      if(_sc_ai_line_of_sight(c, p, vp, sc_ai_trajectory_line_wind, sc_ai_trajectory_wind)) {
         /* I AM A RETURN STATEMENT!  PH33R MY LACK OF B33R! */
         return(vp);
      }
   } /* Was victim alive? */

   /* If at this point, we have no victim; find another. */
   return(_sc_ai_calculated_fire_wind(c, p));

}



static void _sc_ai_raise_shields(const sc_config *c, sc_player *p) {
/* sc_ai_raise_shields
   If we don't have any shielding, raise them now.  */

   assert(c != NULL && p != NULL);

   if(p->shield == NULL || p->shield->life <= 0) {
      sc_player_activate_best_shield(c, p);
   }

}



static void _sc_ai_set_contact_triggers(const sc_config *c, sc_player *p) {
/* sc_ai_set_contact_triggers
   Always try to arm the contact triggers, if we have any.  */

   assert(c != NULL && p != NULL);

   sc_player_set_contact_triggers(c, p, true);

}



static void _sc_ai_recharge_tank(const sc_config *c, sc_player *p) {
/* sc_ai_recharge_tank
   Recharge tank to full health, if weakened.  */

   assert(c != NULL && p != NULL);

   while(sc_player_activate_battery(c, p)) /* Just loop */;

}



static inline void _sc_ai_turn_status(const sc_config *c, const sc_player *p) {
/* sc_ai_turn_status */

   assert(c != NULL && p != NULL);

   if(SC_CONFIG_GFX_FAST(c)) return;
   sc_status_player_message(c->window, p, "AI player is taking their turn ...");
   sc_window_idle(c->window);

}



sc_ai_result sc_ai_player_turn(const sc_config *c, sc_player *p) {
/* sc_ai_player_turn
   AI player takes a turn. */

   const sc_player *target = NULL;

   assert(c != NULL && p != NULL);

   switch(p->ai->realaitype) {
      case SC_AI_HUMAN:
         return(SC_AI_NO_ACTION);
         break;

      case SC_AI_RANDOM:
      case SC_AI_NETWORK:
         /* No-ops */
         break;

      case SC_AI_MORON:
         _sc_ai_turn_status(c, p);
         _sc_ai_raise_shields(c, p);
         _sc_ai_set_contact_triggers(c, p);
         _sc_ai_recharge_tank(c, p);
         _sc_ai_select_last_weapon(c, p);
         target = _sc_ai_random_fire(c, p);
         _sc_ai_select_shield_sappers(c, p, target);
         break;

      case SC_AI_SHOOTER:
      case SC_AI_SPREADER:
         _sc_ai_turn_status(c, p);
         _sc_ai_raise_shields(c, p);
         _sc_ai_set_contact_triggers(c, p);
         _sc_ai_recharge_tank(c, p);
         _sc_ai_select_weapon_by_score(c, p);
         target = _sc_ai_calculated_fire(c, p);
         _sc_ai_select_shield_sappers(c, p, target);
         break;

      case SC_AI_CHOOSER:
         _sc_ai_turn_status(c, p);
         _sc_ai_raise_shields(c, p);
         _sc_ai_set_contact_triggers(c, p);
         _sc_ai_recharge_tank(c, p);
         _sc_ai_select_weapon_by_score(c, p);
         target = _sc_ai_fire_at_victim(c, p);
         _sc_ai_select_shield_sappers(c, p, target);
         break;

      case SC_AI_CALCULATER:
      case SC_AI_ANNIHILATER:
      case SC_AI_INSANITY:
         _sc_ai_turn_status(c, p);
         _sc_ai_raise_shields(c, p);
         _sc_ai_set_contact_triggers(c, p);
         _sc_ai_recharge_tank(c, p);
         _sc_ai_select_weapon_by_score(c, p);
         target = _sc_ai_fire_at_victim_ruthlessly(c, p);
         _sc_ai_select_shield_sappers(c, p, target);
         if(c->aicontrol->enablescan) {
            if(p->ai->victim != NULL) sc_ai_trajectory_scan(c, p, p->ai->victim);
         } /* scan refinement? */
         break;

   }

   return(SC_AI_CONTINUE);

}
