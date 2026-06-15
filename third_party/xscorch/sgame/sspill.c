/* $Header: /fridge/cvs/xscorch/sgame/sspill.c,v 1.11 2011-08-01 00:01:41 jacob Exp $ */
/*

   xscorch - sspill.c         Copyright(c) 2000-2003 Justin David Smith
                              Copyright(c) 2003      Jacob Luna Lundberg
   justins(at)chaos2.org      http://chaos2.org/
   jacob(at)gnifty.net        http://www.gnifty.net/

   Scorched spillage functions (napalm, liquid dirt)


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

#include <sspill.h>
#include <scolor.h>
#include <sconfig.h>
#include <sland.h>
#include <swindow.h>

#include <sutil/srand.h>


static void _sc_spill_fill(const sc_config *c, const sc_land *l,
                           sc_spill *sp, int curx, int cury, bool allowup);



static bool _sc_spill_try_fill(const sc_config *c, const sc_land *l,
                               sc_spill *sp, int x, int y, bool allowup) {
/* sc_spill_try_fill
   Attempt to fill in a region of a spill.  If allowup is set, then
   if no alternative is available (we are in a valley) we are allowed
   to spill in an upward direction. Otherwise, spill to the left or
   right whenever possible.  True is returned if we were successful
   in spilling somewhere. */

   int i;

   /* Make sure this is a valid index and land location. */
   if(sp->index >= sp->size) {
      return(false);
   }
   if(!sc_land_translate_xy(l, &x, &y)) {
      return(false);
   }

   /* Check that this location is unoccupied. */
   if(SC_LAND_IS_SKY(*SC_LAND_XY(l, x, y))) {
      for(i = 0; i < sp->index; ++i) {
         if(x == sp->spillx[i] && y == sp->spilly[i]) {
            return(false);
         }
      } /* Scan that the current position unoccupied */

      /* We can spill here! */
      sp->spillx[sp->index] = x;
      sp->spilly[sp->index] = y;
      ++sp->index;

      /* Continue spilling... */
      _sc_spill_fill(c, l, sp, x, y, allowup);
      return(true);
   }

   /* We are on ground, cannot spill. */
   return(false);

}



static void _sc_spill_fill(const sc_config *c, const sc_land *l,
                           sc_spill *sp, int curx, int cury, bool allowup) {
/* sc_spill_fill
   Attempt to fill in a region of a spill.  This tries all directions from
   the current location in order to continue a spill; this assumes the current
   location has already been spilled (or is dirt).  */

   /* Randomly try to spill right/left first. */
   int randdir = game_lrand(2) * 2 - 1;

   /* Attempt spill below first; then try to right/left downward */
   _sc_spill_try_fill(c, l, sp, curx, cury - 1, false);
   _sc_spill_try_fill(c, l, sp, curx + randdir, cury - 1, false);
   _sc_spill_try_fill(c, l, sp, curx - randdir, cury - 1, false);

   /* Again attempt a spill, horizontally to right/left. */
   _sc_spill_try_fill(c, l, sp, curx + randdir, cury, false);
   _sc_spill_try_fill(c, l, sp, curx - randdir, cury, false);

   /* If we can go up and are forced to, then spill upward. Note:
      we can only spill upward from the center column, so this is
      the only case where allowup is set. */
   if(allowup) {
      _sc_spill_try_fill(c, l, sp, curx, cury + 1, true);
      _sc_spill_try_fill(c, l, sp, curx + randdir, cury + 1, true);
      _sc_spill_try_fill(c, l, sp, curx - randdir, cury + 1, true);
      /* TEMP JDS:  If napalm is no longer impacting deep in the earth
         then okay. But something occurs to me: it is POSSIBLE that a
         napalm weapon that tunnels will be buried by landfall before
         it gets a chance to spill.  */
      /* TEMP JTL:  It seems okay to me.  And I'm not sure the land-
         slide thing is a real issue for us.  People who don't want
         to risk it should have used contact triggers... */
   }

}



static void _sc_spill_fill_initial(const sc_config *c, const sc_land *l,
                                   sc_spill *sp, int curx, int cury) {
/* sc_spill_fill_initial
   Construct an entire spill. */

   /* Start a spill at the current location; allow upward spills. */
   _sc_spill_fill(c, l, sp, curx, cury - 1, true);

}



sc_spill *sc_spill_new(const sc_config *c, const sc_land *l,
                       int size, int centerx, int centery) {
/* sc_spill_new
   Create a new spill structure. This initialises the array that
   will describe the coordinates of the spill, in an order that is
   appropriate for animating the flow of the spill.  Specify the
   explosion coordinates (usually at the topmost layer of the
   ground) where the spill starts, and the number of pixels that
   should be flooded by the spill.  */

   sc_spill *sp;
   int *lp;
   int i;

   /* Sanity checks */
   assert(c != NULL);
   assert(l != NULL);
   assert(size >= 0);

   /* Allocate spill data structures */
   sp = (sc_spill *)malloc(sizeof(sc_spill));
   if(sp == NULL) {
      return(NULL);
   }
   sp->spillx = (int *)malloc(sizeof(int) * size);
   if(sp->spillx == NULL) {
      free(sp);
      return(NULL);
   }
   sp->spilly = (int *)malloc(sizeof(int) * size);
   if(sp->spilly == NULL) {
      free(sp->spillx);
      free(sp);
      return(NULL);
   }

   /* Make sure the centerx, centery coordinate is valid */
   if(!sc_land_translate_xy(l, &centerx, &centery)) {
      /* Offscreen and unable to compensate; the spill is NULL */
      sp->size = 0;
      sp->index = 0;
      sp->count = 0;
      for(i = 0; i < size; ++i) {
         sp->spillx[i] = -1;
         sp->spilly[i] = -1;
      }
      return(sp);
   }

   /* Initialise the spill structure */
   sp->size = size;
   sp->index = 0;
   sp->count = 0;
   for(i = 0; i < size; ++i) {
      sp->spillx[i] = centerx;
      sp->spilly[i] = centery;
   }

   /* Clear land data for the impact crater... */
   lp = SC_LAND_XY(l, centerx, centery);
   *lp = sc_land_sky_flag(c) |
         sc_color_gradient_index(c->graphics.gfxdither,
                                 sc_land_sky_index(c), centery);
   sc_window_paint(c->window, centerx, centery, 1, 1,
                   SC_REGENERATE_LAND | SC_PAINT_EVERYTHING);

   /* Compute the spill; note, we may not be able to spill all
      the requested number of pixels, therefore size must be
      updated after this computation to reflect the actual number
      of pixels flooded (not the requested number). */
   _sc_spill_fill_initial(c, l, sp, centerx, centery);
   sp->size = sp->index;
   sp->index = 0;

   /* Return the spill structure */
   return(sp);

}



void sc_spill_free(sc_spill **sp) {
/* sc_spill_free */

   if(sp == NULL || *sp == NULL) {
      return;
   }
   free((*sp)->spillx);
   free((*sp)->spilly);
   free(*sp);
   *sp = NULL;

}
