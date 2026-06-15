/* $Header: /fridge/cvs/xscorch/sgame/sweapon.c,v 1.75 2011-08-01 00:01:42 jacob Exp $ */
/*

   xscorch - sweapon.c        Copyright(c) 2000-2004 Jacob Luna Lundberg
                              Copyright(c) 2000-2003 Justin David Smith
   jacob(at)gnifty.net        http://www.gnifty.net/
   justins(at)chaos2.org      http://chaos2.org/

   Scorched basic weapon system


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

#include <sweapon.h>
#include <saddconf.h>
#include <scolor.h>
#include <sconfig.h>
#include <sexplosion.h>
#include <sinventory.h>
#include <sland.h>
#include <sphoenix.h>
#include <sphysics.h>
#include <splayer.h>
#include <sregistry.h>
#include <sshield.h>
#include <sspill.h>
#include <swindow.h>

#include <libj/jstr/libjstr.h>



/***  Weapon Information    ***/



typedef struct _sc_weapon_test_arg {
   int counter;
   int flags;
   const char *name;
   const sc_weapon_config *wc;
} sc_weapon_test_arg;



static inline bool _sc_weapon_viewable(const sc_weapon_config *wc, const sc_weapon_info *info, int flags) {
/* _sc_weapon_viewable
   We return true if the weapon is viewable under the given rules. */

   if( (!(flags & SC_WEAPON_LIMIT_ARMS)     || info->armslevel <= wc->armslevel)   &&
       (!(flags & SC_WEAPON_LIMIT_USELESS)  || wc->uselessitems || !info->useless) &&
       (!(flags & SC_WEAPON_LIMIT_INDIRECT) || !info->indirect)                    ) {
      return(true);
   } else {
      return(false);
   }

}



static bool _sc_weapon_test_count(void *data, long arg) {
/* _sc_weapon_test_count
   This is an sc_registry_test function.
   We will iterate, incrementing a counter, but always returning false. */

   sc_weapon_info *info    = (sc_weapon_info *)data;
   sc_weapon_test_arg *wta = (sc_weapon_test_arg *)arg;

   /* We don't validate args; please do so in the caller! */

   const sc_weapon_config *wc = wta->wc;
   int flags = wta->flags;

   if(_sc_weapon_viewable(wc, info, flags)) ++wta->counter;
   return(false);

}


int sc_weapon_count(const sc_weapon_config *wc, int flags) {
/* sc_weapon_count
   Counts the number of weapons that have been registered with the game's data registry.
   This is optimized but of course if you're calling it a lot you should cache the data.
   The only exception is that the count may change if a new weapon list file is appended. */

   sc_registry_iter *iter;
   sc_weapon_test_arg wta;

   if(wc == NULL) return(0);

   /* Set up for iteration. */
   wta.counter = 0;
   wta.flags   = flags;
   wta.wc      = wc;
   iter = sc_registry_iter_new(wc->registry, wc->registryclass, SC_REGISTRY_FORWARD,
                               _sc_weapon_test_count, (long)(&wta));
   if(iter == NULL) return(0);

   /* Iterate using the fast registry iterators. */
   sc_registry_iterate(iter);

   /* Clean up. */
   sc_registry_iter_free(&iter);

   return(wta.counter);

}



sc_weapon_info *sc_weapon_lookup(const sc_weapon_config *wc, int id, int flags) {
/* sc_weapon_lookup
   Pass along a registry request for the weapon. */

   sc_weapon_info *info;

   if(wc == NULL || id < 0) return(NULL);

   /* Find the weapon in the registry. */
   info = (sc_weapon_info *)sc_registry_find_by_int(wc->registry, id);

   /* Verify that the rules in place allow viewing the weapon. */
   if(info != NULL && _sc_weapon_viewable(wc, info, flags))
      return(info);
   else
      return(NULL);

}



static bool _sc_weapon_test_lookup_name(void *data, long arg) {
/* _sc_weapon_test_lookup_name
   This is an sc_registry_test function.
   We will seek, search, discover the weapon by name. */

   sc_weapon_info *info    = (sc_weapon_info *)data;
   sc_weapon_test_arg *wta = (sc_weapon_test_arg *)arg;

   /* We don't validate args; please do so in the caller! */

   const char *name = wta->name;

   /* Use sloppy string comparison on the name (true if similar). */
   return(strsimilar(info->name, name));

}


