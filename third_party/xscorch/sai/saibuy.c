/* $Header: /fridge/cvs/xscorch/sai/saibuy.c,v 1.31 2011-08-01 00:01:40 jacob Exp $ */
/*

   xscorch - sai.c            Copyright(c) 2000-2003 Justin David Smith
                              Copyright(c) 2001-2003 Jacob Luna Lundberg
   justins(at)chaos2.org      http://chaos2.org/
   jacob(at)gnifty.net        http://www.gnifty.net/

   AI purchasing code


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
#include <saiint.h>              /* AI internal header */

#include <sgame/sconfig.h>       /* Need basic configuration */
#include <sgame/seconomy.h>      /* Need current economical state */
#include <sgame/sinventory.h>    /* Need to get player inventory */
#include <sgame/splayer.h>       /* Need player data */
#include <sgame/sweapon.h>       /* Yield calculations */
#include <sgame/swindow.h>       /* Used for status messages */



static inline int _sc_ai_budget(const sc_config *c, const sc_player *p) {
/* sc_ai_budget
   Calculate the amount of money the AI is permitted to spend this turn */

   const sc_economy_config *ec; /* Economy data */
   double percent;              /* Percentage of rounds played */

   /* Any budget constraints? */
   if(c->aicontrol->nobudget) {
      return(p->money * 0.75);
   }

   ec = c->economics;
   if(ec->computersbuy) {
      /* Only buy based on current interest rate, or the default
         conservative budget, whichever is larger */
      if(ec->computersaggressive) {
         percent = SC_AI_AGGRESSIVE_BUDGET;
      } else {
         percent = SC_AI_CONSERVATIVE_BUDGET;
      } /* What rate should we set as the "minimum"? */
      if(ec->interestrate > percent) {
         percent = ec->currentinterest;
      } /* Take maximum of "percent" and current interest rate */

      /* Also factor in the number of rounds remaining :) */
      percent += ((double)(c->curround + 1) / c->numrounds) * (1 - percent);
      if(percent > 1) percent = 1;

      if(SC_AI_DEBUG_BUY) {
         printf("AI_budget:   %s, %s has budget %d\n", sc_ai_name(p->aitype), p->name, (int)(p->money * percent));
      }

      /* That ought to be sufficiently devastating. */
      return(p->money * percent);
   } /* Can computers buy? */

   /* Computers cannot buy. */
   return(0);

}



static bool _sc_ai_buy_last_weapons(const sc_config *c, sc_player *p, int *budget) {
/* sc_ai_buy_last_weapons
   Buy the last weapons on the list that we can afford.  This is a rather
   naive approach, in assuming that weapons at the end of the list must
   surely be better?  */

   sc_weapon_info *info;        /* Weapon information */
   int count = sc_weapon_count(c->weapons, SC_WEAPON_LIMIT_ALL);
   int inner, outer;            /* Iterators */
   bool result = false;

   for(outer = SC_AI_BUY_MAX_OF_WEAPON; outer > 0; --outer) {
      info = sc_weapon_first(c->weapons, SC_WEAPON_LIMIT_ALL | SC_WEAPON_SCAN_REVERSE);
      for(inner = count; inner > 0; --inner) {
         if(sc_inventory_can_buy_weapon(p, info, *budget) &&
            sc_weapon_statistic(c->weapons, info, p, SC_WEAPON_STAT_YIELD) > 0) {
            /* Buy some of this weapon. */
            sc_inventory_buy_weapon(p, info);
            *budget -= info->price;
            if(SC_AI_DEBUG_BUY) {
               printf("   AI_buy:   %s, %s bought %s.  $%d\n", sc_ai_name(p->aitype), p->name, info->name, *budget);
            }
            result = true;
         } /* Can we buy this weapon? */
         info = sc_weapon_next(c->weapons, info, SC_WEAPON_LIMIT_ALL | SC_WEAPON_SCAN_REVERSE);
      } /* Iteration on info lists */
   } /* Iterate multiples of weapons */

   return(result);

}



