/* $Header: /fridge/cvs/xscorch/sgame/splayer.c,v 1.49 2011-08-01 00:01:41 jacob Exp $ */
/*

   xscorch - splayer.c        Copyright(c) 2000-2003 Justin David Smith
                              Copyright(c) 2001-2003 Jacob Luna Lundberg
   justins(at)chaos2.org      http://chaos2.org/
   jacob(at)gnifty.net        http://www.gnifty.net/

   Scorched Player information


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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <splayer.h>
#include <saccessory.h>
#include <sconfig.h>
#include <seconomy.h>
#include <sexplosion.h>
#include <sinventory.h>
#include <sland.h>
#include <sshield.h>
#include <sspill.h>
#include <stankpro.h>
#include <sweapon.h>
#include <swindow.h>

#include <snet/snet.h>
#include <sutil/srand.h>

#include <libj/jstr/libjstr.h>



void sc_player_init_game(const sc_config *c, sc_player *p) {
/* sc_player_init_game
   Game initialization for this player */

   /* Initialise the player's currency and inventory */
   p->money = c->economics->initialcash;

   /* No wins (yet) */
   p->numwins = 0;
   p->kills = 0;
   p->suicides = 0;
   p->ac_state = 0;
   p->turret = 0;
   p->power  = 0;

   /* Set player arms level */
   p->armslevel = c->weapons->armslevel;

   /* Initialise AI */
   sc_ai_init_game(c, p);

}



void sc_player_init_round(sc_config *c, sc_player *p) {
/* sc_player_init_round
   Called once for each player at the beginning of any round. */

   /* Determine the tank's position on the field */
   p->field_index = game_lrand(c->numplayers);
   while(c->field_position[p->field_index] >= 0) {
      p->field_index = (p->field_index + 1) % c->numplayers;
   }
   c->field_position[p->field_index] = p->index;

   /* Initialise player X, Y coordinates -- we may need to modify
      the land a bit so the tank is initially on flat ground.  */
   p->x = rint((p->field_index + 1) * c->fieldwidth / (c->numplayers + 1.0));
   p->y = sc_land_avg_height_around(c->land, p->x, c->fieldheight, p->tank->radius);
   sc_land_level_around(c, c->land, p->x, p->tank->radius, p->y);

   /* Setup player turret and initial power */
   p->turret = game_lrand(181);
   p->power  = SC_PLAYER_MAX_POWER / 5;

   /* Set initial life status
      Scale from game life units to this tank's life units. */
   p->life = SC_PLAYER_MAX_LIFE * p->tank->hardness;
   p->dead = false;
   p->armed = false;
   p->killedby = -1;

   /* Setup fuel level */
   p->fuel = 0;

   /* Clear contact triggers */
   p->contacttriggers = false;

   /* Load weapons and accessories */
   p->selweapon = SC_WEAPON_DEFAULT(c->weapons);
   p->selshield = SC_ACCESSORY_DEFAULT(c->accessories);
   sc_weapon_free_chain(&p->weapons);
   sc_shield_free(&p->shield);

   /* Money is as it was at beginning of the round */
   p->oldmoney = p->money;

   /* Initialise AI */
   sc_ai_init_round(c, p);

}



void sc_player_init_turn(const sc_config *c, sc_player *p) {
/* sc_player_init_turn
   Called for the player at the beginning of their turn. */

   int life;

   /* Clear any land that got stuck on our profile (sanity check) */
   /* This sanity check must not be removed, as it is the only thing
      protecting tanks from dirt bombs that might PERMANENTLY lock the
      tank into position, as a consequence. */
   sc_land_clear_profile(c, c->land, p);

   /* make sure we aren't occupying someone else's space */
   if(!sc_player_passable(c, p, p->x, p->y)) {
      fprintf(stderr, "warning: Player %d is LOCKED, cannot resolve   ** This is a bug **\n", p->index);
      fprintf(stderr, "warning: ** If there are any levitating tanks, well, this is the cause.\n");
   } /* levitation check */

   /* make sure tanks aren't levitating */
   if(sc_player_passable(c, p, p->x, p->y - 1)) {
      fprintf(stderr, "warning: Player %d is LEVITATING, cannot resolve   ** This is a bug **\n", p->index);
      fprintf(stderr, "warning: ** If there are any levitating tanks, well, this is the cause.\n");
   } /* levitation check */
   if(sc_land_support(c, c->land, p->x, p->y, p->tank->radius, p->tank->shelfsize) != 0) {
      fprintf(stderr, "warning: Player %d is UNSTABLE, cannot resolve   ** This is a bug **\n", p->index);
      fprintf(stderr, "warning: ** If there are any levitating tanks, well, this is the cause.\n");
   } /* levitation check */
   
   /* Check tank damage; update firepower if needed */
   life = INT_ROUND_DIV(p->life, p->tank->hardness);
   if(p->power > life) p->power = life;
   if(p->power < 0) p->power = 0;

   /* Update shields for the new turn. */
   sc_shield_init_turn(p);

}



