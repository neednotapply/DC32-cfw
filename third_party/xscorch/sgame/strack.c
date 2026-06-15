/* $Header: /fridge/cvs/xscorch/sgame/strack.c,v 1.28 2011-08-01 00:01:41 jacob Exp $ */
/*

   xscorch - strack.c         Copyright(c) 2000-2003 Justin David Smith
                              Copyright(c) 2003      Jacob Luna Lundberg
   justins(at)chaos2.org      http://chaos2.org/
   jacob(at)gnifty.net        http://www.gnifty.net/

   Scorched basic weapon tracking


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
#include <stdlib.h>

#include <strack.h>
#include <scolor.h>
#include <sconfig.h>
#include <sexplosion.h>
#include <sland.h>
#include <sphoenix.h>
#include <sphysics.h>
#include <splayer.h>
#include <sshield.h>
#include <sspill.h>
#include <swindow.h>

#include <sutil/srand.h>



/***  Weapon tracking   ***/



static void _sc_traj_landfall(sc_trajectory *tr, int x) {
/* sc_traj_landfall
   This function is called with an x coordinate indicating that at least one
   land tile in column x was cleared.  A window is maintained indicating the
   minimum and maximum X coordinates where landfall might occur, to optimize
   performance during landfall.  A hint in tr is updated to indicate that
   landfall may occur in column x.  */

   /* Update the landfall for this trajectory */
   if(tr->landfall_x1 > x) tr->landfall_x1 = x;
   if(tr->landfall_x2 < x) tr->landfall_x2 = x;

}



static sc_trajectory_result _sc_weapon_track_point(sc_config *c, sc_trajectory *tr, void *data) {
/* sc_weapon_track_point
   This function takes a current trajectory tr, and a weapon (passed in
   using the extra data argument).  This function is called when a
   weapon passes through a particular coordinate; this function clears
   any land that may have occupied the coordinate, applies "smoke" to
   the land mask if necessary, and updates hints indicating landfall
   should occur and that the screen needs to be redrawn.

   This function takes the trajectory's current x, y coordinates and
   rounds them to the nearest integer values to determine what coordinate
   the weapon currently occupies.  It will update the land mask and hints
   in tr, but it will not adjust the weapon.

   This function has no effect if the weapon is out-of-bounds.  This
   will always return SC_TRAJ_CONTINUE, indicating tracking should
   proceed.  */

   const int *gradient;    /* Sky gradient */
   int gradientflag;       /* Sky gradient flag */
   bool dither;            /* Sky: dithering allowed? */
   bool repaint = false;   /* Is a repaint needed? */
   sc_weapon *wp = (sc_weapon *)data;  /* Weapon data */
   int *trace;             /* Trace location on land */
   int x;                  /* Weapon's current X */
   int y;                  /* Weapon's current Y */

   /* Get the current land pointer at (x, y) */
   x = rint(tr->curx);
   y = rint(tr->cury);
   if(!sc_land_translate_xy(c->land, &x, &y)) return(SC_TRAJ_CONTINUE);
   trace = SC_LAND_XY(c->land, x, y);

   /* Check if the weapon just clobbered some land... */
   if(SC_LAND_IS_GROUND(*trace)) {
      /* Get the sky gradient */
      gradient = sc_land_sky_index(c);
      gradientflag = sc_land_sky_flag(c);
      dither = c->graphics.gfxdither;
      *trace = gradientflag | sc_color_gradient_index(dither, gradient, y);
      _sc_traj_landfall(tr, x);
      repaint = true;
   } /* Obliterated some land? */

   /* Check if the weapon is leaving a smoke trail */
   if((c->weapons->tracepaths || SC_WEAPON_IS_SMOKING(wp)) && !SC_CONFIG_GFX_FAST(c)) {
      *trace = SC_LAND_SMOKE | wp->playerid;
      repaint = true;
   }

   if(repaint) {
      sc_window_paint(c->window, x, y, x, y, SC_REGENERATE_LAND | SC_PAINT_EVERYTHING);
   }

   /* Nothing interesting ever happens... */
   return(SC_TRAJ_CONTINUE);

}