static void _sc_ai_sorted_weapon_scores(sc_weapon_config *wc, int *ordering, double *scores) {
/* sc_ai_sorted_weapon_scores
   Impose an ordering on the weapons, based on scores loaded into *scores.  */

   int inner, outer;             /* Iterator variables... */
   int count = sc_weapon_count(wc, SC_WEAPON_LIMIT_ALL);

   /* Bubble-sort the scores */   
   for(outer = 0; outer < count; ++outer) {
      for(inner = 0; inner < outer; ++inner) {
         if(scores[inner] > scores[outer]) {
            scores[inner] += scores[outer];
            scores[outer] = scores[inner] - scores[outer];
            scores[inner] = scores[inner] - scores[outer];
            ordering[inner] += ordering[outer];
            ordering[outer] = ordering[inner] - ordering[outer];
            ordering[inner] = ordering[inner] - ordering[outer];
         }
      }
   }

}



static bool _sc_ai_buy_weapons_from_list(const sc_config *c, sc_player *p, int *budget, const int *ordering) {
/* sc_ai_buy_weapons_from_list
   Buys weapons from a presorted list, beginning at the _end_ of the list.  */

   sc_weapon_info *info;   /* Weapon information. */
   int count = sc_weapon_count(c->weapons, SC_WEAPON_LIMIT_ALL);
   int inner, outer;             /* iterators */
   bool result = false;

   /* Iterate through, beginning with the "best" weapons. */
   for(outer = 0; outer < SC_AI_BUY_MAX_OF_WEAPON; ++outer) {
      for(inner = count - 1; inner >= 0; --inner) {
         info = sc_weapon_lookup(c->weapons, ordering[inner], SC_WEAPON_LIMIT_ALL);
         /* We don't want to buy weapons that won't kill things!
            We also bias slightly in favor of buying weapons we don't own yet. */
         if(sc_inventory_can_buy_weapon(p, info, *budget) &&
            sc_weapon_statistic(c->weapons, info, p, SC_WEAPON_STAT_YIELD) &&
            (outer || (info->inventories[p->index] / info->bundle) < SC_AI_INVENTORY_HIGH)) {
            /* buy some of this weapon. */
            sc_inventory_buy_weapon(p, info);
            *budget -= info->price;
            if(SC_AI_DEBUG_BUY) {
               printf("   AI_buy:   %s, %s bought %s.  $%d\n", sc_ai_name(p->aitype), p->name, info->name, *budget);
            }
            result = true;
         } /* Can we buy this weapon? */
      } /* Buying weapons */
   } /* Iterate multiples of weapons */

   return(result);

}



static bool _sc_ai_buy_weapons_by_score(const sc_config *c, sc_player *p, int *budget) {
/* sc_ai_buy_weapons_by_score
   Buy the weapons yielding the best destructive power.  This is a great
   buying strategy for players that just want to annihilate the entire
   playing field.  Coupled with agressive, this can be quite deadly!  */

   sc_weapon_info *info;
   int count = sc_weapon_count(c->weapons, SC_WEAPON_LIMIT_ALL);
   /* Sorted indices, based on score. */
   int *ordering = (int *)malloc(sizeof(int) * count);
   /* Sorted scores corresponding to ordering */
   double *scores = (double *)malloc(sizeof(double) * count);
   int i; /* Iterator */
   bool result;

   if(ordering == NULL || scores == NULL) {
      fprintf(stderr, "Malloc error in sc_ai_buy_weapons_by_score!\n");
      free(ordering);
      free(scores);
      return(false);
   }

   /* Calculate scores, and set initial ordering */
   info = sc_weapon_first(c->weapons, SC_WEAPON_LIMIT_ALL | SC_WEAPON_SCAN_FORWARD);
   for(i = 0; i < count; ++i) {
      ordering[i] = info->ident;
      scores[i] = sc_weapon_statistic(c->weapons, info, p, SC_WEAPON_STAT_YIELD);
      info = sc_weapon_next(c->weapons, info, SC_WEAPON_LIMIT_ALL | SC_WEAPON_SCAN_FORWARD);
   }

   /* Bubble-sort the scores */   
   _sc_ai_sorted_weapon_scores(c->weapons, ordering, scores);

   /* Iterate through, beginning with the "best" weapons. */
   result = _sc_ai_buy_weapons_from_list(c, p, budget, ordering);

   free(ordering);
   free(scores);
   return(result);

}