sc_player *sc_player_new(int index, const sc_tank_profile *tank) {
/* sc_player_new
   Create a new player */

   sc_player *p;

   /* Allocate player */
   p = (sc_player *)malloc(sizeof(sc_player));
   if(p == NULL) return(NULL);

   /* Initialize player name and ID number */
   sbprintf(p->name, sizeof(p->name), "Player %d", index + 1);
   p->name[SC_PLAYER_NAME_LENGTH - 1] = '\0';
   p->index  = index;
   p->aitype = SC_AI_HUMAN;
   p->tank   = tank;

   /* No weapons or accessories, by default */
   p->armed = false;
   p->weapons = NULL;
   p->shield = NULL;
   p->turret = 0;
   p->power  = 0;

   /* Setup AI state */
   p->ai = sc_ai_new();

   return(p);

}



void sc_player_free(sc_player **p) {
/* sc_player_free
   Remove a player from the game */

   if(p == NULL || *p == NULL) return;
   sc_weapon_free_chain(&(*p)->weapons);
   sc_shield_free(&(*p)->shield);
   sc_ai_free(&(*p)->ai);
   free(*p);
   *p = NULL;

}



void sc_player_advance_power(const sc_config *c, sc_player *p, int delta) {
/* sc_player_advance_power
   Modifies power output by the delta given. */

   int life;

   /* Sanity checks */
   if(c == NULL || p == NULL) return;
   
   /* Figure out current life */
   life = INT_ROUND_DIV(p->life, p->tank->hardness);

   /* Update power */
   p->power += delta;
   if(p->power > SC_PLAYER_MAX_POWER) p->power = SC_PLAYER_MAX_POWER;
   if(p->power > life) p->power = life;
   if(p->power < 0) p->power = 0;

   /* Update state */
   #if USE_NETWORK
      sc_net_client_send_orders(c, c->client, p->index);
   #endif
   sc_status_update(c->window, p);

}



void sc_player_advance_turret(const sc_config *c, sc_player *p, int delta) {
/* sc_player_advance_turret
   Adjusts turret angle by the delta given. */

   /* Sanity checks */
   if(c == NULL || p == NULL) return;

   /* Update turret angle */
   p->turret += delta;
   while(p->turret > 180) p->turret -= 180;
   while(p->turret < 0) p->turret += 180;

   /* Update state */
   #if USE_NETWORK
      sc_net_client_send_orders(c, c->client, p->index);
   #endif
   sc_status_update(c->window, p);

}



void sc_player_advance_weapon(const sc_config *c, sc_player *p, int delta) {
/* sc_player_advance_weapon
   Jump <delta> weapons forward (negative: jump back by so many weapons). */

   sc_weapon_info *info;

   /* Sanity checks */
   if(c == NULL || p == NULL) return;

   info = p->selweapon;
   if(info == NULL) {
      fprintf(stderr, "warning: player %i has selected invalid weapon, aborting search\n", p->index);
      return;
   }

   /* Cycle through the weapons */
   do {
      if(delta > 0) info = sc_weapon_next(c->weapons, info, SC_WEAPON_LIMIT_ALL | SC_WEAPON_SCAN_FORWARD);
      else          info = sc_weapon_next(c->weapons, info, SC_WEAPON_LIMIT_ALL | SC_WEAPON_SCAN_REVERSE);
      if(info == NULL) {
         fprintf(stderr, "warning: player %i has no weapons (at all!), aborting search\n", p->index);
         break;
      }
      sc_player_set_weapon(c, p, info);
   } while(info->inventories[p->index] <= 0);

   /* Update state */
   #if USE_NETWORK
      sc_net_client_send_orders(c, c->client, p->index);
   #endif
   sc_status_update(c->window, p);

}



void sc_player_advance_shield(const sc_config *c, sc_player *p, int flags) {
/* sc_player_advance_shield
   Jump to the next available shield.  */

   sc_accessory_info *info;
   bool sawone = false;

   /* Sanity checks */
   if(c == NULL || p == NULL) return;

   info = p->selshield;
   if(info == NULL) {
      fprintf(stderr, "warning: player %i has selected invalid shield, aborting search\n", p->index);
      return;
   }
   
   /* Always advance, unless CHECK_CUR is set */
   if(!(flags & SC_PLAYER_SHIELD_CHECK_CUR)) {
      /* If we're not on a shield, try to find one. */
      if(!SC_ACCESSORY_IS_SHIELD(info)) {
         info = sc_shield_find_best(c, p);
      } else {
         info = sc_accessory_next(c->accessories, info, SC_ACCESSORY_LIMIT_ALL);
      }
   }

   /* Cycle through the shields */
   while(info != NULL && (!sawone || info->ident != p->selshield->ident) && 
         (!SC_ACCESSORY_IS_SHIELD(info) || info->inventories[p->index] <= 0)) {
      info = sc_accessory_next(c->accessories, info, SC_ACCESSORY_LIMIT_ALL);
      sawone = true;
   }
   
   /* Were we successful? */
   if(info == NULL || !SC_ACCESSORY_IS_SHIELD(info) || info->inventories[p->index] <= 0) {
      /* No shields left; fall back to the default */
      info = SC_ACCESSORY_DEFAULT(c->accessories);
      if(info == NULL) {
         fprintf(stderr, "warning: no default accessory found! aborting...\n");
         return;
      }
   }

   /* Found a shield which we have in stock! */
   sc_player_set_shield(c, p, info);
   #if USE_NETWORK
      sc_net_client_send_orders(c, c->client, p->index);
   #endif
   sc_status_update(c->window, p);

}