sc_weapon_info *sc_weapon_lookup_by_name(const sc_weapon_config *wc, const char *name, int flags) {
/* sc_weapon_lookup_by_name
   Tries to find a weapon by roughly the requested name.
   This is much slower than sc_weapon_lookup. */

   sc_registry_iter *iter;
   sc_weapon_test_arg wta;
   sc_weapon_info *info;

   if(wc == NULL || name == NULL) return(NULL);

   /* Set up for iteration. */
   wta.name = name;
   iter = sc_registry_iter_new(wc->registry, wc->registryclass,
                               (flags & SC_WEAPON_SCAN_REVERSE) ? SC_REGISTRY_REVERSE : SC_REGISTRY_FORWARD,
                               _sc_weapon_test_lookup_name, (long)(&wta));
   if(iter == NULL) return(NULL);

   /* Iterate using the fast registry iterators. */
   info = (sc_weapon_info *)sc_registry_iterate(iter);

   /* Clean up. */
   sc_registry_iter_free(&iter);

   /* Verify that the rules in place allow viewing the weapon. */
   if(info != NULL && _sc_weapon_viewable(wc, info, flags))
      return(info);
   else
      return(NULL);

}



static bool _sc_weapon_test_viewable(void *data, long arg) {
/* _sc_weapon_test_viewable
   This is an sc_registry_test function.
   We will return true for the first viewable weapon. */

   sc_weapon_info *info    = (sc_weapon_info *)data;
   sc_weapon_test_arg *wta = (sc_weapon_test_arg *)arg;

   /* We don't validate args; please do so in the caller! */

   const sc_weapon_config *wc = wta->wc;
   int flags = wta->flags;

   return(_sc_weapon_viewable(wc, info, flags));

}


sc_weapon_info *sc_weapon_first(const sc_weapon_config *wc, int flags) {
/* sc_weapon_first
   Return the first/last viable weapon in the weapon config space. */

   sc_weapon_test_arg wta;
   sc_weapon_info *info;

   if(wc == NULL) return(NULL);

   wta.wc    = wc;
   wta.flags = flags;

   /* Try and find the first/last weapon that meets our criteria. */
   info = sc_registry_find_first(wc->registry, wc->registryclass,
                                 (flags & SC_WEAPON_SCAN_REVERSE) ? SC_REGISTRY_REVERSE : SC_REGISTRY_FORWARD,
                                 _sc_weapon_test_viewable, (long)(&wta));

   return(info);

}


sc_weapon_info *sc_weapon_next(const sc_weapon_config *wc, const sc_weapon_info *info, int flags) {
/* sc_weapon_next
   Advance to the next/prev weapon in the list (with wrapping).  */

   sc_weapon_test_arg wta;
   sc_weapon_info *nxtnfo;

   if(wc == NULL || info == NULL) return(NULL);

   /* Set up the flags and args for the mission. */
   wta.wc    = wc;
   wta.flags = flags;

   /* Try and find the first/last weapon that meets our criteria. */
   nxtnfo = sc_registry_find_next(wc->registry, wc->registryclass, info->ident,
                                  (flags & SC_WEAPON_SCAN_REVERSE) ? SC_REGISTRY_REVERSE : SC_REGISTRY_FORWARD,
                                  _sc_weapon_test_viewable, (long)(&wta));

   /* In case we iterated off the end of the list, wrap to the beginning. */
   if(nxtnfo == NULL)
      nxtnfo = sc_weapon_first(wc, flags);

   return(nxtnfo);

}



/***  Weapon Statistics  ***/



