/* $Header: /fridge/cvs/xscorch/sgame/sshield.c,v 1.41 2011-08-01 00:01:41 jacob Exp $ */
/*

   xscorch - sshield.c        Copyright(c) 2000-2003 Justin David Smith
                              Copyright(c) 2000-2003 Jacob Luna Lundberg
                              Copyright(c) 2003           Jason House
   justins(at)chaos2.org      http://chaos2.org/
   jacob(at)gnifty.net        http://www.gnifty.net/

   Scorched shields


   This program is free software; you can redistribute it and/or modify 
   it under the terms of the GNU General Public License as published by 
   the Free Software Foundation; either version 2 of the License, or 
   (at your option) any later version.

   This program is distributed in the hope that it will be useful, 
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
   General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

*/
#include <math.h>
#include <stdlib.h>

#include <sshield.h>
#include <saccessory.h>
#include <sconfig.h>
#include <sexplosion.h>
#include <sland.h>
#include <sphysics.h>
#include <splayer.h>
#include <stankpro.h>

#include <sutil/srand.h>



sc_shield *sc_shield_new(sc_accessory_info *acc) {
/* sc_shield_new
   Create a new shield. */

   sc_shield *sh;

   if(!SC_ACCESSORY_IS_SHIELD(acc)) return(NULL);

   sh = (sc_shield *)malloc(sizeof(sc_shield));
   if(sh == NULL) return(NULL);

   sh->info = acc;
   sh->life = acc->shield;

   return(sh);

}



void sc_shield_free(sc_shield **sh) {
/* sc_shield_free
   Free an old (dead) shield. */

   if(sh == NULL || *sh == NULL) return;
   free(*sh);
   *sh = NULL;

}



sc_accessory_info *sc_shield_find_best(const sc_config *c, const sc_player *p) {
/* sc_shield_find_best
   Find the best shield in a player's inventory.
   This is very accurate, but also very sensitive to shield definition changes. */

   int count;        /* Iterator variable */
   sc_accessory_info *info, *best = NULL;

   /* Sanity check */
   if(c == NULL || p == NULL) return(false);

   /* Search for the best shield in inventory */
   count = sc_accessory_count(c->accessories, SC_ACCESSORY_LIMIT_ALL);
   info = sc_accessory_first(c->accessories, SC_ACCESSORY_LIMIT_ALL | SC_ACCESSORY_SCAN_REVERSE);
   for(; count > 0; --count) {
      if(SC_ACCESSORY_IS_SHIELD(info) && info->inventories[p->index] > 0) {
         if(best == NULL) {
            best = info;
         } else {
            if(info->shield > best->shield) {
               best = info;
            } else if(info->shield == best->shield) {
               if(SC_ACCESSORY_SHIELD_IS_FORCE(info)) {
                  best = info;
               } else if(SC_ACCESSORY_SHIELD_IS_STANDARD(info) && SC_ACCESSORY_SHIELD_IS_MAGNETIC(best)) {
                  best = info;
               } else if(SC_ACCESSORY_SHIELD_IS_MAGNETIC(info) && SC_ACCESSORY_SHIELD_IS_MAGNETIC(best)) {
                  if(info->repulsion > best->repulsion) {
                     best = info;
                  } /* shield has more repulsion? */
               } /* shield better by type? */
            } /* shield obviously better, or just maybe better? */
         } /* found a shield already? */
      } /* shield we own? */
      info = sc_accessory_next(c->accessories, info, SC_ACCESSORY_LIMIT_ALL | SC_ACCESSORY_SCAN_REVERSE);
   }

   /* done */
   return(best);

}



void sc_shield_init_turn(struct _sc_player *p) {
/* sc_shield_init_turn
   Prepare a player's shield for the next turn. */

   /* How can you expect us to recharge a nonexistant shield? */
   if(p == NULL || p->shield == NULL) return;

   /* If the player has a shield recharger (like a Solar Panel), recharge! */
   if(p->ac_state & SC_ACCESSORY_STATE_RECHARGE) {
      p->shield->life += SC_SHIELD_RECHARGE_RATE;
      if(p->shield->life > p->shield->info->shield) {
         p->shield->life = p->shield->info->shield;
      }
   }

}