void sc_player_set_power(const sc_config *c, sc_player *p, int power) {
/* sc_player_set_power */

   int life;

   /* Sanity checks */
   if(c == NULL || p == NULL) return;
   
   /* Figure out current life */
   life = INT_ROUND_DIV(p->life, p->tank->hardness);

   /* Update power; return if no change */
   if(power < 0) power = 0;
   if(power > SC_PLAYER_MAX_POWER) power = SC_PLAYER_MAX_POWER;
   if(power > life) power = life;
   if(p->power == power) return;
   p->power = power;

   /* Update state */
   #if USE_NETWORK
      sc_net_client_send_orders(c, c->client, p->index);
   #endif
   sc_status_update(c->window, p);

}



void sc_player_set_turret(const sc_config *c, sc_player *p, int turret) {
/* sc_player_set_turret */

   /* Sanity checks */
   if(c == NULL || p == NULL) return;

   /* Update turret angle; return if no change */
   while(turret < 0) turret += 180;
   while(turret > 180) turret -= 180;
   if(p->turret == turret) return;
   p->turret = turret;

   /* Update state */
   #if USE_NETWORK
      sc_net_client_send_orders(c, c->client, p->index);
   #endif
   sc_status_update(c->window, p);

}



void sc_player_set_weapon(const sc_config *c, sc_player *p, sc_weapon_info *info) {
/* sc_player_set_weapon */

   /* Sanity checks */
   if(c == NULL || p == NULL || info == NULL) return;

   /* Make sure index is sane, and player has that weapon */
   if(info->inventories[p->index] <= 0) return;
   p->selweapon = info;

   /* Update state */
   #if USE_NETWORK
      sc_net_client_send_orders(c, c->client, p->index);
   #endif
   sc_status_update(c->window, p);

}



void sc_player_set_shield(const sc_config *c, sc_player *p, sc_accessory_info *info) {
/* sc_player_set_shield */

   /* Sanity checks */
   if(c == NULL || p == NULL || info == NULL) return;

   /* Make sure info is a shield, and player has that shield */
   if(info->inventories[p->index] > 0 && SC_ACCESSORY_IS_SHIELD(info))
      p->selshield = info;
   else
      return;

   /* Update state */
   #if USE_NETWORK
      sc_net_client_send_orders(c, c->client, p->index);
   #endif
   sc_status_update(c->window, p);

}



bool sc_player_activate_shield(const sc_config *c, sc_player *p) {
/* sc_player_activate_shield
   Activates the currently selected player shield.  If unable to activate,
   then false is returned.  Any existing shield will be destroyed, even
   if fully powered; so use with caution.  The player shield will also
   be advanced if necessary.  */

   sc_accessory_info *info;

   /* Sanity check */
   if(c == NULL || p == NULL) return(false);
   
   /* Verify this is a shield */
   info = p->selshield;
   if(!SC_ACCESSORY_IS_SHIELD(info) || info->inventories[p->index] <= 0) return(false);

   /* Activate this shield, perhaps destroying any old shielding */
   sc_shield_free(&p->shield);
   p->shield = sc_shield_new(info);

   /* Update state */
   #if USE_NETWORK
      sc_net_client_send_shields(c, c->client, p->index);
   #endif

   /* Update inventory - Must happen AFTER send_shields! */
   --info->inventories[p->index];
   sc_player_advance_shield(c, p, SC_PLAYER_SHIELD_CHECK_CUR);

   sc_status_update(c->window, p);
   return(true);

}



bool sc_player_activate_best_shield(const sc_config *c, sc_player *p) {
/* sc_player_activate_best_shield
   Activates the best player shield available.  If unable to activate,
   then false is returned.  Any existing shield will be destroyed, even
   if fully powered; so use with caution. */

   /* Find the best shields available */
   sc_accessory_info *info = sc_shield_find_best(c, p);

   /* If successful, activate them! */
   if(info != NULL) {
      sc_player_set_shield(c, p, info);
      return(sc_player_activate_shield(c, p));
   }

   /* Failure */
   return(false);

}



bool sc_player_activate_auto_shield(const sc_config *c, sc_player *p) {
/* sc_player_activate_auto_shield
   Activate a shield via Auto Defense, on the fly, if we can. */

   /* Sanity check */
   if(c == NULL || p == NULL) return(false);

   /* Check for auto defense capability. */
   if(!(p->ac_state & SC_ACCESSORY_STATE_AUTO_DEF)) return(false);

   /* Try to activate the currently selected shield. */
   if(sc_player_activate_shield(c, p)) return(true);

   /* Otherwise try and activate the best shield available. */
   return(sc_player_activate_best_shield(c, p));

}