static bool _sc_ai_buy_weapons_by_weighted_score(const sc_config *c, sc_player *p, int *budget) {
/* sc_ai_buy_weapons_by_weighted_score
   Buy the weapons yielding the best destructive power per price of
   unit.  This is the "efficient" way to buy, but there are times when
   a good screen-destroying weapon is better than a weapon that will
   take us 100 turns to wipe out the opponent...  */

   sc_weapon_info *info;
   int count = sc_weapon_count(c->weapons, SC_WEAPON_LIMIT_ALL);
   /* Sorted indices, based on score. */
   int *ordering = (int *)malloc(sizeof(int) * count);
   /* Sorted scores corresponding to ordering */
   double *scores = (double *)malloc(sizeof(double) * count);
   int i; /* Iterator */
   bool result;

   if(ordering == NULL || scores == NULL) {
      fprintf(stderr, "Malloc error in sc_ai_buy_weapons_by_weighted_score!\n");
      free(ordering);
      free(scores);
      return(false);
   }

   /* Calculate scores, and set initial ordering */
   info = sc_weapon_first(c->weapons, SC_WEAPON_LIMIT_ALL | SC_WEAPON_SCAN_FORWARD);
   for(i = 0; i < count; ++i) {
      ordering[i] = info->ident;
      scores[i] = sc_weapon_statistic(c->weapons, info, p, SC_WEAPON_STAT_ECON_YIELD);
      info = sc_weapon_next(c->weapons, info, SC_WEAPON_LIMIT_ALL | SC_WEAPON_SCAN_FORWARD);
   }

   /* Bubble-sort the scores */   
   _sc_ai_sorted_weapon_scores(c->weapons, ordering, scores);

   /* Iterate through, beginning with the "best" weapons. */
   result = _sc_ai_buy_weapons_from_list(c, p, budget, ordering);

   free(ordering);
   free(scores);
   return(result);

}



static bool _sc_ai_buy_precision_weapons(const sc_config *c, sc_player *p, int *budget) {
/* sc_ai_buy_precision_weapons
   Buy the weapons yielding high power and precision.  This is for use
   by AI players who know exactly how to aim and exactly what/who they
   want to destroy.  Such a player can take out her neighbor without
   also destroying herself as one might expect of AI players that make
   use of _sc_ai_buy_weapons_by_score(). */

   sc_weapon_info *info;
   int count = sc_weapon_count(c->weapons, SC_WEAPON_LIMIT_ALL);
   /* Sorted indices, based on score. */
   int *ordering = (int *)malloc(sizeof(int) * count);
   /* Sorted scores corresponding to ordering */
   double *scores = (double *)malloc(sizeof(double) * count);
   int i; /* Iterator */
   bool result;

   if(ordering == NULL || scores == NULL) {
      fprintf(stderr, "Malloc error in sc_ai_buy_precision_weapons!\n");
      free(ordering);
      free(scores);
      return(false);
   }

   /* Calculate scores, and set initial ordering */
   info = sc_weapon_first(c->weapons, SC_WEAPON_LIMIT_ALL | SC_WEAPON_SCAN_FORWARD);
   for(i = 0; i < count; ++i) {
      ordering[i] = info->ident;
      scores[i] = sc_weapon_statistic(c->weapons, info, p, SC_WEAPON_STAT_PRECISION_YIELD);
      info = sc_weapon_next(c->weapons, info, SC_WEAPON_LIMIT_ALL | SC_WEAPON_SCAN_FORWARD);
   }

   /* Bubble-sort the scores */   
   _sc_ai_sorted_weapon_scores(c->weapons, ordering, scores);

   /* Iterate through, beginning with the "best" weapons. */
   result = _sc_ai_buy_weapons_from_list(c, p, budget, ordering);

   free(ordering);
   free(scores);
   return(result);

}



