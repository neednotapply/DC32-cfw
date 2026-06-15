/* $Header: /fridge/cvs/xscorch/sgame/sphoenix.c,v 1.31 2011-08-01 00:01:41 jacob Exp $ */
/*

   xscorch - sphoenix.c       Copyright(c) 2000-2003 Jacob Luna Lundberg
   jacob(at)gnifty.net        http://www.gnifty.net/

   Scorched phoenix type weapons calls


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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <sphoenix.h>
#include <scolor.h>
#include <sconfig.h>
#include <sexplosion.h>
#include <sinventory.h>
#include <sland.h>
#include <sphysics.h>
#include <splayer.h>
#include <sshield.h>
#include <sweapon.h>
#include <swindow.h>

#include <sutil/srand.h>



static sc_phoenix_result _sc_phoenix_chain(__libj_unused int locate, const sc_config *c, sc_weapon **wp, sc_explosion **e) {
/* _sc_phoenix_chain
   Split a missile with detonation on land and bounce a child if it has any */

   double dx, dy;
   int ph_type, ph_count;
   sc_explosion *expl;
   sc_weapon_info *ph_i;
   sc_weapon *ph_wp;

   /* What shall we reincarnate as? */
   ph_type = SC_PHOENIX_CHILD_TYPE((*wp)->weaponinfo);
   ph_count = SC_WEAPON_CHILD_COUNT(*wp);

   /* Allow the final child to detonate on its own. */
   if(ph_count >= SC_PHOENIX_CHILD_COUNT((*wp)->weaponinfo))
      return(SC_PHOENIX_NO_ACTION);

   /* Return subsurface weapons to the playing field. */
   sc_land_translate_y_d(c->land, &(*wp)->tr->cury);
   if((*wp)->tr->cury <= 0) (*wp)->tr->cury = 1;

   /* Insert the next phoenix-like frog (replacing the current weapon). */
   ph_i = sc_weapon_lookup(c->weapons, ph_type, SC_WEAPON_LIMIT_NONE);
   ph_wp = sc_weapon_new(c, (*wp)->weaponinfo, (*wp)->tr->curx, (*wp)->tr->cury, sc_traj_get_velocity_x((*wp)->tr), sc_traj_get_velocity_y((*wp)->tr), (*wp)->triggered, (*wp)->playerid);
   ph_wp->state |= SC_WEAPON_STATE_DEFER;
   ph_wp->chain = (*wp)->chain;
   (*wp)->chain = ph_wp;

   /* Scale and modify values of the new frog. */
   ph_wp->exp_res = (*wp)->exp_res * SC_PHOENIX_INCREASE_FACTOR;
   dx = fabs(sc_traj_get_velocity_x((*wp)->tr)) * ph_wp->exp_res;
   dy = fabs(sc_traj_get_velocity_y((*wp)->tr)) * ph_wp->exp_res;

   /* Bounding by acceptable values. */
   if(dy < SC_PHYSICS_VELOCITY_SCL * SC_PHOENIX_MIN_FROG_POWER)
      dy = SC_PHYSICS_VELOCITY_SCL * SC_PHOENIX_MIN_FROG_POWER;
   if(dx < SC_PHYSICS_VELOCITY_SCL * SC_PHOENIX_MIN_FROG_POWER)
      dx = SC_PHYSICS_VELOCITY_SCL * SC_PHOENIX_MIN_FROG_POWER;
   if(dy > SC_PHYSICS_VELOCITY_SCL * SC_PHOENIX_MAX_FROG_POWER)
      dy = SC_PHYSICS_VELOCITY_SCL * SC_PHOENIX_MAX_FROG_POWER;
   if(dx > SC_PHYSICS_VELOCITY_SCL * SC_PHOENIX_MAX_FROG_POWER)
      dx = SC_PHYSICS_VELOCITY_SCL * SC_PHOENIX_MAX_FROG_POWER;

   /* Restore sign of X axis (Y axis must be positive). */
   if(sc_traj_get_velocity_x((*wp)->tr) < 0)
      dx *= -1;

   /* Finalize the new trajectory. */
   sc_traj_set_velocity(ph_wp->tr, dx, dy);

   /* This is the a run-time child count. */
   ph_wp->children = ++ph_count;

   /* We want the old warhead detonated now. */
   expl = sc_weapon_get_explosion(c, *wp, (*wp)->tr->curx, (*wp)->tr->cury, SC_EXPL_DEFAULT_DIR);
   if(expl == NULL) return(SC_PHOENIX_FAILURE);
   expl->radius = c->weapons->scaling * (*wp)->exp_res * ph_i->radius;
   expl->force  = ph_i->force;
   sc_expl_add(e, expl);

   return(SC_PHOENIX_DETONATE);

}