static double _sc_weapon_stat_yield(const sc_weapon_config *wc, const sc_weapon_info *info, bool triple) {
/* _sc_weapon_stat_yield
   Find an upper bound yield for a weapon.  */

   double yield;                 /* Total weapon yield */
   const sc_weapon_info *child;  /* Info on child weapon(s) */
   int numchildren;              /* Number of children for phoenix weapon */
   int count;                    /* A counter for frog type phoenix weapons */

   /* Yield from self */
   yield = SQR(info->radius) * info->force;

   /* If liquid (napalm), must multiply by potential area of flooding
      -- this should scale hot napalm to near the vicinity of a nuke
      (but less), which is reasonable, considering both can take down
      large tanks.  */
   if(SC_WEAPON_IS_LIQUID(info)) yield *= info->liquid;

   /* Triple Turret weapons get three times the kaboom, but only if we have such a turret. */
   if(triple && SC_WEAPON_IS_TRIPLE(info))
      yield *= 3;

   /* Phoenix weapons have special yield rules. */
   if(SC_WEAPON_IS_PHOENIX(info)) {
      if(!sc_phoenix_verify(wc, info)) {
         /* This means somebody snuck in a bad child pointer on a
            phoenix weapon.  It shouldn't happen, but if it does, we
            promise to return a negative yield.  AIs are advised to
            select another weapon as this one may cause the game to
            segfault or die and nobody wants that! */
         fprintf(stderr, "Bad phoenix weapon: %s.  Children loop or do not exist.\n", info->name);
         return(yield ? -abs(yield) : -1);
      }
      child = sc_weapon_lookup(wc, SC_PHOENIX_CHILD_TYPE(info), SC_WEAPON_LIMIT_NONE);
      numchildren = SC_PHOENIX_CHILD_COUNT(info);
      if(SC_PHOENIX_IS_CHAIN(info)) {
         /* Each child is made smaller by a constant factor, _including_ the first child */
         for(count = 1; count <= numchildren; ++count)
            yield += _sc_weapon_stat_yield(wc, child, triple) * pow(SC_PHOENIX_INCREASE_FACTOR, count);
      } else if(SC_PHOENIX_IS_CONVERT(info)) {
         /* conversions probably aren't well represented here */
         if(child != info) yield = _sc_weapon_stat_yield(wc, child, triple);
      } else if(SC_PHOENIX_IS_SCATTER(info)) {
         /* scattering doesn't detonate the original weapon */
         if(child != info) yield = numchildren * _sc_weapon_stat_yield(wc, child, triple);
         else              yield = numchildren * yield;
         /* as a policy, scattering takes a slight bonus for "effectiveness" */
         yield *= (SC_PHOENIX_YIELD_SCATTER_SCALE + SC_PHOENIX_SCATTER(info)) / SC_PHOENIX_YIELD_SCATTER_SCALE;
      } else if(SC_PHOENIX_IS_SPIDER(info)) {
         /* spiders detonate the original weapon in the center, and on average 75% of their children */
         if(child != info) yield += 0.75 * numchildren * _sc_weapon_stat_yield(wc, child, triple);
         else              yield += 0.75 * numchildren * yield;
      } else if(SC_PHOENIX_IS_DETONATE(info)) {
         /* We do nothing with detonation requests; they have their face value yield. */
      }
   }

   return(yield);

}



static double _sc_weapon_stat_precision(const sc_weapon_config *wc, const sc_weapon_info *info, bool triple) {
/* _sc_weapon_stat_precision
   Find a rough estimator of a weapon's precision.
   Later on we might have a wind shear value, and/or a
   weapon eccentricity value, which would also contribute.
   During most of this code precision is actually inverted... */

   double precision;             /* Estimate of weapon precision */
   const sc_weapon_info *child;  /* Info on child weapon(s) */
   int numchildren;              /* Number of children for phoenix weapon */
   int count;                    /* A counter for frog type phoenix weapons */

   /* Horizonal (i.e. not area) space covered, usually 2 * radius.
      If liquid (napalm), the area of flooding is the horizonal space. */
   if(SC_WEAPON_IS_LIQUID(info))
      precision = info->liquid;
   else
      precision = 2 * info->radius;

   /* Triple Turret weapons are a third as precise, if we have such a turret. */
   if(triple && SC_WEAPON_IS_TRIPLE(info))
      precision *= 2;

   /* Phoenix weapons have special precision rules. */
   if(SC_WEAPON_IS_PHOENIX(info)) {
      if(!sc_phoenix_verify(wc, info)) {
         /* This means somebody snuck in a bad child pointer on a
            phoenix weapon.  It shouldn't happen, but if it does, we
            promise to return a negative yield.  AIs are advised to
            select another weapon as this one may cause the game to
            segfault or die and nobody wants that! */
         fprintf(stderr, "Bad phoenix weapon: %s.  Children loop or do not exist.\n", info->name);
         return(precision ? -abs(precision) : -1);
      }
      child = sc_weapon_lookup(wc, SC_PHOENIX_CHILD_TYPE(info), SC_WEAPON_LIMIT_NONE);
      numchildren = SC_PHOENIX_CHILD_COUNT(info);
      if(SC_PHOENIX_IS_CHAIN(info)) {
         /* Allow the children to vote on their precisions. */
         for(count = 1; count <= numchildren; ++count)
            precision += pow(SC_PHOENIX_INCREASE_FACTOR, count) * 100 / _sc_weapon_stat_precision(wc, child, triple);
      } else if(SC_PHOENIX_IS_CONVERT(info)) {
         /* Conversions could be more precise, but for now AI's don't know about them. */
         precision *= 100;  /* TEMP - Arbitrary! - JL */
      } else if(SC_PHOENIX_IS_SCATTER(info)) {
         /* Scattering doesn't detonate the original weapon. */
         if(child != info) precision = numchildren * 100 / _sc_weapon_stat_precision(wc, child, triple);
         else              precision = numchildren * precision;
         /* As a rule, scattering is very imprecise, and we hit its precision value hard here. */
         precision *= SC_PHOENIX_SCATTER(info);
      } else if(SC_PHOENIX_IS_SPIDER(info)) {
         /* Spiders detonate the original weapon in the center, and on average 75% of their children */
         if(child != info) precision += 0.75 * numchildren * 100 / _sc_weapon_stat_precision(wc, child, triple);
         else              precision += 0.75 * numchildren * precision;
         /* Spiders are also docked for their imprecise nature. */
         precision *= 2 * SC_PHOENIX_SCATTER(info);
      } else if(SC_PHOENIX_IS_DETONATE(info)) {
         /* We do nothing with detonation requests; they have their face value precision. */
      }
   }

   /* Invert the precision so bigger is better, and normalize a bit.
      This will give a Missile a precision of 1.25, by the way.
      If the weapon is completely imprecise, it scores a zero.
      Likewise, if it doesn't really explode, it gets a zero. */
   precision = (precision && info->force) ? (100 / precision) : 0;

   return(precision);

}