bool sc_player_activate_battery(const sc_config *c, sc_player *p) {
/* sc_player_activate_battery
   Activates a player battery, if any are available.  This will
   allow the player to partially recharge their tank.  */

   int count;        /* Iterator variable */
   int maxlife;      /* Max life, scaled to hardness */
   sc_accessory_info *info;

   /* Sanity check */
   if(c == NULL || p == NULL) return(false);
   maxlife = SC_PLAYER_MAX_LIFE * p->tank->hardness;
   if(p->life >= maxlife) return(false);  /* No recharge needed */

   /* Search for batteries in the inventory */
   count = sc_accessory_count(c->accessories, SC_ACCESSORY_LIMIT_ALL);
   info = sc_accessory_first(c->accessories, SC_ACCESSORY_LIMIT_ALL);
   for(; count > 0; --count) {
      if(SC_ACCESSORY_IS_BATTERY(info) && info->inventories[p->index] > 0) {
         /* Activate this battery */
         --info->inventories[p->index];
         p->life += SC_BATTERY_RECHARGE_PERCT * maxlife / 100;
         if(p->life > maxlife) p->life = maxlife;
         /* Update state */
         #if USE_NETWORK
            sc_net_client_send_battery(c, c->client, p->index);
         #endif
         sc_status_update(c->window, p);
         return(true);
      }
      info = sc_accessory_next(c->accessories, info, SC_ACCESSORY_LIMIT_ALL);
   }

   /* Failure, sorry */
   return(false);

}



int sc_player_battery_count(const sc_config *c, const sc_player *p) {
/* sc_player_battery_count
   Determine the number of batteries in the player's inventory.  */

   int count;        /* Iterator variable */
   sc_accessory_info *info;

   /* Sanity check */
   if(c == NULL || p == NULL) return(false);

   /* Search a battery in inventory */
   count = sc_accessory_count(c->accessories, SC_ACCESSORY_LIMIT_ALL);
   info = sc_accessory_first(c->accessories, SC_ACCESSORY_LIMIT_ALL);
   for(; count > 0; --count) {
      if(SC_ACCESSORY_IS_BATTERY(info) && info->inventories[p->index] > 0) {
         return(info->inventories[p->index]);
      }
      info = sc_accessory_next(c->accessories, info, SC_ACCESSORY_LIMIT_ALL);
   }

   /* Failure, sorry */
   return(0);

}



void sc_player_set_contact_triggers(const sc_config *c, sc_player *p, bool flag) {
/* sc_player_set_contact_triggers
   Toggles the contact-trigger flag automagically.
   Won't toggle it unless the player has contact triggers. */

   /* Sanity check */
   assert(c != NULL);
   assert(p != NULL);

   /* Update the flag */
   if(flag && (p->ac_state & SC_ACCESSORY_STATE_TRIGGER)) {
      p->contacttriggers = true;
   } else {
      p->contacttriggers = false;
   }

   /* Make sure player flags are transmitted over network */
   #if USE_NETWORK
      sc_net_client_send_flags(c, c->client, p->index);
   #endif

   /* Update status window. */
   sc_status_update(c->window, p);

}



void sc_player_toggle_contact_triggers(const sc_config *c, sc_player *p) {
/* sc_player_toggle_contact_triggers
   Toggle the "use contact triggers" flag for this player.  */

   assert(p != NULL);
   sc_player_set_contact_triggers(c, p, !p->contacttriggers);

}



bool sc_player_use_contact_trigger(const sc_config *c, sc_player *p) {
/* sc_player_use_contact_trigger
   Try to use a contact trigger.  If successful, return true. */

   sc_accessory_info *info, *cons = NULL, *perm = NULL;
   int count, totrig = 0;

   /* Sanity check */
   if(c == NULL || p == NULL) return(false);

   /* Are they enabled? */
   if(!p->contacttriggers) return(false);

   /* Check to see if we have any contact triggers to use. */
   count = sc_accessory_count(c->accessories, SC_ACCESSORY_LIMIT_ALL);
   info = sc_accessory_first(c->accessories, SC_ACCESSORY_LIMIT_ALL);
   for(; count > 0; --count) {
      /* Make sure the damn thing is a batch-o-contact-triggers */
      if(SC_ACCESSORY_IS_TRIGGER(info) && info->inventories[p->index] > 0) {
         ++totrig;
         if(SC_ACCESSORY_IS_CONSUMABLE(info))
            cons = info;
         if(SC_ACCESSORY_IS_PERMANENT(info))
            perm = info;
      }
      info = sc_accessory_next(c->accessories, info, SC_ACCESSORY_LIMIT_ALL);
   }

   /* Use permanent triggers if possible; otherwise try consumable ones. */
   if(perm != NULL) {
      /* Make use of permanent (distance?) trigger. */
      return(true);
   } else if(cons != NULL) {
      /* Consume a consumable (mechanical?) trigger. */
      --cons->inventories[p->index];
      if(totrig <= 1 && info->inventories[p->index] <= 0) {
         p->ac_state &= ~SC_ACCESSORY_STATE_TRIGGER;
         sc_player_set_contact_triggers(c, p, false);
      }
      return(true);
   } else {
      /* Failure; sorry. */
      return(false);
   }

}