static bool _sc_ai_buy_shield_sappers(const sc_config *c, sc_player *p, int *budget) {
/* sc_ai_buy_shield_sappers
   Buy up Shield Sappers, which have no yield,
   but AIs can use them against shielded players. */

   sc_weapon_info *info;        /* Weapon information */
   int count = sc_weapon_count(c->weapons, SC_WEAPON_LIMIT_ALL);
   int sapbudget = *budget / 4; /* Sappers shouldn't drain the budget */
   int inner, outer;            /* Iterators */
   bool result = false;

/* TEMP - We're not using this for now.  See the comment in the
   shield sappers section of saiturn.c for more information...  */
return(result);

   for(outer = SC_AI_BUY_MAX_OF_WEAPON; outer > 0; --outer) {
      info = sc_weapon_first(c->weapons, SC_WEAPON_LIMIT_ALL | SC_WEAPON_SCAN_REVERSE);
      for(inner = count; inner > 0; --inner) {
         if(sc_inventory_can_buy_weapon(p, info, sapbudget) && SC_WEAPON_IS_SAPPER(info)) {
            /* Buy some of this weapon. */
            sc_inventory_buy_weapon(p, info);
            *budget -= info->price;
            if(SC_AI_DEBUG_BUY) {
               printf("   AI_buy:   %s, %s bought %s.  $%d\n", sc_ai_name(p->aitype), p->name, info->name, *budget);
            }
            result = true;
         } /* Can we buy this weapon? */
         info = sc_weapon_next(c->weapons, info, SC_WEAPON_LIMIT_ALL | SC_WEAPON_SCAN_REVERSE);
      } /* Iteration on info lists */
   } /* Iterate multiples of weapons */

   return(result);

}



static bool _sc_ai_buy_auto_defense(const sc_config *c, sc_player *p, int *budget) {
/* sc_ai_buy_auto_defense
   Buy auto-defense, only if this is a sequential game. */

   sc_accessory_info *info;  /* Item information */
   int count = sc_accessory_count(c->accessories, SC_ACCESSORY_LIMIT_ALL);

   /* Check if this is a sequential game. */
   if(c->options.mode == SC_CONFIG_MODE_SYNCHRONOUS) return(false);

   /* If this is a sequential game, make damn sure we buy autodefense */
   info = sc_accessory_first(c->accessories, SC_ACCESSORY_LIMIT_ALL | SC_ACCESSORY_SCAN_FORWARD);
   for(; count > 0; --count) {
      if(sc_inventory_can_buy_accessory(p, info, *budget)) {
         /* Make sure the damn thing is auto-defence. */
         if(SC_ACCESSORY_IS_AUTO_DEF(info)) {
            /* Buy some of this autodefense */
            sc_inventory_buy_accessory(p, info);
            *budget -= info->price;
            if(SC_AI_DEBUG_BUY) {
               printf("   AI_buy:   %s, %s bought %s.  $%d\n", sc_ai_name(p->aitype), p->name, info->name, *budget);
            }
            return(true);
         } /* Is accessory auto-defense? */
      } /* Can we buy? */
      info = sc_accessory_next(c->accessories, info, SC_ACCESSORY_LIMIT_ALL | SC_ACCESSORY_SCAN_FORWARD);
   } /* Iterate through */

   return(false);

}