double sc_weapon_statistic(const sc_weapon_config *wc, const sc_weapon_info *info,
                           const sc_player *p, sc_weapon_stat statistic) {
/* sc_weapon_statistic
   Calculate various weapon statistics used in AI purchasing.
   The sc_player pointer is optional (can be NULL, in which case you get defaults). */

   bool triple;
   double result;

   /* Can't allow people to pass us bad info, now can we...? */
   if(wc == NULL || info == NULL)
      return(0);

   /* We set triple to a bool value that is true if we have a triple turret. */
   triple = p && (p->ac_state & SC_ACCESSORY_STATE_TRIPLE);

   switch(statistic) {
      case SC_WEAPON_STAT_PRICE:
         if(info->bundle)
            result = info->price / info->bundle;
         else
            result = 0;
         break;
      case SC_WEAPON_STAT_YIELD:
         result = _sc_weapon_stat_yield(wc, info, triple);
         break;
      case SC_WEAPON_STAT_PRECISION:
         result = _sc_weapon_stat_precision(wc, info, triple);
         break;
      case SC_WEAPON_STAT_ECON_YIELD:
         if(info->price)
            result = _sc_weapon_stat_yield(wc, info, triple) * info->bundle / info->price;
         else
            result = _sc_weapon_stat_yield(wc, info, triple) * info->bundle / SC_INVENTORY_CHEAPO_FACTOR;
         break;
      case SC_WEAPON_STAT_ECON_PRECISION:
         if(info->price)
            result = _sc_weapon_stat_precision(wc, info, triple) * info->bundle / info->price;
         else
            result = _sc_weapon_stat_precision(wc, info, triple) * info->bundle / SC_INVENTORY_CHEAPO_FACTOR;
         break;
      case SC_WEAPON_STAT_PRECISION_YIELD:
         result = _sc_weapon_stat_precision(wc, info, triple) * _sc_weapon_stat_yield(wc, info, triple);
         break;
      default:
         printf("Warning - someone asked for a weapon statistic we don't know how to calculate!\n");
         result = 0;
   }

   return(result);

}



void sc_weapon_info_line(const sc_weapon_config *wc, const sc_weapon_info *info, char *buffer, int buflen) {
/* sc_weapon_info_line
   Create a line of information about the weapon. */

   int moving;

   assert(wc != NULL);
   assert(buflen >= 0);

   if(buffer == NULL || buflen == 0)
      return;

   if(info == NULL) {
      buffer[0] = '\0';
      return;
   }

   /* Clear the buffer. */
   memset(buffer, 0, buflen * sizeof(char));
   /* Terminating NULL character. */
   --buflen;

   /* Add the name to the buffer. */
   sbprintf(buffer, buflen, "%s:", info->name);
   moving = strlenn(info->name) + 1;
   buffer += moving;
   buflen -= moving;

   /* Add spaces out to the end of the name area. */
   while(++moving < 20 && --buflen > 0)
      *(buffer++) = ' ';

   /* Display the weapon's arms level. */
   sbprintf(buffer, buflen, "arms = %1i, ", info->armslevel);
   moving = 10;
   buffer += moving;
   buflen -= moving;

   /* Display the weapon's yield. */
   moving = sc_weapon_statistic(wc, info, NULL, SC_WEAPON_STAT_YIELD) / 1000;
   sbprintf(buffer, buflen, "yield = %-8i", moving);
   moving = 8 + (moving ? (int)log10(moving) : 1);
   buffer += moving;
   buflen -= moving;

   /* Add the comma. */
   if(buflen-- > 0)
      *(buffer++) = ',';

   /* Add spaces out to the end of the yield area, plus two. */
   while(++moving < 18 && --buflen > 0)
      *(buffer++) = ' ';

   /* Write out some weapon info flags. */
   sbprintf(buffer, buflen, "%7s %8s %5s %6s %7s",
            SC_WEAPON_IS_PHOENIX(info) ? "complex" : "",
            SC_WEAPON_IS_INFINITE(info) ? "infinite" : "",
            SC_WEAPON_IS_SMOKING(info) ? "smoke" : "",
            SC_WEAPON_IS_TRIPLE(info) ? "triple" : "",
            info->useless ? "useless" : "");

}