int sc_player_contact_trigger_count(const sc_config *c, const sc_player *p) {
/* sc_player_contact_trigger_count
   Return the number of contact triggers in this player's inventory. */

   sc_accessory_info *info, *cons = NULL, *perm = NULL;
   int count;

   /* Sanity check */
   if(c == NULL || p == NULL) return(false);

   /* Check to see if we have any contact triggers to use. */
   count = sc_accessory_count(c->accessories, SC_ACCESSORY_LIMIT_ALL);
   info = sc_accessory_first(c->accessories, SC_ACCESSORY_LIMIT_ALL);
   for(; count > 0; --count) {
      /* Make sure the damn thing is a batch-o-contact-triggers */
      if(SC_ACCESSORY_IS_TRIGGER(info) && info->inventories[p->index] > 0) {
         if(SC_ACCESSORY_IS_CONSUMABLE(info)) cons = info;
         if(SC_ACCESSORY_IS_PERMANENT(info)) perm = info;
      }
      info = sc_accessory_next(c->accessories, info, SC_ACCESSORY_LIMIT_ALL);
   }

   /* Use permanent triggers if possible; otherwise try consumable ones. */
   if(perm != NULL) {
      /* Make use of permanent (distance?) trigger. */
      return(perm->inventories[p->index]);
   } else if(cons != NULL) {
      /* Consume a consumable (mechanical?) trigger. */
      return(cons->inventories[p->index]);
   } else {
      /* Failure; sorry. */
      return(0);
   }

}



void sc_player_set_position(const sc_config *c, sc_player *p, int x, int y) {
/* sc_player_set_position
   The coordinate (x, y) indicates the new player position, in virtual
   coordinates.  This function always succeeds and performs no sanity
   checks (i.e. to make sure player can actually OCCUPY said position) */

   /* Sanity checks */
   if(c == NULL || p == NULL) return;

   /* Make sure tank has actually moved */
   if(p->x == x && p->y == y) return;

   /* Update tank position on screen */
   sc_window_undraw_tank(c->window, p);
   p->x = x;
   p->y = y;
   sc_window_draw_tank(c->window, p);

   /* Transmit state by network */
   #if USE_NETWORK
      sc_net_client_send_orders(c, c->client, p->index);
   #endif /* Network? */

}



void sc_player_inc_wins(sc_config *c, sc_player *p) {
/* sc_player_inc_wins */

   ++p->numwins;
   p->money += c->economics->survivalbonus;

}



void sc_player_died(sc_config *c, sc_player *p) {
/* sc_player_died */

   p->dead = true;
   p->money -= c->economics->deathloss;

}



static bool _sc_player_drop(sc_config *c, sc_player *p, int falldist) {
/* sc_player_drop
   Drop current player; return nonzero if still falling */

   int deltax;
   int dropdist;

   if(falldist <= 0) return(true);

   if(SC_CONFIG_NO_ANIM(c)) dropdist = c->fieldheight;
   else dropdist = falldist;

   /* Note: this function needs to use the dead flag; if the player is
      technically lifeless but the tank has not been removed from the
      playfield yet, then the tank should still take a plunge.  */
   if(!p->dead) {
      /* Get maximum height of land around and just below the tank */
      falldist = 0;
      while(falldist < dropdist && sc_player_passable(c, p, p->x, p->y - 1)) {
         /* Still falling vertically down */
         if(falldist == 0) sc_window_undraw_tank(c->window, p);
         ++falldist;
         --p->y;
      } /* Falling tank vertically down */
      if(falldist > 0) {
         sc_window_draw_tank(c->window, p);
         _sc_player_drop(c, p, dropdist - falldist);
         /* Assume need recurse, a tank may have been using us for support */
         return(true);
      } /* Haven't landed yet */

      /* We've landed */
      /* Might be sliding on the slope at this point */
      deltax = sc_land_support(c, c->land, p->x, p->y, p->tank->radius, p->tank->shelfsize);
      if(deltax != 0 && sc_player_passable(c, p, p->x + deltax, p->y - 1)) {

         /* We slid to one side (deltax > 0, slide right) */
         sc_window_undraw_tank(c->window, p);
         p->x += deltax;
         --p->y;
         sc_window_draw_tank(c->window, p);
         falldist = dropdist - 4 * abs(deltax);
         _sc_player_drop(c, p, falldist);
         /* Assume need recurse, a tank may have been using us for support */
         return(true);
      } /* Tank slid on a steep slope */
   } /* Tank isn't dead */

   /* Tank has settled onto the ground */
   return(false);

}



bool sc_player_drop_all(sc_config *c) {
/* sc_player_drop_all
   Drop all players */

   bool needsrecurse;
   int i;

   needsrecurse = 0;
   i = c->numplayers - 1;
   while(i >= 0) {
      /* Make sure _sc_player_drop is FIRST, so short-circuiting does
         not kill the function call (which has side effects).  */
      needsrecurse = _sc_player_drop(c, c->players[i], SC_TANK_MAX_DROP_PER_CYCLE) || needsrecurse;
      --i;
   }

   return(needsrecurse);

}