static bool _sc_ai_buy_contact_triggers(const sc_config *c, sc_player *p, int *budget) {
/* sc_ai_buy_contact_triggers
   Buy contact triggers if tunneling is allowed. */

   sc_accessory_info *info;  /* Item information */
   int count = sc_accessory_count(c->accessories, SC_ACCESSORY_LIMIT_ALL);

   /* Only buy if tunneling is allowed. */
   if(!c->weapons->tunneling) return(false);

   /* Buy contact triggers if we can. */
   info = sc_accessory_first(c->accessories, SC_ACCESSORY_LIMIT_ALL | SC_ACCESSORY_SCAN_FORWARD);
   for(; count > 0; --count) {
      /* Make sure the damn thing is a contact trigger. */
      if(SC_ACCESSORY_IS_TRIGGER(info)) {
         while(sc_inventory_can_buy_accessory(p, info, *budget)) {
            /* Buy some of these contact triggers */
            sc_inventory_buy_accessory(p, info);
            *budget -= info->price;
            if(SC_AI_DEBUG_BUY) {
               printf("   AI_buy:   %s, %s bought %s.  $%d\n", sc_ai_name(p->aitype), p->name, info->name, *budget);
            }
            return(true);
         } /* Can we buy? */
      } /* Is accessory a contact trigger? */
      info = sc_accessory_next(c->accessories, info, SC_ACCESSORY_LIMIT_ALL | SC_ACCESSORY_SCAN_FORWARD);
   } /* Iterate through */

   return(false);

}



static bool _sc_ai_buy_last_shields(const sc_config *c, sc_player *p, int *budget) {
/* sc_ai_buy_last_shields
   Buy the last shields on the list that we can afford.  This is a rather
   naive approach, in assuming that shields at the end of the list must
   surely be better?  */

   sc_accessory_info *info;  /* Shield information */
   int count = sc_accessory_count(c->accessories, SC_ACCESSORY_LIMIT_ALL);

   /* Buy some shielding */
   info = sc_accessory_first(c->accessories, SC_ACCESSORY_LIMIT_ALL | SC_ACCESSORY_SCAN_REVERSE);
   for(; count > 0; --count) {
      if(sc_inventory_can_buy_accessory(p, info, *budget)) {
         /* Make sure the damn thing is a shield. */
         if(SC_ACCESSORY_IS_SHIELD(info)) {
            /* Buy some of this shield */
            sc_inventory_buy_accessory(p, info);
            *budget -= info->price;
            if(SC_AI_DEBUG_BUY) {
               printf("   AI_buy:   %s, %s bought %s.  $%d\n", sc_ai_name(p->aitype), p->name, info->name, *budget);
            }
            return(true);  /* That's all, for now */
         } /* Is accessory a shield? */
      } /* Can we buy? */
      info = sc_accessory_next(c->accessories, info, SC_ACCESSORY_LIMIT_ALL | SC_ACCESSORY_SCAN_REVERSE);
   } /* Iterate up the accessory list. */

   return(false);

}



static void _sc_ai_sorted_accessory_scores(sc_accessory_config *ac, int *ordering, int *scores) {
/* sc_ai_sorted_accessory_scores
   Impose an ordering on the accessorys, based on scores loaded into *scores.  */

   int inner, outer;             /* Iterator variables... */
   int count = sc_accessory_count(ac, SC_ACCESSORY_LIMIT_ALL);

   /* Bubble-sort the scores */   
   for(outer = 0; outer < count; ++outer) {
      for(inner = 0; inner < outer; ++inner) {
         if(scores[inner] > scores[outer]) {
            scores[inner] += scores[outer];
            scores[outer] = scores[inner] - scores[outer];
            scores[inner] = scores[inner] - scores[outer];
            ordering[inner] += ordering[outer];
            ordering[outer] = ordering[inner] - ordering[outer];
            ordering[inner] = ordering[inner] - ordering[outer];
         }
      }
   }

}