void sc_weapon_print_yields(const sc_weapon_config *wc) {
/* sc_weapon_print_yields
   Print current weapon yields.
   This is very unoptimized, in favor of stressing things more.
   It can help test/troubleshoot the inventorying system... */

   double price, yield, precision, econ_yield, econ_prec, prec_yield;
   int count = sc_weapon_count(wc, SC_WEAPON_LIMIT_NONE);
   const sc_weapon_info *info;

   /* Header */
   printf("\n   %-20s  %8s  %12s  %12s  %12s  %12s  %12s\n",
          "Weapon Name", "Price", "Yield", "Precision", "Econ Yield", "Econ Prec", "Prec Yield");

   /* Get the first weapon. */
   info = sc_weapon_first(wc, SC_WEAPON_SCAN_FORWARD);

   /* Loop over the weapon space. */
   while(count--) {
      assert(info != NULL);
      if(info->bundle > 0) {
         price      = sc_weapon_statistic(wc, info, NULL, SC_WEAPON_STAT_PRICE);
         yield      = sc_weapon_statistic(wc, info, NULL, SC_WEAPON_STAT_YIELD);
         precision  = sc_weapon_statistic(wc, info, NULL, SC_WEAPON_STAT_PRECISION);
         econ_yield = sc_weapon_statistic(wc, info, NULL, SC_WEAPON_STAT_ECON_YIELD);
         econ_prec  = sc_weapon_statistic(wc, info, NULL, SC_WEAPON_STAT_ECON_PRECISION);
         prec_yield = sc_weapon_statistic(wc, info, NULL, SC_WEAPON_STAT_PRECISION_YIELD);
         printf("   %-20s  %8.0f  %12.0f  %12.9f  %12.0f  %12.9f  %12.0f\n",
                info->name, price, yield, precision, econ_yield, econ_prec, prec_yield);
      }
      /* Get the next weapon. */
      info = sc_weapon_next(wc, info, SC_WEAPON_SCAN_FORWARD);
   }

}



/***  Weapon Creation / Destruction  ***/



sc_weapon *sc_weapon_new(const sc_config *c, sc_weapon_info *info,
                         double x, double y, double vx, double vy,
                         bool has_contact_trigger, int playerid) {
/* sc_weapon_new
   Create a new weapon with the parametres described.  The weapon info to
   use is in info.  The starting coordinates are in (x,y), and the starting
   velocities are in (vx,vy).  Also indicate if the weapon is using a
   contact trigger, and which player owns the weapon.  */

   sc_weapon *wp;    /* Weapon pointer */
   int flags;        /* Trajectory flags */

   assert(c != NULL);
   assert(info != NULL);
   assert(playerid >= 0 && playerid < c->numplayers);

   /* Allocate the new weapon structure */
   wp = (sc_weapon *)malloc(sizeof(sc_weapon));
   if(wp == NULL) return(NULL);

   /* Set all weapon creation defaults */
   wp->playerid      = playerid;
   wp->state         = info->state;
   wp->weaponinfo    = info;
   wp->children      = 0;
   wp->chain         = NULL;
   wp->exp_res       = 1;

   /* Are we tunnelling? This is important,
      as we must set the trajectory flags now! */
   wp->triggered     = has_contact_trigger;
   flags = SC_WEAPON_TRAJ_FLAGS(c, wp);

   /* Setup initial trajectory */
   wp->tr = sc_traj_new_velocities(c, c->players[playerid], flags, x, y, vx, vy);

   /* Return the newly created weapon. */
   return(wp);

}



void sc_weapon_landfall(sc_config *c, const sc_weapon *wp) {
/* sc_weapon_landfall
   Perform any landfall operations pending that are associated with
   this particular weapon.  Note that landfall may also affect player
   tank positions. */

   if(c == NULL || wp == NULL || wp->tr == NULL) return;
   sc_traj_landfall(c, wp->tr);

}