static sc_weapon_track_result _sc_weapon_track(sc_config *c, sc_weapon **wp, sc_explosion **e) {
/* sc_weapon_track
   Track a single weapon component.  This function undraws the weapon at
   its current position ((*wp)->tr->cur{x,y}).  Then, it tracks the weapon
   to its new location, or to the point of impact if the weapon hit any-
   thing.  Impacts are dealt with, deleting the current weapon if needed,
   and adding an explosion to e.

   wp is a list; we are only tracking the weapon at the HEAD of the list
   but we may append extra warheads to the tail if needed (phoenix code
   will do this).  Similarly, e is a list; we may append new explosions
   to it at the tail of the list.

   A fair amount of phoenix processing is done here to determine if the
   weapon hit its apex and should break apart (for MIRVs, etc).  */

   sc_trajectory_result stepres;    /* Indicates result of sc_traj_step */
   sc_trajectory *traj;             /* Shortcut to (*wp)->traj. */
   bool goingup;                    /* True if weapon initially ascending */
   int flags;                       /* Flags for if we are tunnelling, etc */

   assert(c != NULL && wp != NULL && e != NULL);
   assert(*wp != NULL);

   /* We don't want to process this weapon now if it's deferring.
      Instead, we'll mark it to be processed next time and let the
      tracking state know it needs to keep processing the weapon. */
   if(SC_WEAPON_IS_DEFERRING(*wp)) {
      (*wp)->state &= ~SC_WEAPON_STATE_DEFER;
      return(SC_WEAPON_TRACK_NEED_RECURSE);
   }

   /* At this point, traj->cur{x,y} indicate the original control
      point; the point that the weapon was at upon entering this
      function. */
   traj = (*wp)->tr;

   sc_window_undraw_weapon(c->window, *wp);
   goingup = sc_traj_get_velocity_y(traj) >= 0;
   if(traj->timestep == 0) goingup = true;

   /* Are we tunnelling? This is important,
      as we must set the trajectory flags now! */
   flags = SC_WEAPON_TRAJ_FLAGS(c, *wp);

   /* WEAPON collision check;  since the weapon is likely to be skipping
      more than one pixel away per iteration, we need to make sure it
      wouldn't have collided with something along the way.  We need to
      check all points between the previous and current position.  We do
      this with a line approximation, which is well, good enough...  */
   stepres = sc_traj_step(c, traj, flags, _sc_weapon_track_point, *wp);

   /* At this point, the values traj->cur{x,y} either indicate the new
      control point, or the point of impact if the weapon hit something.  */

   /* Deal damage to shields if appropriate */
   if(stepres == SC_TRAJ_IMPACT_SHIELD && !SC_WEAPON_IS_LIQUID(*wp)) {
      sc_shield_absorb_hit(c->players[traj->victim], SC_WEAPON_IS_SAPPER(*wp) ? 1 : 0);
      stepres = SC_TRAJ_SIZZLE;
   }

   /* This code checks if we just detonated.  If we did, it checks whether
      the weapon is a LAND phoenix.  If so, it calls sc_phoenix() which
      checks which phoenix subroutine to run on the weapon. */
   if(SC_TRAJ_IS_IMPACT(stepres) && SC_PHOENIX_IS_AT_LAND((*wp)->weaponinfo)) {
      switch(sc_phoenix(SC_PHOENIX_AT_LAND, c, wp, e)) {
         /* Weapon hit something, exploded, and also created children */
         case SC_PHOENIX_DETONATE:
            sc_weapon_landfall(c, *wp);
            sc_weapon_free(wp);
            return(SC_WEAPON_TRACK_DETONATE);

         /* Weapon hit something and created children but we call it a sizzle
            because we don't detonate the original weapon.  Note that this is
            never done in the original Scorched Earth. */
         case SC_PHOENIX_SIZZLE:
            sc_weapon_landfall(c, *wp);
            sc_weapon_free(wp);
            return(SC_WEAPON_TRACK_SIZZLE);

         /* We need to skip tracking the weapon this turn */
         case SC_PHOENIX_RESET:
            /* Draw the weapon at its new position */
            sc_window_draw_weapon(c->window, *wp);
            /* Defer to next tracking round */
            return(SC_WEAPON_TRACK_NEED_RECURSE);

         /* There was an error (oom, for example) in sc_phoenix() */
         case SC_PHOENIX_FAILURE:

         /* The default case should not arise (we hope) */
         default:
            /* do nothing */;
      }
   }

   /* Decide what to do based on the return value of the path traversal */
   if(SC_TRAJ_IS_IMPACT(stepres)) {
      /* Weapon hit something, and detonated */
      if(*e == NULL) {
         sc_expl_add(e, sc_weapon_get_explosion(c, *wp, traj->curx, traj->cury, SC_EXPL_DEFAULT_DIR));
         sc_weapon_landfall(c, *wp);
         sc_weapon_free(wp);
         return(SC_WEAPON_TRACK_DETONATE);
      } else {
         /* Draw the weapon at its new position.  Probably not needed... */
         sc_window_draw_weapon(c->window, *wp);
         /* We deferred */
         return(SC_WEAPON_TRACK_NEED_RECURSE);
      }
   } else if(stepres == SC_TRAJ_SIZZLE) {
      /* Weapon sizzled */
      sc_weapon_landfall(c, *wp);
      sc_weapon_free(wp);
      return(SC_WEAPON_TRACK_SIZZLE);
   } /* Any special orders? */

   /* This code checks if we're at the apex of the weapon's flight curve.
      If we are and this is an APEX phoenix weapon, it calls sc_phoenix()
      which checks which phoenix function to run on the weapon.  For
      reasons of timing it is important to interrupt sc_weapon_track. */
   if(goingup && sc_traj_get_velocity_y(traj) <= 0 && SC_PHOENIX_IS_AT_APEX((*wp)->weaponinfo)) {
      switch(sc_phoenix(SC_PHOENIX_AT_APEX, c, wp, e)) {
         /* In this case the weapon split successfully */
         case SC_PHOENIX_SIZZLE:
            sc_weapon_landfall(c, *wp);
            sc_weapon_free(wp);
            return(SC_WEAPON_TRACK_SIZZLE);

         /* In this case, we detonated in mid-air.  Strange... */
         case SC_PHOENIX_DETONATE:
            sc_weapon_landfall(c, *wp);
            sc_weapon_free(wp);
            return(SC_WEAPON_TRACK_DETONATE);

         /* In this case something is probably very wrong, since phoenix failed */
         case SC_PHOENIX_FAILURE:

         /* In this case, nothing happened */
         case SC_PHOENIX_NO_ACTION:
         default:
            /* do nothing */;
      }
   }

   /* Check for RAND phoenix weapons and if so check whether now is the time to run them */
   /* TEMP: the probability should be selectable or perhaps scaled by the expected arc length? */
   if(SC_PHOENIX_IS_AT_RAND((*wp)->weaponinfo) && game_drand() < SC_PHOENIX_PROB_AT_RAND) {
      switch(sc_phoenix(SC_PHOENIX_AT_RAND, c, wp, e)) {
         /* In this case the weapon was successfully modified by sphoenix */
         case SC_PHOENIX_SIZZLE:
            sc_weapon_landfall(c, *wp);
            sc_weapon_free(wp);
            return(SC_WEAPON_TRACK_SIZZLE);

         /* In this case, we detonated */
         case SC_PHOENIX_DETONATE:
            sc_weapon_landfall(c, *wp);
            sc_weapon_free(wp);
            return(SC_WEAPON_TRACK_DETONATE);

         /* Failure in the sphoenix code */
         case SC_PHOENIX_FAILURE:

         /* No action to perform; we hope this case doesn't arise */
         case SC_PHOENIX_NO_ACTION:
         default:
            /* do nothing */;
      }
   }

   /* If we're still running, then the weapon is still live.  Go ahead
      and re-draw it at its new position. */
   sc_window_draw_weapon(c->window, *wp);

   /* We were tracking a weapon; we'll need to recurse. */
   return(SC_WEAPON_TRACK_NEED_RECURSE);

}