static void _sc_player_damage(sc_config *c, sc_player *p, const sc_explosion *e) {
/* sc_player_damage */

   int damage;

   /* If player is already dead, then there's nothing to do */
   if(!SC_PLAYER_IS_ALIVE(p)) return;

   /* How much damage was taken? */
   damage = sc_expl_damage_at_point(c->land, e, p->x, p->y);
   if(damage <= 0) return;

   /* Check if the shields absorbed the explosion for us. */
   damage = sc_shield_absorb_explosion(p, e, damage);
   if(damage <= 0) return;

   /* Take any remaining damage ourselves */
   p->life -= damage * SC_TANK_NORMAL_HARDNESS;

   /* Revive shields if necessary (and if auto-defense enabled) */
   if(SC_PLAYER_IS_ALIVE(p)) {
      sc_player_activate_auto_shield(c, p);
   }

   /* Tank took a direct or partial hit.  Damn. */
   p->money -= c->economics->damageloss;
   if(!SC_PLAYER_IS_ALIVE(p)) {
      p->life = 0;  /* We died */
      p->killedby = e->playerid;
      /* Who killed us? */
      if(e->playerid == p->index) {
         /* It was a suicide */
         ++c->players[e->playerid]->suicides;
         p->money -= c->economics->suicideloss;
      } else {
         /* Give opponent some points for killing us */
         ++c->players[e->playerid]->kills;
         c->players[e->playerid]->money += c->economics->killbonus;
      }
   }

   if(e->playerid != p->index) {
      /* Give points to other player for damaging us */
      c->players[e->playerid]->money += c->economics->damagebonus;
   } /* Make sure not self */

}



void sc_player_damage_all(sc_config *c, const sc_explosion *e) {
/* sc_player_damage_all */

   int i;

   for(i = c->numplayers - 1; i >= 0; --i) {
      _sc_player_damage(c, c->players[i], e);
   }

}



sc_player **sc_player_random_order(sc_config *c, sc_player **playerlist) {
/* sc_player_random_order */

   int order[SC_MAX_PLAYERS];
   int i;
   int j;

   for(i = 0; i < c->numplayers; ++i) {
      order[i] = i;
   }

   for(i = 0; i < c->numplayers; ++i) {
      for(j = 0; j < c->numplayers; ++j) {
         if(i != j && game_lrand(100) < 50) {
            order[i] += order[j];
            order[j] = order[i] - order[j];
            order[i] = order[i] - order[j];
         }
      }
   }

   for(i = 0; i < c->numplayers; ++i) {
      playerlist[i] = c->players[order[i]];
   }

   return(playerlist);

}



sc_player **sc_player_winner_order(sc_config *c, sc_player **playerlist) {
/* sc_player_winner_order */

   sc_player *tmp;
   int i;
   int j;

   sc_player_random_order(c, playerlist);

   for(i = 1; i < c->numplayers; ++i) {
      for(j = 0; j < i; ++j) {
         if(playerlist[i]->kills - playerlist[i]->suicides > playerlist[j]->kills - playerlist[j]->suicides) {
            tmp = playerlist[i];
            playerlist[i] = playerlist[j];
            playerlist[j] = tmp;
         }
      }
   }

   return(playerlist);

}



sc_player **sc_player_loser_order(sc_config *c, sc_player **playerlist) {
/* sc_player_loser_order */

   sc_player *tmp;
   int i;
   int j;

   sc_player_random_order(c, playerlist);

   for(i = 1; i < c->numplayers; ++i) {
      for(j = 0; j < i; ++j) {
         if(playerlist[i]->kills - playerlist[i]->suicides < playerlist[j]->kills - playerlist[j]->suicides) {
            tmp = playerlist[i];
            playerlist[i] = playerlist[j];
            playerlist[j] = tmp;
         }
      }
   }

   return(playerlist);

}



int sc_player_total_fuel(const sc_accessory_config *ac, const sc_player *p) {
/* sc_player_total_fuel
   Used to discover how much fuel a player has, for the movement menu. */

   const sc_accessory_info *info;
   int count;
   int fuel = 0;

   if(p == NULL) return(0);

   /* This translates this tank's tank fuel rate to the game fuel rate. */
   fuel = INT_ROUND_DIV(p->fuel, p->tank->efficiency);

   /* Find unopened fuel in the inventory. */
   count = sc_accessory_count(ac, SC_ACCESSORY_LIMIT_ALL);
   info = sc_accessory_first(ac, SC_ACCESSORY_LIMIT_ALL);
   for(; count > 0; --count) {
      if(SC_ACCESSORY_IS_FUEL(info)) {
         fuel += info->fuel * info->inventories[p->index];
      }
      info = sc_accessory_next(ac, info, SC_ACCESSORY_LIMIT_ALL);
   }

   return(fuel);

}