static bool _sc_ai_buy_shields_from_list(const sc_config *c, sc_player *p, int *budget, const int *ordering) {
/* sc_ai_buy_shields_from_list
   Buy shields from the ordered list given; keep trying to
   buy until we find a set we can afford, then stop there.  */

   sc_accessory_info *info;  /* Accessory information. */
   int count = sc_accessory_count(c->accessories, SC_ACCESSORY_LIMIT_ALL);

   /* Iterate through, beginning with the "best" shields. */
   for(--count; count >= 0; --count) {
      /* Make sure the damn thing is a shield. */
      info = sc_accessory_lookup(c->accessories, ordering[count], SC_ACCESSORY_LIMIT_ALL);
      if(SC_ACCESSORY_IS_SHIELD(info)) {
         /* Can we afford this set of shields? */
         if(sc_inventory_can_buy_accessory(p, info, *budget)) {
            /* buy some of this shielding. */
            sc_inventory_buy_accessory(p, info);
            *budget -= info->price;
            if(SC_AI_DEBUG_BUY) {
               printf("   AI_buy:   %s, %s bought %s.  $%d\n", sc_ai_name(p->aitype), p->name, info->name, *budget);
            }
            return(true);  /* That's all, for now */
         } /* Can we buy these shields? */
      } /* Is this accessory a shield? */
   } /* Iterate through the shields */

   return(false);

}



static bool _sc_ai_buy_best_shields(const sc_config *c, sc_player *p, int *budget) {
/* sc_ai_buy_best_shields
   Buy the best shields on the list that we can afford.  We will
   want to sort the shields out before deciding which to buy.  */

   sc_accessory_info *info;
   int count = sc_accessory_count(c->accessories, SC_ACCESSORY_LIMIT_ALL);
   /* Sorted indices, based on score. */
   int *ordering = (int *)malloc(sizeof(int) * count);
   /* Sorted scores corresponding to ordering */
   int *scores = (int *)malloc(sizeof(int) * count);
   int i; /* Iterator */
   bool result;

   if(ordering == NULL || scores == NULL) {
      fprintf(stderr, "Malloc error in sc_ai_buy_best_shields!\n");
      free(ordering);
      free(scores);
      return(false);
   }

   /* Calculate scores, and set initial ordering */
   info = sc_accessory_first(c->accessories, SC_ACCESSORY_LIMIT_ALL | SC_ACCESSORY_SCAN_FORWARD);
   for(i = 0; i < count; ++i) {
      ordering[i] = info->ident;
      /* TEMP HACK - This should call a function in sgame/saccessory.c like the weapon version does. -JL */
      /* TEMP NOTE ABOUT MY TEMP HACK - Actually maybe sgame/sshield.c would make more sense, heh... */
      scores[i] = info->shield;  /* = sc_accessory_yield(c->accessories, info); */
      info = sc_accessory_next(c->accessories, info, SC_ACCESSORY_LIMIT_ALL | SC_ACCESSORY_SCAN_FORWARD);
   }

   /* TEMP NOTE NO. 2 - Ideally we should decide what's best based on type as well...
                        For example, a spreader probably prefers standard shielding.  ? */
   /* Bubble-sort the scores */
   _sc_ai_sorted_accessory_scores(c->accessories, ordering, scores);

   /* Iterate through, beginning with the "best" shields. */
   result = _sc_ai_buy_shields_from_list(c, p, budget, ordering);

   free(ordering);
   free(scores);
   return(result);

}



static bool _sc_ai_buy_batteries(const sc_config *c, sc_player *p, int *budget) {
/* sc_ai_buy_batteries
   Buys some batteries for the AI.  */

   sc_accessory_info *info;
   int count = sc_accessory_count(c->accessories, SC_ACCESSORY_LIMIT_ALL);

   /* Buy batteries if we can. */
   info = sc_accessory_first(c->accessories, SC_ACCESSORY_LIMIT_ALL | SC_ACCESSORY_SCAN_FORWARD);
   for(; count > 0; --count) {
      /* Make sure the damn thing is a battery. */
      if(SC_ACCESSORY_IS_BATTERY(info)) {
         while(sc_inventory_can_buy_accessory(p, info, *budget) && sc_player_battery_count(c, p) < SC_AI_MAX_BATTERIES) {
            /* Buy some of these here batteries */
            sc_inventory_buy_accessory(p, info);
            *budget -= info->price;
            if(SC_AI_DEBUG_BUY) {
               printf("   AI_buy:   %s, %s bought %s.  $%d\n", sc_ai_name(p->aitype), p->name, info->name, *budget);
            }
            return(true);
         } /* Can we buy? */
      } /* Is accessory a battery? */
      info = sc_accessory_next(c->accessories, info, SC_ACCESSORY_LIMIT_ALL | SC_ACCESSORY_SCAN_FORWARD);
   } /* Iterate through */

   return(false);

}