static inline sc_weapon_track_result _sc_weapon_track_chain(sc_config *c, sc_weapon **wp, sc_explosion **e) {
/* sc_weapon_track_chain
   Track a weapon chain for a single player.  This processes all
   weapons in the chain; it will return either:
      NO_ACTION, indicating that tracking has completed
      NEED_RECURSE, indicating that nothing of particular interest
         happened but tracking is not completed (we need to call
         this function again),
      DETONATE, indicating that at least one detonation occurred.
         Tracking will resume once the detonation is processed.

   All weapons in the weapon chain wp will be processed, even if some
   weapons at the head of the list explode, and any explosions are
   added to the explosions list e.  */

   sc_weapon_track_result result;

   assert(c != NULL && wp != NULL && e != NULL);

   result = SC_WEAPON_TRACK_NO_ACTION;
   while(*wp != NULL) {
      switch(_sc_weapon_track(c, wp, e)) {
         case SC_WEAPON_TRACK_DETONATE:
            result = SC_WEAPON_TRACK_DETONATE;
         case SC_WEAPON_TRACK_SIZZLE:
            /* In these cases, sc_weapon_track has already removed the
               current weapon from the queue, so wp is already updated
               to point to the next weapon. */
            break;

         case SC_WEAPON_TRACK_NEED_RECURSE:
            if(result == SC_WEAPON_TRACK_NO_ACTION) {
               result = SC_WEAPON_TRACK_NEED_RECURSE;
            } /* Only if nothing occurred yet */
         case SC_WEAPON_TRACK_NO_ACTION:
            /* Advance to the next weapon in the queue. */
            wp = &((*wp)->chain);

      }
   }
   return(result);

}



sc_weapon_track_result sc_weapon_track_all(sc_config *c, sc_explosion **e) {
/* sc_weapon_track_all
   Track the weapons of every player, in sync.  This function will always
   process weapons for all players; if at least one player created a new
   explosion, it will return DETONATE.  If at least one player still needs
   tracking assistance, then it will return NEED_RECURSE.  This function
   only returns NO_ACTION if ALL players are done tracking.  */

   /* Note: we process all weapons in the chain, even after seeing a
      detonation.  This is to prevent other weapons from ``lagging''
      when one weapon (i.e. chaos) starts a long chain of detonates.
      In other words, every weapon gets its fair timeslice even if
      there are a lot of explosions going on. */

   sc_weapon_track_result result;
   int i;

   assert(c != NULL && e != NULL);

   result = SC_WEAPON_TRACK_NO_ACTION;
   for(i = 0; i < c->numplayers; ++i) {
      switch(_sc_weapon_track_chain(c, &(c->players[i]->weapons), e)) {
         case SC_WEAPON_TRACK_DETONATE:
            result = SC_WEAPON_TRACK_DETONATE;
            break;

         case SC_WEAPON_TRACK_NEED_RECURSE:
            if(result == SC_WEAPON_TRACK_NO_ACTION) {
               result = SC_WEAPON_TRACK_NEED_RECURSE;
            } /* Only if nothing occurred yet */
            break;

         default:
            /* do nothing */;
      }
   }
   return(result);

}