void sc_weapon_free(sc_weapon **wp) {
/* sc_weapon_free
   Releases the weapon at the head of the list, and sets *wp to its old
   chain pointer (the next weapon in the list).  */

   sc_weapon *chain; /* On success, wp will be set to the chain weapon */

   /* Release the weapon */
   if(wp == NULL || *wp == NULL) return;
   chain = (*wp)->chain;
   sc_traj_free(&(*wp)->tr);
   free(*wp);
   (*wp) = chain;

}



void sc_weapon_free_chain(sc_weapon **wp) {
/* sc_weapon_free_chain
   Deletes an entire weapon chain.  *wp == NULL after this call.  */

   sc_weapon *chain; /* We must follow the chain and exterminate it */

   /* Release each weapon */
   if(wp == NULL) return;
   while(*wp != NULL) {
      chain = (*wp)->chain;
      free(*wp);
      (*wp) = chain;
   }

}



static inline bool _sc_weapon_fire_new(sc_config *c, sc_player *p, sc_explosion **e) {
/* _sc_weapon_fire_new
   A tank wants to fire a weapon.
   Called by sc_weapon_create_all, below.
   Returns true if it creates explosions. */

   sc_weapon *wp;    /* A pointer to point to the weapon to return */
   double factorx;   /* Based on turret angle; contribution to X velocity */
   double factory;   /* Based on turret angle; contribution to Y velocity */
   sc_weapon_info *i, *ti = NULL;  /* Weapon info pointers */
   double x, y, vx, vy;            /* Temporary weapon info */
   bool ct;                        /* Temporary weapon info */
   bool explosions = false;        /* Did we trigger explosions? */

   assert(c != NULL && p != NULL && e != NULL);
   /* But *e can be NULL if it wants. ;) */

   /* Make sure we can fire and are firing something */
   i = p->selweapon;
   p->weapons = NULL;
   if(p->dead || !p->armed || SC_WEAPON_IS_NULL(i))
      return(false);

   /* Calculate contribution to each direction, based on turret angle */
   factorx = cos(p->turret * M_PI / 180);
   factory = sin(p->turret * M_PI / 180);

   /* Set initial weapon position (based on turret radius and angle) */
   /* Also set initial weapon velocity based on the tank's power. */
   x = sc_player_turret_x(p, p->turret);
   y = sc_player_turret_y(p, p->turret);
   vx = p->power * SC_PHYSICS_VELOCITY_SCL * factorx;
   vy = p->power * SC_PHYSICS_VELOCITY_SCL * factory;

   /* Do we need to apply a contact trigger? */
   ct = sc_player_use_contact_trigger(c, p);

   /* Decrement player weapon inventory */
   if(!SC_WEAPON_IS_INFINITE(i) && (--(i->inventories[p->index]) <= 0)) {
      /* We ran out, so return to the default weapon. */
      p->selweapon = SC_WEAPON_DEFAULT(c->weapons);
   }

   /* If it's a triple turret type weapon and we have a triple turret, then
      we must temporarily replace its weaponinfo with a new phoenix one.
      NOTE that this info will be free'd before this function exits... */
   if((p->ac_state & SC_ACCESSORY_STATE_TRIPLE) && (i->state & SC_WEAPON_STATE_TRIPLE)) {
      ti = (sc_weapon_info *)malloc(sizeof(sc_weapon_info));
      if(ti == NULL) {
         printf("Your weapon hasn't enough memory to fire, sir!  (fire_new() malloc failure)\n");
         return(false);
      }
      memcpy((void *)ti, (void *)i, sizeof(sc_weapon_info));
      ti->ph_fl |= SC_PHOENIX_AT_TANK | SC_PHOENIX_TYPE_SCATTER;
      ti->ph_ch = i->ident;
      /* NOTE that we hard-code behavior of the triple turret here.
         It is a TRIPLE turret after all...  ;-)
         Still, this should probably change at some point. */
      ti->children = 3;
      ti->scatter = 3;
      /* Allocate the new weapon structure */
      wp = sc_weapon_new(c, ti, x, y, vx, vy, ct, p->index);
   } else {
      /* Allocate the new weapon structure */
      wp = sc_weapon_new(c, i, x, y, vx, vy, ct, p->index);
   }

   /* If it is the right phoenix type, the weapon should be split now. */
   if(SC_WEAPON_IS_PHOENIX(wp->weaponinfo) && SC_PHOENIX_IS_AT_TANK(wp->weaponinfo))
      switch(sc_phoenix(SC_PHOENIX_AT_TANK, c, &wp, e)) {
         /* This is the case we expect if it is a triple turret weapon */
         case SC_PHOENIX_SIZZLE:
            sc_weapon_landfall(c, wp);
            sc_weapon_free(&wp);
            break;

         /* This is the case we expect for detonate at_tank weapons */
         case SC_PHOENIX_DETONATE:
            sc_weapon_landfall(c, wp);
            sc_weapon_free(&wp);
            explosions = true;
            break;

         /* How I hate it when things fail... */
         case SC_PHOENIX_FAILURE:
            sc_weapon_free_chain(&wp);
            break;

         /* This is the case we expect if it is not a tank phoenix weapon. */
         case SC_PHOENIX_NO_ACTION:
            wp->weaponinfo = i; /* safety */
         default:
            /* do nothing */;
      }

   /* Set the player as owner of the weapon chain */
   p->weapons = wp;

   /* This is quite important, despite being usually free(NULL). */
   free(ti);
   return(explosions);

}