bool sc_shield_would_impact(const sc_config *c, const sc_player *owner, const sc_player *p, int traj_flags,
                            double x, double y, double nextx, double nexty) {
/* sc_shield_would_impact
   When a shield will, to the best of our knowledge, take a hit in the
   next trajectory step.  We look to see that we will be passing from
   the outside of a player's shield to the inside. */

   double dx, dy, nextdx, nextdy, rad2, nextrad2;

   /* Sanity checks */   
   if(c == NULL || p == NULL) return(false);

   /* Make sure we have a shield here */
   if(p->shield == NULL)
      return(false);

   /* Make sure the shield is absorptive, unless we must impact it anyway */
   if(!(traj_flags & SC_TRAJ_HIT_SHIELDS) && SC_ACCESSORY_SHIELD_IS_MAGNETIC(p->shield->info))
      return(false);

   /* A player's own shield is disallowed from stopping their missiles.
      I guess you've gotta let them past in order to fire them after all... */
   if(owner != NULL && p->index == owner->index) return(false);

   /* Compute the distance to the missile's present and future. */
   if(!sc_land_calculate_deltas_d(c->land, &dx, &dy, (double)p->x, (double)p->y, x, y) ||
      !sc_land_calculate_deltas_d(c->land, &nextdx, &nextdy, (double)p->x, (double)p->y, nextx, nexty))
      return(false);

   /* Square radii, whee... */
   rad2 = SQR(dx) + SQR(dy);
   nextrad2 = SQR(nextdx) + SQR(nextdy);

   /* We must start outside the tank's shield range, and end up inside it to impact it. */
   return((rad2 >= (double)SQR(p->tank->radius + 1)) && (nextrad2 < (double)SQR(p->tank->radius + 1)));

}



bool sc_shield_absorb_hit(sc_player *p, bool sapper) {
/* sc_shield_absorb_hit
   When a shield takes a direct hit. */

   /* Don't accept no substitutes! */
   if(p->shield == NULL) {
      return(false);
   }

   /* Shield sappers take more life out of a shield. */
   if(sapper) {
      p->shield->life -= SC_SHIELD_ABSORB_HIT * SC_SHIELD_SAPPER_RATE;
   } else {
      p->shield->life -= SC_SHIELD_ABSORB_HIT;
   }

   /* If the shield was completely obliterated, get rid of it. */
   if(p->shield->life <= 0) {
      sc_shield_free(&p->shield);
   }

   return(true);

}



int sc_shield_absorb_explosion(sc_player *p, const sc_explosion *e, int damage) {
/* sc_shield_absorb_explosion
   Try to absorb damage into a player's shield.  The damage undealt
   is returned, or zero if all damage absorbed.  */

   /* Must have a shield for it to take damage */
   if(p->shield == NULL) return(damage);

   /* Find out how much of the damage it took */
   p->shield->life -= damage;
   if(p->shield->life <= 0) {
      damage = -p->shield->life;
      sc_shield_free(&p->shield);
      if(e->type == SC_EXPLOSION_NAPALM) {
         damage = 0;
      }
   } else {
      damage = 0;
   }

   /* Return damage to the actual tank */
   return(damage);

}