static inline void _sc_ai_buy_status(const sc_config *c, const sc_player *p) {
/* sc_ai_buy_status */

   if(SC_CONFIG_GFX_FAST(c)) return;
   sc_status_player_message(c->window, p, "AI player is buying ...");
   sc_window_idle(c->window);

}



sc_ai_result sc_ai_player_buy(const sc_config *c, sc_player *p) {
/* sc_ai_player_buy
   Buy some weapons and accessories.  */

   int budget;    /* Amount of money allocated to spend */

   budget = _sc_ai_budget(c, p);

   switch(p->ai->realaitype) {
      case SC_AI_HUMAN:
         return(SC_AI_NO_ACTION);
         break;

      case SC_AI_NETWORK:
      case SC_AI_RANDOM:
         /* No-ops */
         break;

      case SC_AI_MORON:
         _sc_ai_buy_status(c, p);
         _sc_ai_buy_contact_triggers(c, p, &budget);
         _sc_ai_buy_auto_defense(c, p, &budget);
         _sc_ai_buy_last_shields(c, p, &budget);
         _sc_ai_buy_last_weapons(c, p, &budget);
         break;

      case SC_AI_SHOOTER:
         _sc_ai_buy_status(c, p);
         _sc_ai_buy_contact_triggers(c, p, &budget);
         _sc_ai_buy_auto_defense(c, p, &budget);
         _sc_ai_buy_best_shields(c, p, &budget);
         _sc_ai_buy_shield_sappers(c, p, &budget);
         _sc_ai_buy_weapons_by_weighted_score(c, p, &budget);
         break;

      case SC_AI_SPREADER:
         _sc_ai_buy_status(c, p);
         _sc_ai_buy_contact_triggers(c, p, &budget);
         _sc_ai_buy_auto_defense(c, p, &budget);
         _sc_ai_buy_best_shields(c, p, &budget);
         _sc_ai_buy_weapons_by_score(c, p, &budget);
         _sc_ai_buy_shield_sappers(c, p, &budget);
         break;

      case SC_AI_CHOOSER:
         _sc_ai_buy_status(c, p);
         _sc_ai_buy_contact_triggers(c, p, &budget);
         _sc_ai_buy_auto_defense(c, p, &budget);
         _sc_ai_buy_best_shields(c, p, &budget);
         _sc_ai_buy_shield_sappers(c, p, &budget);
         _sc_ai_buy_precision_weapons(c, p, &budget);
         _sc_ai_buy_batteries(c, p, &budget);
         break;

      case SC_AI_CALCULATER:
         _sc_ai_buy_status(c, p);
         _sc_ai_buy_contact_triggers(c, p, &budget);
         _sc_ai_buy_auto_defense(c, p, &budget);
         _sc_ai_buy_best_shields(c, p, &budget);
         _sc_ai_buy_shield_sappers(c, p, &budget);
         _sc_ai_buy_precision_weapons(c, p, &budget);
         _sc_ai_buy_batteries(c, p, &budget);
         break;

      case SC_AI_ANNIHILATER:
      case SC_AI_INSANITY:
         _sc_ai_buy_status(c, p);
         budget = p->money * 0.85;
         _sc_ai_buy_contact_triggers(c, p, &budget);
         _sc_ai_buy_weapons_by_score(c, p, &budget);
         budget = p->money;
         _sc_ai_buy_auto_defense(c, p, &budget);
         _sc_ai_buy_best_shields(c, p, &budget);
         _sc_ai_buy_batteries(c, p, &budget);
         _sc_ai_buy_shield_sappers(c, p, &budget);
         break;

   }

   return(SC_AI_CONTINUE);

}