static sc_phoenix_result _sc_phoenix_spider(__libj_unused int locate, const sc_config *c, sc_weapon **wp, sc_explosion **e) {
/* _sc_phoenix_spider
   Split a missile with detonation on land and give random destinations */

   int i, j, ph_type, ph_count, expl_count, leg_radius, leg_force, scatter;
   sc_explosion *cure; /* for CURrent Explosion ;) */
   sc_weapon_info *ph_i;
   double srcx;
   double srcy;

   /* What shall we reincarnate as? */
   ph_type = SC_PHOENIX_CHILD_TYPE((*wp)->weaponinfo);
   ph_count = SC_PHOENIX_CHILD_COUNT((*wp)->weaponinfo);

   ph_i = sc_weapon_lookup(c->weapons, ph_type, SC_WEAPON_LIMIT_NONE);
   /* Random number of explosions based on no less than half of child count. */
   expl_count = ph_count / 2 + game_lrand(ph_count / 2);
   leg_radius = ph_i->radius;
   leg_force = ph_i->force;
   /* Radius of blast. */
   scatter = SC_PHOENIX_SCATTER((*wp)->weaponinfo) * (*wp)->exp_res *
             (*wp)->weaponinfo->radius * SC_PHOENIX_SPIDER_POWER;

   /* First the old weapon explodes (we hope)... */
   if(!sc_expl_add(e, sc_weapon_get_explosion(c, *wp, (*wp)->tr->curx, (*wp)->tr->cury, SC_EXPL_DEFAULT_DIR)))
      goto failure;

   /* Start loop on number of spider legs (child explosions). */
   for(i = 0; i < expl_count; i++) {
      /* Create a new explosion. */
      cure = sc_weapon_get_explosion(c, *wp, 0, 0, SC_EXPL_DEFAULT_DIR);
      if(cure == NULL) goto failure;
      cure->radius = c->weapons->scaling * (*wp)->exp_res * leg_radius;
      cure->force  = leg_force;
      cure->type   = SC_EXPLOSION_SPIDER;

      /* Pick a random x within blast radius.  The wall rules and location validation
         are performed in the low-level land manipulation and trajectory functions now.
         Also, we make sure to bail out if we try many legs and they all fail validity.
         In this case, we will simply make (at least) one fewer child explosion(s). */
      for(j = 0; j < SC_PHOENIX_SPIDER_MAX_LEG_ATTEMPTS; ++j) {
         /* Select what we hope is a reasonable center. */
         cure->centerx = (*wp)->tr->curx - scatter + game_lrand(2 * scatter);
         /* Locate the y location corresponding to our x. */
         cure->centery = sc_land_height(c->land, cure->centerx, c->fieldheight);
         /* Valid spider leg found? */
         if(cure->centery <= (*wp)->tr->cury + SC_PHOENIX_SPIDER_ARC) {
            /* Create arc. */
            srcx = (*wp)->tr->curx;
            srcy = sc_land_height(c->land, srcx, c->fieldheight);
            if((*wp)->tr->cury > srcy) srcy = (*wp)->tr->cury;
            cure->data = sc_traj_new_dest_height(c, c->players[(*wp)->playerid], SC_TRAJ_IGNORE_WIND | SC_TRAJ_IGNORE_VISC | SC_TRAJ_TERMINUS, srcx, srcy, cure->centerx - srcx, cure->centery - srcy, SC_PHOENIX_SPIDER_ARC);
            /* Add explosion to explosion chain. */
            sc_expl_add(e, cure);
            /* Avoid free(cure) when we've added it to e. */
            cure = NULL;
            break;
         }
      } /* maximum leg attempts */

      /* If we utterly failed to create this leg, free its explosion. */
      if(cure != NULL) sc_expl_free(&cure);
   } /* expl_count */

   /* We have detonated the initial weapon and (hopefully) some children. */
   return(SC_PHOENIX_DETONATE);

   failure:
   if(cure != NULL) sc_expl_free(&cure);
   return(SC_PHOENIX_FAILURE);

}