bool sc_weapon_create_all(sc_config *c, sc_explosion **e) {
/* sc_weapon_create_all
   Create the weapons for each player, to be launched.
   Returns true if we already have explosions to run... */

   bool explosions = false;
   sc_player *p;
   int i;

   for(i = c->numplayers - 1; i >= 0; --i) {
      p = c->players[i];
      if(!p->dead && p->armed) {
         if(_sc_weapon_fire_new(c, p, e)) explosions = true;
         p->armed = false;
      }
   }

   return(explosions);

}



/***  Weapon Lists and Configuration Structs  ***/



sc_weapon_config *sc_weapon_config_create(const sc_config *c) {
/* sc_weapon_config_create
   Allocate space and set defaults on a new sc_weapon_config struct. */

   sc_weapon_config *wc;
   const char *filename;

   assert(c != NULL && c->registry != NULL);

   wc = (sc_weapon_config *)malloc(sizeof(sc_weapon_config));
   if(wc == NULL) return(NULL);

   /* Default settings for weapon config. */
   wc->armslevel     = SC_ARMS_LEVEL_DEF;
   wc->bombiconsize  = SC_WEAPON_BOMB_ICON_DEF;
   wc->tunneling     = false;
   wc->scaling       = 1.0;
   wc->tracepaths    = false;
   wc->uselessitems  = true;

   /* Get a class ID for this weapon config. */
   wc->registryclass = sc_registry_get_new_class_id(c->registry);
   wc->registry      = c->registry;

   /* Read in the weapon info list */
   filename = SC_GLOBAL_DIR "/" SC_WEAPON_FILE;
   if(!sc_addconf_append_file(SC_ADDCONF_WEAPONS, filename, wc) ||
      sc_weapon_count(wc, SC_WEAPON_LIMIT_INDIRECT) <= 0) {
      /* This is the root weapon list...  Die! */
      free(wc);
      return(NULL);
   }

   return(wc);

}



void sc_weapon_config_destroy(sc_weapon_config **wc) {
/* sc_weapon_config_destroy
   Invalidate memory used by a weapon config struct. */

   sc_weapon_info *info, *temp;

   if(wc == NULL || *wc == NULL) return;

   /* Delete all of our registry entries. */
   info = (sc_weapon_info *)sc_registry_find_first((*wc)->registry, (*wc)->registryclass,
                                                   SC_REGISTRY_FORWARD, SC_REGISTRY_TEST_NULL, 0);
   while(info != NULL) {
      temp = info;
      info = (sc_weapon_info *)sc_registry_find_next((*wc)->registry, (*wc)->registryclass, info->ident,
                                                     SC_REGISTRY_FORWARD, SC_REGISTRY_TEST_NULL, 0);
      sc_registry_del_by_int((*wc)->registry, temp->ident);
      sc_weapon_info_free(&temp);
   }

   /* And delete ourself. */
   free(*wc);
   *wc = NULL;

}



void sc_weapon_info_free(sc_weapon_info **wi) {
/* sc_weapon_info_free
   Invalidate memory used by an sc_weapon_info. */

   /* Make sure there is an item to free */
   if(wi == NULL || *wi == NULL) return;
   /* Free the item's name if it has one */
   if((*wi)->name != NULL) free((*wi)->name);
   /* Free the item */
   free(*wi);
   *wi = NULL;

}



/***  Misc weapons functions  ***/