static bool _sc_player_consume_fuel(const sc_accessory_config *ac, sc_player *p) {
/* _sc_player_consume_fuel
   Used to eat up fuel, based on tank type and such. */

   sc_accessory_info *info;
   int count;

   if(p == NULL) return(false);

   /* We are basically looking to move one pixel at this point.
      Thus, we bring the fuel up to at least one unit positive.
      If we actually have enough, that is. */
   count = sc_accessory_count(ac, SC_ACCESSORY_LIMIT_ALL);
   info = sc_accessory_first(ac, SC_ACCESSORY_LIMIT_ALL);
   for(; count > 0; --count) {
      if(p->fuel >= SC_TANK_NORMAL_EFFICIENCY) break;
      if(SC_ACCESSORY_IS_FUEL(info)) {
         while(info->inventories[p->index] > 0 && p->fuel < SC_TANK_NORMAL_EFFICIENCY) {
            /* Scale game fuel to this tank's fuel rate. */
            p->fuel += info->fuel * p->tank->efficiency;
            --info->inventories[p->index];
         }
      }
      info = sc_accessory_next(ac, info, SC_ACCESSORY_LIMIT_ALL);
   }

   /* Actually consume some fuel if we can. */
   if(p->fuel < SC_TANK_NORMAL_EFFICIENCY) return(false);
   p->fuel -= SC_TANK_NORMAL_EFFICIENCY;
   return(true);

}



bool sc_player_move(const sc_config *c, sc_player *p, int delta) {
/* sc_player_move
   Move the player's tank by means of consuming fuel cells.  This call
   is only used when moving by using fuel, and should not be used to
   set player position by ``forces''.  If the player does not have any
   fuel, then we will give up and not attempt the move at all.  */

   int y;      /* Height of the land at the destination */

   /* Sanity checks */
   if(c == NULL || p == NULL || delta == 0) return(false);

   /* Check that the player is in a mobile tank. */
   if(!p->tank->mobile) return(false);

   /* Check that the height is not too high a climb for a heavy tank. */
   y = sc_land_height_around(c->land, p->x + delta, c->fieldheight, p->tank->radius);
   if(y > p->y + SC_TANK_CLIMB_HEIGHT) return(false);

   /* Check that the land is otherwise ``passable'', i.e. no walls
      or other tanks in our way.  */
   if(!sc_player_passable(c, p, p->x + delta, y)) return(false);

   /* Attempt to consume fuel. If this succeeds then the move goes thru */
   if(!_sc_player_consume_fuel(c->accessories, p)) return(false);
   sc_player_set_position(c, p, p->x + delta, y);
   sc_status_update(c->window, p);
   return(true);

}



int sc_player_turret_x(const sc_player *p, int angle) {
/* sc_player_turret_x */

   return(p->x + (p->tank->turretradius + 1) * cos(angle * M_PI / 180));

}



int sc_player_turret_y(const sc_player *p, int angle) {
/* sc_player_turret_y */

   return(p->y + (p->tank->turretradius + 1) * sin(angle * M_PI / 180));

}



void sc_player_death(const sc_config *c, const sc_player *p, sc_explosion **e) {
/* sc_player_death
   Nice knowin' ya... Note, this function may add new explosions to the
   queue e that is passed in.  This function does not (directly) modify
   the player state.  */

   sc_explosion *expl;

   assert(c != NULL && p != NULL && e != NULL);

   switch(game_lrand(10)) {

      case 0:
         /* Just say it with napalm ... */
         expl = sc_expl_new(p->x, p->y, c->weapons->scaling * SC_WEAPON_NAPALM_RADIUS,
                            SC_WEAPON_NAPALM_FORCE, p->killedby, SC_EXPLOSION_NAPALM);
         expl->data = sc_spill_new(c, c->land, SC_WEAPON_NAPALM_LIQUID, expl->centerx, expl->centery);
         sc_expl_add(e, expl);
         break;

      default:
         /* Just the usual, 3-stage detonation */
         sc_expl_add(e, sc_expl_new(p->x, p->y, c->weapons->scaling * SC_WEAPON_SMALL_EXPLOSION, 
                                    SC_WEAPON_SMALL_FORCE, p->killedby, SC_EXPLOSION_NORMAL));
         if(game_drand() < 0.5) {
            sc_expl_add(e, sc_expl_new(p->x, p->y, c->weapons->scaling * SC_WEAPON_MEDIUM_EXPLOSION, 
                                       SC_WEAPON_MEDIUM_FORCE, p->killedby, SC_EXPLOSION_NORMAL));
            if(game_drand() < 0.1) {
               sc_expl_add(e, sc_expl_new(p->x, p->y, c->weapons->scaling * SC_WEAPON_LARGE_EXPLOSION, 
                                          SC_WEAPON_LARGE_FORCE, p->killedby, SC_EXPLOSION_PLASMA));
            } /* Do third stage? */
         } /* Do second stage? */
         break;

   } /* End of switch */

}



bool sc_player_passable(const sc_config *c, const sc_player *p, int x, int y) {
/* sc_player_passable
   Returns true if the player can be placed at the virtual coordinate 
   indicated.  (x, y) represents the new `center' of the tank.  To do
   this we actually check the tank profile to make sure the land is
   passable everywhere where the tank profile is opaque. */

   const unsigned char *data;
   int radius;
   int cx;
   int cy;

   if(c == NULL || p == NULL || p->tank == NULL || p->tank->data == NULL) return(false); 

   data = p->tank->data;
   radius = p->tank->radius;
   for(cy = radius; cy >= 0; --cy) {
      for(cx = radius; cx >= -radius; --cx, ++data) {
         if(*data != SC_TANK_PROFILE_CLEAR) {
            if(!sc_land_passable_point(c, p, x + cx, y + cy)) return(false);
         }
      }
   }

   return(true);

}