static sc_phoenix_result _sc_phoenix_scatter(__libj_unused int locate, const sc_config *c, sc_weapon **wp, __libj_unused sc_explosion **e) {
/* _sc_phoenix_scatter
   Scatter a missile without detonation */

   float xscatter, yscatter;
   sc_weapon *ph_wp, *chain;
   sc_weapon_info *ph_i;
   double velx;
   double vely;
   int ph_type;
   int ph_count;

   /* What shall we reincarnate as? */
   ph_type = SC_PHOENIX_CHILD_TYPE((*wp)->weaponinfo);
   ph_count = SC_PHOENIX_CHILD_COUNT((*wp)->weaponinfo);

   /* Set some more variables. */
   ph_i = sc_weapon_lookup(c->weapons, ph_type, SC_WEAPON_LIMIT_NONE);
   chain = (*wp)->chain;
   ph_wp = *wp;

   /* Set us up to scatter the missiles.  Conceptually, this should be thought
      of as if the old weapon was actually a chain of new weapons linked together.
      This function just sets off explosions at all of the linkeages, starting
      first with the middle one(s), so that the outer weapons are given the
      largest velocity boosts (in opposing directions) and the center weapon, if
      there are an odd number, retains the old velocity.  This behavior is
      fairly consistant with the behavior of the original Scorched Earth. */
   velx = sc_traj_get_velocity_x((*wp)->tr);
   vely = sc_traj_get_velocity_y((*wp)->tr);
   if(ph_count > 1) {
      if(velx == 0 && vely == 0) {
         /* Avoid divide by zero. */
         xscatter = SC_PHOENIX_SCATTER_SCALE * SC_PHOENIX_SCATTER((*wp)->weaponinfo);
         yscatter = 0;
      } else {
         /* Find the x and y portions of the scatter. */
         xscatter = SC_PHOENIX_SCATTER_SCALE * SC_PHOENIX_SCATTER((*wp)->weaponinfo) * velx / DIST(velx, vely);
         yscatter = SC_PHOENIX_SCATTER_SCALE * SC_PHOENIX_SCATTER((*wp)->weaponinfo) * vely / DIST(velx, vely);
      }
      velx -= 0.5 * xscatter;
      vely -= 0.5 * yscatter;
      xscatter /= ph_count;
      yscatter /= ph_count;
   } else {
      xscatter = 0;
      yscatter = 0;
   }

   /* The weapons have monotonically differential velocities. */
   for(; ph_count > 0; --ph_count) {
      velx += xscatter;
      vely += yscatter;
      ph_wp->chain = sc_weapon_new(c, ph_i, (*wp)->tr->curx, (*wp)->tr->cury, velx, vely, (*wp)->triggered, (*wp)->playerid);
      ph_wp = ph_wp->chain;
   }
   ph_wp->chain = chain;

   /* sc_phoenix_scatter never detonates weapons; they just sizzle.
      This is not a mandate; it's really more of a cosmetic thing.
      It would be odd to have them exploding in mid air... */
   return(SC_PHOENIX_SIZZLE);

}