bool _sc_weapon_inventory_clear_func(void *data, __libj_unused long arg) {
/* _sc_weapon_inventory_clear_func
   Clear weapon inventory data, in registry fast iterators. */

   sc_weapon_info *info;

   if(data == NULL) return(false);
   info = (sc_weapon_info *)data;

   /* Clear the weapon's inventory. */
   if(!SC_WEAPON_IS_INFINITE(info))
      memset(info->inventories, 0, SC_MAX_PLAYERS * sizeof(int));

   /* We never "find the right weapon" ... it's faster this way. */
   return(false);

}


void sc_weapon_inventory_clear(sc_weapon_config *wc) {
/* sc_weapon_inventory_clear
   Clear out the player weapon inventories. */

   sc_registry_iter *iter;

   /* Prepare a registry iterator. */
   iter = sc_registry_iter_new(wc->registry, wc->registryclass, SC_REGISTRY_FORWARD,
                               _sc_weapon_inventory_clear_func, 0);

   /* Iterate the entire weapons registry,
      with the side effect of erasing inventories. */
   sc_registry_iterate(iter);

   /* Clean up. */
   sc_registry_iter_free(&iter);

}



sc_explosion *sc_weapon_get_explosion(const sc_config *c, const sc_weapon *wp,
                                      int x, int y, double direction) {
/* sc_weapon_get_explosion
   Take a weapon and a location, and return a ready-made explosion.
   The explosion will be created with a point-of-impact at (x,y).  */

   double angular_width;
   int realradius;
   int force;
   int playerid;
   sc_explosion *expl;
   sc_explosion_type type;
   bool create_spill = false;

   assert(c != NULL && c->weapons != NULL);
   assert(wp != NULL && wp->weaponinfo != NULL);

   /* Figure out the real radius and force of the explosion */
   angular_width  = wp->weaponinfo->angular_width;
   realradius     = c->weapons->scaling * wp->exp_res * wp->weaponinfo->radius;
   force          = wp->weaponinfo->force;
   playerid       = wp->playerid;

   /* Figure out what explosion type to create. */
   if(SC_WEAPON_IS_PLASMA(wp)) {
      type = SC_EXPLOSION_PLASMA;
   } else if(SC_WEAPON_IS_NAPALM(wp)) {
      type = SC_EXPLOSION_NAPALM;
      create_spill = true;
   } else if(SC_WEAPON_IS_LIQ_DIRT(wp)) {
      /* Caution: must appear before IS_DIRT! */
      type = SC_EXPLOSION_LIQ_DIRT;
      create_spill = true;
   } else if(SC_WEAPON_IS_DIRT(wp)) {
      type = SC_EXPLOSION_DIRT;
   } else if(SC_WEAPON_IS_RIOT(wp)) {
      type = SC_EXPLOSION_RIOT;
   } else {
      type = SC_EXPLOSION_NORMAL;
   } /* Which type of explosion? */

   /* Create the initial explosion */
   if(angular_width != 0.0) {
      expl = sc_expl_new_with_angle(x, y, realradius, force,
                                 direction, angular_width,
                                 playerid, type);
   } else {
      expl = sc_expl_new(x, y, realradius, force, playerid, type);
   }
   if(expl == NULL) return(NULL);

   /* If a spill was requested, build the spill now. */
   if(create_spill) {
      expl->data = sc_spill_new(c, c->land, wp->weaponinfo->liquid,
                                expl->centerx, expl->centery);
   }

   /* Return the new explosion. */
   return(expl);

}



/***  Weapon Configuration File Support  ***/



/* Summary of the weapon state bits, for use in saddconf.c */
static const char *_sc_weapon_state_bit_names[] = {
   "beam",
   "defer",
   "digger",
   "dirt",
   "liquid",
   "null",
   "plasma",
   "riot",
   "roller",
   "sapper",
   "smoke",
   "triple",
   NULL
};
static const unsigned int _sc_weapon_state_bit_items[] = {
   SC_WEAPON_STATE_BEAM,
   SC_WEAPON_STATE_DEFER,
   SC_WEAPON_STATE_DIGGER,
   SC_WEAPON_STATE_DIRT,
   SC_WEAPON_STATE_LIQUID,
   SC_WEAPON_STATE_NULL,
   SC_WEAPON_STATE_PLASMA,
   SC_WEAPON_STATE_RIOT,
   SC_WEAPON_STATE_ROLLER,
   SC_WEAPON_STATE_SAPPER,
   SC_WEAPON_STATE_SMOKE,
   SC_WEAPON_STATE_TRIPLE,
   0
};



const char **sc_weapon_state_bit_names(void) {

   return(_sc_weapon_state_bit_names);

}



const unsigned int *sc_weapon_state_bit_items(void) {

   return(_sc_weapon_state_bit_items);

}