bool sc_shield_get_deflection(sc_config *c, const sc_player *owner, int traj_flags, double x, double y, double *vx, double *vy) {
/* sc_shield_get_deflection
   Find the total deflection in velocity of a missile by magnetic shields. */

   double dx, dy, sdist, force;
   bool changed = false;
   sc_player *p;
   int index;

   /* Look at each player. */
   for(index = 0; index < c->numplayers; ++index) {
      p = c->players[index];

      /* There are THREE continue statements below. */

      /* A player's mag shield is tuned so as not to affect his own weapons. */
      if(p == NULL || p->index == owner->index)
         continue;  /* NOTE this is a flow control point! */

      /* Determine whether this player has a magnetic shield. */
      if(p->shield == NULL || !SC_ACCESSORY_SHIELD_IS_MAGNETIC(p->shield->info))
         continue;  /* NOTE this is a flow control point! */

      /* Try and get a proper distance estimate. */
      if(!sc_land_calculate_deltas_d(c->land, &dx, &dy, (double)p->x, (double)p->y, x, y))
         continue;  /* This is a rare (perhaps nonexistant) failure case. */

      /* Determine whether the tank is close enough, but not too close. */
      sdist = SQR(dx) + SQR(dy);
      if(sdist < SQR(SC_SHIELD_MAG_MAX_DIST) && sdist > SQR(p->tank->radius)) {
         /* The weapon and tank are in range, so push the weapon. */
         changed = true;

         /*
            Find the force to be imparted to the missile, and scale it.
            We avoid using the evil sqrt() function by performing most
            of the calculations here working with squares.

            This is where the LORC reside.  :)  To be specific:

            o  The repulsion strength is p->shield->info->repulsion.
            o  The repulsion strength must be attenuated by distance.
               To do this, we use the square of the distance, which
               is stored in sdist, and the attenuation rate constant,
               from sshield.h, SC_SHIELD_MAG_ATTENUATION.
            o  Finally we scale the result to units of tank power with
               sshield.h's SC_SHIELD_MAG_TO_POWER constant, and from
               there to velocity units with SC_PHYSICS_VELOCITY_SCL,
               from sphysics.h.

            The result will be multiplied by the x or y distance,
            and then divided by the total distance, in order to get
            velocity differentials which can then be added into the
            current weapon velocities, *vx and *vy.
         */
         force  = p->shield->info->repulsion;
         force *= (double)SQR(SC_SHIELD_MAG_ATTENUATION) / sdist;
         force *= (double)(SC_SHIELD_MAG_TO_POWER * SC_PHYSICS_VELOCITY_SCL);

         /* Set the new partial velocities, accelerated by the magnetic shield. */
         *vx += force * dx / sqrt(sdist);
         *vy += force * dy / sqrt(sdist);

         /* Sap energy off the player's shield, based on the acceleration given. */
         if(!(traj_flags & SC_TRAJ_NO_MODIFY)) {
            p->shield->life -= rint(force / (double)(SC_PHYSICS_VELOCITY_SCL * SC_SHIELD_MAG_TO_COST));
            if(p->shield->life <= 0) {
               sc_shield_free(&p->shield);
            }
         }
      }
   }

   return(changed);

}



bool sc_shield_get_reflection(sc_config *c, sc_player *p, int traj_flags,
                              double *x, double *y, double *velx, double *vely) {
/* sc_shield_get_reflection
   Find the new reflection in velocity of a missile off a force shield. */

   double dx, dy, radial;

   /* This is only for force shields. */
   if(p->shield == NULL || !SC_ACCESSORY_SHIELD_IS_FORCE(p->shield->info))
      return(false);

   /* If calculate_deltas fails somehow, so do we. */
   if(!sc_land_calculate_deltas_d(c->land, &dx, &dy, (double)p->x, (double)p->y, (*x), (*y)))
      return(false);

   /*
      To perform a reflection, we must reverse the radial velocity
      component.

      First we calculate the radial component of the two vectors
      (the velocity of the impacting missile and the radial vector
      from the center of the tank to the impact at the shield).

      Second, we subtract out twice the radial velocity, to reflect
      the missile off the shield surface at that point.

      The calculations skirt around the need for a sqrt() calculation
      by factoring a division by L = sqrt(SQR(dx) + SQR(dy)) outside
      of the radial, where it cancels in the final subtraction.
   */

   /* Find the radial velocity component through a dot product. */
   radial = (dx * (*velx) + dy * (*vely)) / (SQR(dx) + SQR(dy));

   /* Subtract 2x the perpendicular component to reverse it. */
   *velx -= 2 * radial * dx;
   *vely -= 2 * radial * dy;

   /* Move the weapon just (0.1 pixels) outside the shield (p->tank->radius + 1).
      It's difficult to get gcc to do this with the required precision.  :(  */
   *x = (double)p->x + dx * SQR((double)1.1 + (double)p->tank->radius) / (SQR(dx) + SQR(dy));
   *y = (double)p->y + dy * SQR((double)1.1 + (double)p->tank->radius) / (SQR(dx) + SQR(dy));

   /* Sap the shield. */
   if(!(traj_flags & SC_TRAJ_NO_MODIFY))
      sc_shield_absorb_hit(p, (traj_flags & SC_TRAJ_HIT_SHIELDS));

   return(true);

}