static sc_phoenix_result _sc_phoenix_convert(__libj_unused int locate, const sc_config *c, sc_weapon **wp, sc_explosion **e) {
/* sc_phoenix_convert
 * Convert a weapon to a new trajectory handler */

   sc_phoenix_result result = SC_PHOENIX_NO_ACTION;
   int ph_type;
   int ph_count;
   int flags;

   /* We'll need the weapon flags. */
   flags = SC_WEAPON_TRAJ_FLAGS(c, *wp);

   /* Maybe convert rollers. */
   if(SC_WEAPON_IS_ROLLER(*wp)) {
      /* Skip the conversion if it's already been done. */
      if(SC_TRAJ_IS_ROLLING((*wp)->tr)) {
         result = SC_PHOENIX_NO_ACTION;
         goto out;
      }
      /* Convert the trajectory to the new type. */
      if(!sc_trajectory_convert(c, (*wp)->tr, flags, SC_TRAJ_ROLLER)) {
         /* Manually detonate the failed conversion. */
         if(sc_expl_add(e, sc_weapon_get_explosion(c, *wp, (*wp)->tr->curx, (*wp)->tr->cury, SC_EXPL_DEFAULT_DIR)))
            result = SC_PHOENIX_DETONATE;
         else
            result = SC_PHOENIX_FAILURE;
         goto out;
      }
      /* We must ask the tracking code to reset itself. */
      result = SC_PHOENIX_RESET;
   }

   /* Maybe convert diggers. */
   if(SC_WEAPON_IS_DIGGER(*wp)) {
      /* Skip the conversion if it's already been done. */
      if(SC_TRAJ_IS_DIGGING((*wp)->tr)) {
         result = SC_PHOENIX_NO_ACTION;
         goto out;
      }
      /* Convert the trajectory to the new type. */
      if(!sc_trajectory_convert(c, (*wp)->tr, flags, SC_TRAJ_DIGGER)) {
         /* Manually detonate the failed conversion. */
         if(sc_expl_add(e, sc_weapon_get_explosion(c, *wp, (*wp)->tr->curx, (*wp)->tr->cury, SC_EXPL_DEFAULT_DIR)))
            result = SC_PHOENIX_DETONATE;
         else
            result = SC_PHOENIX_FAILURE;
         goto out;
      }
      /* We must ask the tracking code to reset itself. */
      result = SC_PHOENIX_RESET;
   }

   out:
   return(result);

}



static sc_phoenix_result _sc_phoenix_detonate(int locate, const sc_config *c, sc_weapon **wp, sc_explosion **e) {
/* sc_phoenix_detonate
 * Detonate a weapon in place, immediately */

   double direction;

   /* DETONATE is a special phoenix type; we do not require children... */

   /* What direction should wedged blasts spread in? */
   if(locate == SC_PHOENIX_AT_TANK)
      direction = c->players[(*wp)->playerid]->turret * M_PI / 180.0;
   else
      direction = SC_EXPL_DEFAULT_DIR;

   /* Take whatever we've got and blow it sky high! */
   if(sc_expl_add(e, sc_weapon_get_explosion(c, *wp, (*wp)->tr->curx, (*wp)->tr->cury, direction)))
      return(SC_PHOENIX_DETONATE);
   else
      return(SC_PHOENIX_FAILURE);

}



sc_phoenix_result sc_phoenix(int locate, const sc_config *c, sc_weapon **wp, sc_explosion **e) {
/* sc_phoenix_result
   Check if a weapon is phoenix class and if so call the requested action on it.
   It is important to *NOTE* that when these subfunctions return, the original
   wp pointer is left intact.  It MUST BE DELETED by the calling function if the
   return value is either SC_PHOENIX_DETONATE or SC_PHOENIX_SIZZLE.  In certain
   cases like spider explosions, detonations will be created by the subfunctions.
   It is up to the caller to verify that the weapon is intended to be modified
   at the phoenix location it is calling from. */

   sc_phoenix_result result = SC_PHOENIX_NO_ACTION;

   /* Run the appropriate phoenix function(s); NOTE that although this is not
      an either/or operation, strange things may happen if you take advantage
      of this fact.  We simply haven't tested what will occur. */
   if(SC_PHOENIX_IS_DETONATE((*wp)->weaponinfo)) {
      result = _sc_phoenix_detonate(locate, c, wp, e);
      if(result == SC_PHOENIX_DETONATE || result == SC_PHOENIX_FAILURE) goto out;
   }
   if(SC_PHOENIX_CHILD_TYPE((*wp)->weaponinfo)  <= 0 ||
      SC_PHOENIX_CHILD_COUNT((*wp)->weaponinfo) <= 0) {
      /* All of the remaining operations require children... */
      goto out;
   }
   if(SC_PHOENIX_IS_CONVERT((*wp)->weaponinfo)) {
      result = _sc_phoenix_convert(locate, c, wp, e);
      if(result == SC_PHOENIX_RESET || result == SC_PHOENIX_FAILURE) goto out;
   }
   if(SC_PHOENIX_IS_CHAIN((*wp)->weaponinfo)) {
      result = _sc_phoenix_chain(locate, c, wp, e);
      if(result == SC_PHOENIX_DETONATE || result == SC_PHOENIX_FAILURE) goto out;
   }
   if(SC_PHOENIX_IS_SPIDER((*wp)->weaponinfo)) {
      result = _sc_phoenix_spider(locate, c, wp, e);
      if(result == SC_PHOENIX_DETONATE || result == SC_PHOENIX_FAILURE) goto out;
   }
   if(SC_PHOENIX_IS_SCATTER((*wp)->weaponinfo)) {
      result = _sc_phoenix_scatter(locate, c, wp, e);
      if(result == SC_PHOENIX_DETONATE || result == SC_PHOENIX_FAILURE) goto out;
   }

   out:
   return(result);

}