bool sc_player_would_impact(const sc_config *c, const sc_player *p, int x, int y) {
/* sc_player_would_impact
   Determines whether the x, y coordinate indicated is atop the player,
   i.e. a weapon at that coordinate would actually hit the player.  The
   coordinate is a virtual coordinate; returns true if an impact would
   occur. */

   const unsigned char *data;
   int radius;
   int dx;
   int dy;
   
   /* Sanity checks */
   if(c == NULL || p == NULL || p->tank == NULL || p->tank->data == NULL) return(false);
   
   /* Determine where in the player tank profile we landed */
   radius = p->tank->radius;
   if(!sc_land_calculate_deltas(c->land, &dx, &dy, p->x, p->y, x, y)) return(false);
   if(dx < -radius || dx > radius) return(false);
   if(dy < 0 || dy > radius) return(false);

   /* We're in the bounding box for the profile; determine if we hit */
   data = p->tank->data + (radius - dy) * (radius + radius + 1) + (dx + radius);
   return(*data != SC_TANK_PROFILE_CLEAR);

}



#define  SC_PLAYER_NUM_MESSAGES  36
static const char *_sc_player_messages[SC_PLAYER_NUM_MESSAGES + 1] = {
   "In times of trouble, go with what you know.",
   "Die!",
   "You're toast!",
   "Banzai!",
   "From Hell's heart I stab at thee...",
   "I didn't do it.  Nobody saw me do it.",
   "Make my day.",
   "Charge!",
   "Attack!",
   "You're outta here.",
   "Freeze, or I'll shoot!",
   "Ha ha ha.",
   "We come in peace - Shoot to kill!",
   "In your face!",
   "I love the smell of Napalm in the morning.",
   "Victory!",
   "Show some respect.",
   "Just who do you think you are?",
   "Look out below!",
   "Knock, Knock.",
   "Look over there.",
   "Guess what's coming for dinner?",
   "Merry Christmas.",
   "Open wide!",
   "Here goes nothing...",
   "Don't worry, it isn't a live round.",
   "I wonder what this button does?",
   "Don't take this personally.",
   "Would this make you mad?",
   "I could spare you, but why?",
   "My bomb is bigger than yours.",
   "Don't forget about me!",
   "Take this!",
   "This screen ain't big enough for the both of us.",
   "Say \"Arrgghhhhh....\"",
   "I shall oil my turret with your blood.",
   NULL
};   



#define  SC_PLAYER_NUM_DEATH_MESSAGES  29
static const char *_sc_player_death_messages[SC_PLAYER_NUM_DEATH_MESSAGES + 1] = {
   "Aargh!",
   "I hate it when that happens.",
   "One direct hit can ruin your whole day.",
   "Ouch.",
   "Oh no, not again.",
   "Another one bites the dust.",
   "Goodbye.",
   "Farewell, cruel world.",
   "Another day, another bomb.",
   "Why does everything happen to me?",
   "I've got a bad feeling about this.",
   "What was that noise?",
   "Mama said there'd be days like this.",
   "Its just one of those days...",
   "I see a bright light...",
   "I let you hit me!",
   "I didn't want to live anyway.",
   "Was that as close as I think it was?",
   "Join the army, see the world they said.",
   "I thought you liked me?",
   "Such senseless violence!  I don't understand it.",
   "I think this guy's a little crazy.",
   "Somehow I don't feel like killing anymore.",
   "Gee... thanks.",
   "I've fallen and I can't get up!",
   "I'll be back...",
   "Hey - I've got lawyers.",
   "Time to call 1-900-SUE-TANK.",
   "~But oh, God, under the weight of life / Things seem so much brighter on the other side~",
   NULL
};



const char *sc_player_talk(const sc_config *c, const sc_player *p) {

   switch(c->options.talk) {
      case SC_CONFIG_TALK_OFF:
         break;
      case SC_CONFIG_TALK_COMPUTERS:
         if(p->aitype == SC_AI_HUMAN) break;
      default:
         if(game_lrand(100) < (dword)c->options.talkprob) {
            return(_sc_player_messages[game_lrand(SC_PLAYER_NUM_MESSAGES)]);
         }
   }

   return(NULL);

}



const char *sc_player_death_talk(const sc_config *c, const sc_player *p) {

   switch(c->options.talk) {
      case SC_CONFIG_TALK_OFF:
         break;
      case SC_CONFIG_TALK_COMPUTERS:
         if(p->aitype == SC_AI_HUMAN) break;
      default:
         if(game_lrand(100) < (dword)c->options.talkprob) {
            return(_sc_player_death_messages[game_lrand(SC_PLAYER_NUM_DEATH_MESSAGES)]);
         }
   }

   return(NULL);

}