/* An slink list of slcache structs is used to detect circular weapon definitions. */
typedef struct _slcache {int id; struct _slcache *next;} slcache;



static int _sc_phoenix_safe_verify(const sc_weapon_config *wc, const sc_weapon_info *info, slcache *cache, slcache *prev) {
/* sc_phoenix_verify
   Recursively scan the weapon's info structure and check for fatal errors. */

   const sc_weapon_info *child;
   slcache *temp = cache;
   slcache current;
   int count = 1;
   int level = 0;

   /* Create cache entry for this level. */
   prev->next = &current;
   current.id = SC_WEAPON_TYPE(info);
   current.next = NULL;

   /* DETONATE types aren't *really* phoenix weapons, per se... */
   if(SC_WEAPON_IS_PHOENIX(info) && !SC_PHOENIX_IS_DETONATE(info)) {
      /* Find out what we're expecting as a child. */
      child = sc_weapon_lookup(wc, SC_PHOENIX_CHILD_TYPE(info), SC_WEAPON_LIMIT_NONE);
      /* A non-extant child will really throw a sabo into the machinery. */
      if(child == NULL) return(0);
      /* If we are our own child then we're staring at the face of infinity... */
      if(SC_WEAPON_TYPE(info) == SC_WEAPON_TYPE(child)) return(0);
      /* Scan the weapon's ancestors for loop conditions. */
      while(temp != prev->next) {
         /* If the current weapon is among its ancestors, kill it. */
         if(SC_WEAPON_TYPE(info) == temp->id) return(0);
         temp = temp->next;
      }
      /* Find out what the levels under our child are. */
      level = _sc_phoenix_safe_verify(wc, child, cache, &current);
      /* If our child is invalid, then we are. */
      if(level == 0) return(0);
      /* Add up the potential levels under each type we have. */
      if(SC_PHOENIX_IS_CHAIN(info))   count += level;
      if(SC_PHOENIX_IS_SPIDER(info))  count += level;
      if(SC_PHOENIX_IS_SCATTER(info)) count += level;
   }

   /* A non-phoenix or DETONATE weapon is by definition acceptable.
      Besides, that's the stopping condition... */

   /* Release this level of the cache. */
   prev->next = NULL;

   /* Return count of this and sub levels. */
   return(count);

}



int sc_phoenix_verify(const sc_weapon_config *wc, const sc_weapon_info *info) {
/* sc_phoenix_verify
   Verify the validity of a phoenix weapon.  Return non-zero if it is a valid
   phoenix weapon.  Specifically, return the number of levels of children
   defined by the weapon.  This is a bit expensive; don't call it too often. */

   /* Create the temporary cache head. */
   slcache cache;
   cache.id = -1;
   cache.next = NULL;

   /* Scan the weapon. */
   return(_sc_phoenix_safe_verify(wc, info, &cache, &cache));

}



/* Phoenix flag information for saddconf.c */
static const char *_sc_phoenix_flags_bit_names[] = {
   "at_rand",
   "at_tank",
   "at_apex",
   "at_land",
   "chain",
   "spider",
   "scatter",
   "convert",
   "detonate",
   NULL
};
static const unsigned int _sc_phoenix_flags_bit_items[] = {
   SC_PHOENIX_AT_RAND,
   SC_PHOENIX_AT_TANK,
   SC_PHOENIX_AT_APEX,
   SC_PHOENIX_AT_LAND,
   SC_PHOENIX_TYPE_CHAIN,
   SC_PHOENIX_TYPE_SPIDER,
   SC_PHOENIX_TYPE_SCATTER,
   SC_PHOENIX_TYPE_CONVERT,
   SC_PHOENIX_TYPE_DETONATE,
   0
};



const char **sc_phoenix_flags_bit_names(void) {

   return(_sc_phoenix_flags_bit_names);

}



const unsigned int *sc_phoenix_flags_bit_items(void) {

   return(_sc_phoenix_flags_bit_items);

}
