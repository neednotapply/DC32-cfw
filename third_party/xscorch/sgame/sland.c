/* $Header: /fridge/cvs/xscorch/sgame/sland.c,v 1.16 2011-07-31 18:29:19 jacob Exp $ */
/*

   xscorch - sland.c          Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Scorched Player land generation


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
#include <sland.h>
#include <scolor.h>
#include <sconfig.h>
#include <slscape.h>
#include <sphysics.h>
#include <splayer.h>
#include <stankpro.h>
#include <swindow.h>

#include <sutil/srand.h>



sc_land *sc_land_new(int width, int height, int flags) {
/* sc_land_new
   Setup a new landscape.  This function allocates a new land structure and
   initialises it with the flags and size given.  This function does not
   attempt to initialise the land; you must call sc_land_generate() yourself
   to create the land.  This function returns NULL if an error occurred.  */

   sc_land *l;    /* The newly allocated landscape, on success. */

   /* Allocate land pointer */
   l = (sc_land *)malloc(sizeof(sc_land));
   if(l == NULL) return(NULL);

   /* Setup the screens */
   l->screen = NULL;
   if(!sc_land_setup(l, width, height, flags)) {
      free(l);
      return(NULL);
   }

   /* Setup default land generation options */
   l->sky        = SC_SKY_NIGHT;
   l->hostileenv = false;
   l->bumpiness  = SC_LAND_BUMPINESS_DEF;
   l->generator  = SC_LAND_GEN_TRADITIONAL;

   /* Return the land structure */
   return(l);

}



bool sc_land_setup(sc_land *l, int width, int height, int flags) {
/* sc_land_setup
   Changes the size and flags of the land structure.  This function returns
   true on success, or false if something went wrong.  It should be assumed
   that the screen is uninitialised after this call.  */

   /* Assert the params given are valid */
   if(l == NULL || width <= 0 || height <= 0) return(false);

   /* Setup the new landscape */
   l->width  = width;
   l->height = height;
   l->flags  = flags;
   l->screen = (int *)realloc(l->screen, width * height * sizeof(int));

   /* Make sure the new screen is valid. */
   return(l->screen != NULL);

}



void sc_land_free(sc_land **l) {
/* sc_land_free
   Release a landscape structure, and the associated screens. */

   if(l == NULL || *l == NULL) return;
   free((*l)->screen);
   free(*l);
   *l = NULL;

}



bool sc_land_generate(const sc_config *c, sc_land *l) {
/* sc_land_generate
   Generate a new landscape.  The land is generated based on definitions in
   the configuration structure given, and is created using the profile
   generators.  If true is returned, then the land has been successfully
   generated.  If false is returned, the screen is still uninitialised.  */

   /* Land profile variables */
   double unit_x;    /* Column's x-coordinate within unit interval */

   /* Other variables */
   const int *gnd;   /* Ground indices for gradient */
   const int *sky;   /* Sky indices for gradient */
   int skyflag;      /* Sky type flag */
   int *lpointer;    /* Pointer into the screen. */
   int nearly_there; /* Current land height minus zero to two pixels */
   double height;    /* Current land height (Y) */
   bool dither;      /* Enable dithering? */
   int fheight;      /* Land height */
   int width;        /* Land width */
   int x;            /* Current column */
   int y;            /* Iterator down column */

   /* Sanity checks */
   if(c == NULL || l == NULL) return(false);

   /* Initialise landscapes */
   /* JS:  This must be called whenever land is generated, because it
      generates random data.  Random tables will be out-of-sync if
      this is called before network setup!!! */
   sc_lscape_init();

   /* Setup random sky if needed */
   if(l->sky == SC_SKY_RANDOM) l->realsky = game_lrand(SC_SKY_RANDOM);
   else l->realsky = l->sky;

   /* Setup ground and sky */
   gnd = c->colors->gradindex[SC_GRAD_GROUND];
   sky = sc_land_sky_index(c);
   skyflag = sc_land_sky_flag(c);
   dither = c->graphics.gfxdither;

   /* Create a profile for the landscape */
   sc_lscape_setup(c, l);

   /* Okay, here we go ... */
   fheight  = l->height;
   width    = l->width;
   x        = width;
   lpointer = SC_LAND_ORIGIN(l);
   while(x > 0) {
      /* current column's x-coordinate within the unit interval and
         evaluate the profile function at that point. */
      unit_x = (double)x / width;
      height = sc_lscape_eval(unit_x) * c->maxheight;
      nearly_there = (int)height - 2;

      /* Sanity checks */
      if(nearly_there > fheight) nearly_there = fheight;
      if(height > fheight) height = fheight;

      /* Start at the base of the hill, and fill in ground until we reach
         height.  The remaining part of the column should be filled in with
         the current sky.  */
      y = 0;
      while(y < nearly_there) {
         *lpointer = SC_LAND_GROUND | sc_color_gradient_index(dither, gnd, y);
         ++lpointer;
         ++y;
      }
      /* MH: Draw a thin rind in slightly different colour */
      while(y < height) {
         *lpointer = SC_LAND_GROUND | sc_color_gradient_index(dither, gnd, 0);
         ++lpointer;
         ++y;
      } /* Creating ground.. */
      while(y < fheight) {
         *lpointer = skyflag | sc_color_gradient_index(dither, sky, y);
         ++lpointer;
         ++y;
      } /* Creating sky.. */

      /* Next column */
      --x;
   } /* Iterating through each column */

   return(true);

}



int sc_land_flags(const sc_config *c) {
/* sc_land_flags
   Return the land flags appropriate for the physics model given. */

   sc_physics_walls walls;

   if(c == NULL) return(SC_LAND_DEFAULTS);
   walls = c->physics->walls;
   if(walls == SC_WALL_RANDOM) walls = game_lrand(SC_WALL_RANDOM);
   switch(c->physics->walls) {
      case SC_WALL_NONE:
         return(SC_LAND_DEFAULTS);
      case SC_WALL_WRAP:
         return(SC_LAND_WRAP_X);
      default:
         return(SC_LAND_WALL_X | SC_LAND_CEILING);
   } /* End switch */

}



static inline bool _sc_land_translate_x(const sc_land *l, int *x) {
/* _sc_land_translate_x
   Translates the X coordinate indicated.  This will translate, taking into
   consideration the wall types.  It will return true if the coordinate
   could be translated to a "proper" value (or if the coordinate was
   off-screen on an unbounded boundary), caution the coordinate may NOT
   necessarily be in-bounds.  The function returns false if the coordinate
   could not be translated (coordinate was inside a wall), in which case X
   is set to the nearest valid X coordinate.  */

/* TEMP NOTE - The above documentation is INCORRECT.  The weapon WILL be
   in-bounds if the returned value is true.  If the coordinate is off-
   screen, false will be returned.  This needs audited and updated docs. */

   /* NOTE: changes to this code should also be made to the double code! */

   if(l->flags & SC_LAND_WRAP_X) {
      /* X boundaries wrap-around */
      while(*x >= l->width) *x -= l->width;
      while(*x < 0) *x += l->width;
      return(true);
   } else if(l->flags & SC_LAND_WALL_X) {
      /* X boundaries are solid walls */
      if(*x >= l->width) {
         *x = l->width - 1;
         return(false);
      } else if(*x < 0) {
         *x = 0;
         return(false);
      } /* Is coordinate out-of-bounds? */
      return(true);
   } else {
      /* No boundaries at all. */
      if(*x >= l->width) return(false);
      if(*x < 0) return(false);
      return(true);
   } /* What is X boundary? */

}



static inline bool _sc_land_translate_x_d(const sc_land *l, double *x) {
/* _sc_land_translate_x_d
   This function is identical to above, but uses doubles.  */

   /* NOTE: changes to this code should also be made to the int code! */

   if(l->flags & SC_LAND_WRAP_X) {
      /* X boundaries wrap-around */
      while(*x >= l->width) *x -= l->width;
      while(*x < 0) *x += l->width;
      return(true);
   } else if(l->flags & SC_LAND_WALL_X) {
      /* X boundaries are solid walls */
      if(*x >= l->width) {
         *x = l->width - 1;
         return(false);
      } else if(*x < 0) {
         *x = 0;
         return(false);
      } /* Is coordinate out-of-bounds? */
      return(true);
   } else {
      /* No boundaries at all. */
      if(*x >= l->width) return(false);
      if(*x < 0) return(false);
      return(true);
   } /* What is X boundary? */

}



bool sc_land_translate_x(const sc_land *l, int *x) {
/* sc_land_translate_x
   Public interface to above, with sanity checks.  */

   if(l == NULL || x == NULL) return(false);
   return(_sc_land_translate_x(l, x));

}



bool sc_land_translate_x_d(const sc_land *l, double *x) {
/* sc_land_translate_x_d
   Public interface to above, with sanity checks.  */

   if(l == NULL || x == NULL) return(false);
   return(_sc_land_translate_x_d(l, x));

}



static inline bool _sc_land_translate_y(const sc_land *l, int *y) {
/* _sc_land_translate_y
   Translates the Y coordinate indicated.  This will translate, taking into
   consideration the wall types.  It will return true if the coordinate
   could be translated to a "proper" value (or if the coordinate was
   off-screen on an unbounded boundary), caution the coordinate may NOT
   necessarily be in-bounds.  The function returns false if the coordinate
   could not be translated (coordinate was inside a wall), in which case X
   is set to the nearest valid Y coordinate.  */

/* TEMP NOTE - The above documentation is INCORRECT.  The weapon WILL be
   in-bounds if the returned value is true.  If the coordinate is off-
   screen, false will be returned.  This needs audited and updated docs. */

   /* NOTE: changes to this code should also be made to the double code! */

   if(l->flags & SC_LAND_CEILING) {
      /* Land has a ceiling */
      if(*y >= l->height) {
         *y = l->height - 1;
         return(false);
      } else if(*y < 0) {
         *y = 0;
         return(false);
      } /* Is Y invalid? */
      return(true);
   } else {
      /* Open skies */
      if(*y < 0) {
         *y = 0;
         return(false);
      } else if(*y >= l->height) {
         return(false);
      } /* Is Y invalid? */
      return(true);
   } /* End of switch */

}



static inline bool _sc_land_translate_y_d(const sc_land *l, double *y) {
/* _sc_land_translate_y_d
   This function is identical to above, but uses doubles.  */

   /* NOTE: changes to this code should also be made to the int code! */

   if(l->flags & SC_LAND_CEILING) {
      /* Land has a ceiling */
      if(*y >= l->height) {
         *y = l->height - 1;
         return(false);
      } else if(*y < 0) {
         *y = 0;
         return(false);
      } /* Is Y invalid? */
      return(true);
   } else {
      /* Open skies */
      if(*y < 0) {
         *y = 0;
         return(false);
      } else if(*y >= l->height) {
         return(false);
      } /* Is Y invalid? */
      return(true);
   } /* End of switch */

}



bool sc_land_translate_y(const sc_land *l, int *y) {
/* sc_land_translate_y
   Public interface to above, with sanity checks.  */

   if(l == NULL || y == NULL) return(false);
   return(_sc_land_translate_y(l, y));

}



bool sc_land_translate_y_d(const sc_land *l, double *y) {
/* sc_land_translate_y_d
   Public interface to above, with sanity checks.  */

   if(l == NULL || y == NULL) return(false);
   return(_sc_land_translate_y_d(l, y));

}



bool sc_land_translate_xy(const sc_land *l, int *x, int *y) {
/* sc_land_translate_xy
   Translate an x/y coordinate. */

   return(sc_land_translate_x(l, x) && sc_land_translate_y(l, y));

}



bool sc_land_translate_xy_d(const sc_land *l, double *x, double *y) {
/* sc_land_translate_xy_d
   Translate an x/y coordinate. */

   return(sc_land_translate_x_d(l, x) && sc_land_translate_y_d(l, y));

}



bool sc_land_overlap_x(const sc_land *l, int *x1, int *x2) {
/* _sc_land_overlap_x
   Determines if the X range described by [x1, x2] would overlap a wrapping
   boundary.  This function behaves rather oddly; usually the center of the
   interval was a valid real coordinate, and x1, x2 are obtained by
   calculating some fixed offset from that valid real coordinate.  If an
   overlap on a boundary is detected, then the interval is modified to
   indicate the interval that was `wrapped', i.e. the interval part that was
   excluded because it was not in the valid real range.  False is returned
   if the boundaries aren't wrapping, or if no translation is required (x1,
   x2 are in the valid range for real coordinates.  This function may have
   odd semantics if at least one of the coordinates was not anchored in the
   valid real range.  The semantics should probably be documented better
   than this.  This is primarily used to determine the range of update for
   drawing functions, where the semantics are odd but understood.

   CAUTION: the interval must not be wider than the screen.
   CAUTION: at least part of the interval must be valid real coords.

   You probably only want to give this function intervals that have been
   through the sc_land_translate_x_range() function, first!!!  */

   /* Sanity check */
   if(l == NULL || x1 == NULL || x2 == NULL) return(false);

   if(l->flags & SC_LAND_WRAP_X) {
      /* X boundaries wrap-around */
      if(*x1 < 0) {
         /* Wrap on -x side, return interval part that was truncated
            because it wrapped to the right-hand-side */
         *x1 += l->width;
         *x2  = l->width - 1;
         return(true);
      } else if(*x2 >= l->width) {
         /* Wrap on +x side, return interval part that was truncated
            because it wrapped to the left-hand-side */
         *x2 -= l->width;
         *x1  = 0;
         return(true);
      }
   }
   return(false);

}



bool sc_land_translate_x_range(const sc_land *l, int *x1, int *x2) {
/* _sc_land_translate_x_range
   Translates a range of virtual X coordinates s.t. at least part of the
   range is in the real coordinate space.  On bounded rooms, the interval
   will be truncated to the part that is inside the bound, true is returned
   if the result is nonempty.  On wraparound rooms, the interval is adjusted
   so that at least part of it is in the real coordinate range -- in this
   case, you will need to run sc_land_overlap_x() to see if part of the
   interval is truncated by the wrapping boundary (and get that part
   translated).  True is again, returned if the resulting interval is non-
   empty.  The interval is considered empty if x1 > x2.  */

   /* Sanity check */
   if(l == NULL || x1 == NULL || x2 == NULL) return(false);

   /* Interval is empty if x1 > x2. */
   if(*x1 > *x2) return(false);

   if(l->flags & SC_LAND_WRAP_X) {
      /* X boundaries wrap-around */
      while(*x1 >= l->width) {
         /* Adjust so x2 is in real */
         *x2 -= l->width;
         *x1 -= l->width;
      }
      while(*x2 < 0) {
         /* Adjust so x1 is in real */
         *x1 += l->width;
         *x2 += l->width;
      }
      /* Return success */
      return(true);
   } else {
      /* Bounded; truncate as needed */
      if(*x2 >= l->width) *x2 = l->width - 1;
      if(*x1 >= l->width) return(false);
      if(*x2 < 0) return(false);
      if(*x1 < 0) *x1 = 0;
      return(true);
   }

}



bool sc_land_calculate_deltas(const sc_land *l, int *deltax, int *deltay, int x1, int y1, int x2, int y2) {
/* sc_land_calculate_deltas
   Calculates the short-path deltaX, deltaY between two virtual coordinates,
   taking boundaries into effect. This fixes issues with delta calculation
   that arise when wrapping boundaries are used.  In all other cases, the
   delta calculated is the expected delta after the coordinates are
   translated to real coordinates (so the screen delta, in effect).  Returns
   false if the translations failed for some reason.  If x2 > x1, then dx
   will be positive.  CAUTION:  This function does NOT concern itself with
   points that are out-of-bounds, this is to allow explosions centered off
   the screen to work properly.  As a result, if translation fails, then
   success still occurs!  */

   /* NOTE: changes to this code should also be made to the double code. */

   int dx;        /* Normal DX */
   int dy;        /* Normal DY */
   int d2x;       /* Alternate DX */

   /* Sanity check */
   if(l == NULL) return(false);

   /* translate the coordinates */
   _sc_land_translate_x(l, &x1);
   _sc_land_translate_x(l, &x2);
   _sc_land_translate_y(l, &y1);
   _sc_land_translate_y(l, &y2);

   /* Calculate the `normal' deltas */
   dx = x2 - x1;
   dy = y2 - y1;

   /* If land is wrapping, then calculate an alternate */
   if(l->flags & SC_LAND_WRAP_X) {
      d2x = dx + l->width;
      if(abs(d2x) < abs(dx)) dx = d2x;
   }

   /* Set pointers and return success. */
   if(deltax != NULL) *deltax = dx;
   if(deltay != NULL) *deltay = dy;
   return(true);

}



bool sc_land_calculate_deltas_d(const sc_land *l, double *deltax, double *deltay, double x1, double y1, double x2, double y2) {
/* sc_land_calculate_deltas_d
   Same as above, but uses double values.  */

   /* NOTE: changes to this code should also be made to the double code. */

   double dx;        /* Normal DX */
   double dy;        /* Normal DY */
   double d2x;       /* Alternate DX */

   /* Sanity check */
   if(l == NULL) return(false);

   /* translate the coordinates */
   _sc_land_translate_x_d(l, &x1);
   _sc_land_translate_x_d(l, &x2);
   _sc_land_translate_y_d(l, &y1);
   _sc_land_translate_y_d(l, &y2);

   /* Calculate the `normal' deltas */
   dx = x2 - x1;
   dy = y2 - y1;

   /* If land is wrapping, then calculate an alternate */
   if(l->flags & SC_LAND_WRAP_X) {
      d2x = dx + l->width;
      if(fabs(d2x) < fabs(dx)) dx = d2x;
   }

   /* Set pointers and return success. */
   if(deltax != NULL) *deltax = dx;
   if(deltay != NULL) *deltay = dy;
   return(true);

}



static inline int _sc_land_height(const sc_land *l, int x, int y0) {
/* sc_land_height
   This is an internal function that assumes x, y0 are real coordiantes.
   This function will return a maximum of y0; if you want to scan the entire
   range, give the screen height for y0.  */

   const int *lp;       /* Pointer into land structure. */

   /* Get the land pointer for this column. */
   lp = SC_LAND_XY(l, x, y0);
   while(y0 > 0) {
      if(!SC_LAND_IS_SKY(*lp)) return(y0 + 1);
      --lp;
      --y0;
   } /* End translation valid? */

   /* No land here; we hit the ground. */
   return(0);

}



int sc_land_height(const sc_land *l, int x, int y0) {
/* sc_land_height
   This function returns the height of the land in virtual column x.  The
   height of the lowest non-land pixel will be returned by this function.
   This function only considers land below and at the height y0, so the
   range of return values is from 0 to y0 + 1.  To check the height of an
   entire column, give the screen height for y0.  Caution: y0 is a height,
   not a virtual coordinate.  */

   /* Sanity check */
   if(l == NULL) return(0);

   /* Make sure the height is properly bounded. */
   if(y0 >= l->height) y0 = l->height - 1;

   /* translate the X coordinate to a real coordinate. */
   if(!_sc_land_translate_x(l, &x)) return(0);
   return(_sc_land_height(l, x, y0));

}



int sc_land_height_around(const sc_land *l, int x, int y0, int w) {
/* sc_land_height_around
   This function returns the maximum land height (see sc_land_height),
   for virtual x coordinates in the interval [x - w, x + w].  */

   int height;          /* Maximum height of land found */
   int ch;              /* Height of current column */
   int cx;              /* Current X coordinate (translated) */
   int x1;              /* First virtual X to check */
   int x2;              /* Last virtual X to check */

   /* Sanity check */
   if(l == NULL) return(0);

   /* Calculate the virtual interval */
   x1 = x - w;
   x2 = x + w;

   /* Make sure the maximum height is bounded */
   if(y0 >= l->height) y0 = l->height - 1;

   /* Start searching... */
   height = 0;
   for(x = x1; x <= x2; ++x) {
      /* Attempt to translate X to a real coordinate */
      cx = x;
      if(_sc_land_translate_x(l, &cx)) {
         /* Check the height for this column */
         ch = _sc_land_height(l, cx, y0);
         if(ch > height) height = ch;
      } /* End translation valid? */
   } /* End loop through interval */

   /* Return the maximum height found. */
   return(height);

}



int sc_land_min_height_around(const sc_land *l, int x, int y0, int w) {
/* sc_land_min_height_around
   Returns the MINIMUM height around an interval.  Similar to the
   above function in approach.  */

   int height;          /* Maximum height of land found */
   int ch;              /* Height of current column */
   int cx;              /* Current X coordinate (translated) */
   int x1;              /* First virtual X to check */
   int x2;              /* Last virtual X to check */

   /* Sanity check */
   if(l == NULL) return(0);

   /* Calculate the virtual interval */
   x1 = x - w;
   x2 = x + w;

   /* Make sure the maximum height is bounded */
   if(y0 >= l->height) y0 = l->height - 1;

   /* Start searching... */
   height = 0;
   for(x = x1; x <= x2; ++x) {
      /* Attempt to translate X to a real coordinate */
      cx = x;
      if(_sc_land_translate_x(l, &cx)) {
         /* Check the height for this column */
         ch = _sc_land_height(l, cx, y0);
         if(ch < height) height = ch;
      } /* End translation valid? */
   } /* End loop through interval */

   /* Return the minimum height found. */
   return(height);

}



int sc_land_avg_height_around(const sc_land *l, int x, int y0, int w) {
/* sc_land_avg_height_around
   Returns the average height of the land over a virtual interval.
   Similar to the previous two functions in approach.  */

   int count;           /* Number of heights found */
   int sum;             /* Sum of heights found */
   int cx;              /* Current X coordinate (translated) */
   int x1;              /* First virtual X to check */
   int x2;              /* Last virtual X to check */

   /* Sanity check */
   if(l == NULL) return(0);

   /* Calculate the virtual interval */
   x1 = x - w;
   x2 = x + w;

   /* Make sure the maximum height is bounded */
   if(y0 >= l->height) y0 = l->height - 1;

   /* Start searching... */
   sum = 0;
   count = 0;
   for(x = x1; x <= x2; ++x) {
      /* Attempt to translate X to a real coordinate */
      cx = x;
      if(_sc_land_translate_x(l, &cx)) {
         /* Add the height for this column */
         sum += _sc_land_height(l, cx, y0);
         ++count;
      } /* End translation valid? */
   } /* End loop through interval */

   /* Return the average of heights found. */
   return(sum / count);

}



static inline void _sc_land_level(const sc_config *c, sc_land *l, int x, int ht) {
/* sc_land_level
   Internal function to level a column of land to the specified height.
   Caution, X is a real coordinate in this funciton, no sanity checks run.  */

   const int *gnd;   /* Ground indices for gradient */
   const int *sky;   /* Sky indices for gradient */
   int skyflag;      /* Sky type flag */
   bool dither;      /* Enable dithering? */
   int *lp;          /* Iterate through land */
   int height;       /* Current height */

   /* Set the current height to screen height */
   height = l->height - 1;

   /* Obtain the gradients we will be using */
   gnd = c->colors->gradindex[SC_GRAD_GROUND];
   sky = sc_land_sky_index(c);
   skyflag = sc_land_sky_flag(c);
   dither = c->graphics.gfxdither;

   /* Start at the top, clear everything above and including ht. */
   lp = SC_LAND_XY(l, x, height);
   while(height >= ht) {
      if(!SC_LAND_IS_SKY(*lp)) *lp = skyflag | sc_color_gradient_index(dither, sky, height);
      --lp;
      --height;
   } /* Clear to sky */
   /* Continue, this time filling everything below with ground. */
   while(height >= 0) {
      if(!SC_LAND_IS_GROUND(*lp)) *lp = SC_LAND_GROUND | sc_color_gradient_index(dither, gnd, height);
      --lp;
      --height;
   } /* Clear to ground. */

}



void sc_land_level_around(const sc_config *c, sc_land *l, int x, int w, int ht) {
/* sc_land_level_around
   This function levels the land to a uniform height, ht.  This function
   levels the virtual interval [x - w, x + w], inclusive.  No values are
   returned, this function silently exits on failure.  This function is
   useful for leveling a platform to place a tank on.  */

   int cx;           /* Translated current X coordinate. */
   int x1;           /* Left virtual X coordiante */
   int x2;           /* Right virtual X coordinate */

   /* Sanity checks */
   if(c == NULL || l == NULL) return;

   /* Calculate the left, right bounds of the interval */
   x1 = x - w;
   x2 = x + w;

   /* Level the columns in the interval. */
   cx = 0;
   for(x = x1; cx <= x2; ++x) {
      cx = x;
      if(_sc_land_translate_x(l, &cx)) {
         /* Coordinate translated, level it. */
         _sc_land_level(c, l, cx, ht);
      } /* End translation valid? */
   } /* End interval loop */

}



static inline bool _sc_land_passable_opponent(const sc_config *c, const sc_player *p, int x, int y) {
/* sc_land_passable_opponent
   Returns true if the land at virtual (x, y) is passable, considering only
   p's land profile.  This is an internal function with no sanity checks.  */

   int radius;          /* Radius of player profile */
   int width;           /* Total width of player profile */
   int dx;              /* Delta x from player to x */
   int dy;              /* Delta y from player to y */

   /* Determine width of profile */
   radius = p->tank->radius;
   width = radius + radius + 1;

   /* Calculate deltas */
   if(!sc_land_calculate_deltas(c->land, &dx, &dy, x, y, p->x, p->y)) return(false);
   dx = radius + dx;
   dy = radius - dy;

   /* If delta is in profile, check if profile is opaque or not. */
   if(dx >= 0 && dx < width && dy >= 0 && dy <= radius) {
      if(*(p->tank->data + dx + width * dy) != SC_TANK_PROFILE_CLEAR) {
         /* Not passable */
         return(false);
      } /* Check profile data */
   } /* Check if in profile bounding box */

   /* Pixel is passable. */
   return(true);

}



static inline bool _sc_land_passable_point(const sc_config *c, const sc_player *p, int x, int y) {
/* sc_land_passable_point
   Returns true if the virtual point (x, y) is passable, considering the
   land and player profiles.  If p is non-NULL, that player's profile will
   be excluded when considering profiles.  Otherwise, all profiles are
   considered.  This is an internal function that does not check sanity.  */

   int i;               /* Iterator */

   /* Check player profiles */
   for(i = 0; i < c->numplayers; ++i) {
      /* Make sure player is live, and not == p */
      if(c->players[i] != p && SC_PLAYER_IS_ALIVE(c->players[i])) {
         /* Check if player profile obscures this point */
         if(!_sc_land_passable_opponent(c, c->players[i], x, y)) return(false);
      } /* Checking player */
   } /* Loop through all players */

   /* Check land to make sure it is clear */
   if(!sc_land_translate_xy(c->land, &x, &y)) return(false);
   if(!SC_LAND_IS_SKY(*SC_LAND_XY(c->land, x, y))) return(false);

   /* Point is passable. */
   return(true);

}



bool sc_land_passable_point(const sc_config *c, const sc_player *p, int x, int y) {
/* sc_land_passable_point
   Same as above, but with sanity checks.  */

   if(c == NULL) return(false);
   return(_sc_land_passable_point(c, p, x, y));

}



static int _sc_land_support_shift_right(const sc_config *c, const sc_land *l, int x, int y, int r, int s) {
/* sc_land_support_shift_right
   Returns the offset the tank may shift to the RIGHT if it is not properly
   supported in that direction.  The tank is at coordinates (x,y+1) with a
   base radius of r.  If the tank is sufficiently supported to the RIGHT,
   then 0 is returned.  (x, y) may be virtual coordinates.  */

   int sx;     /* Start X -- first X index/coord which is clear below */
   int fx;     /* Final X -- last X coord which must be clear to slide */
   int cx;     /* Current X index/coordinate */

   /* Make sure Y translates properly. */
   if(!_sc_land_translate_y(l, &y)) return(0);

   /* Check where the "edge" of the land is -- hopefully it
      is within the first SC_TANK_MIN_SHELF_SIZE pixels of
      the left edge; if not, then the tank cannot slide down. */
   cx = 0;     /* 0 is leftmost edge of tank, advancing right */
   sx = 0;     /* Set to one past the left edge (0=no land below) */
   while(cx < s - 1) {
      /* If translation fails, we assume the tank is supported. */
      if(!_sc_land_passable_point(c, NULL, x - r + cx, y)) sx = cx + 1;
      ++cx;
   } /* Scan for edge of land */

   /* CX is now an actual X coordinate, starting at the
      first suspected clear tile from the left edge.
      Calculate the final X coordinate which must be
      clear for the tank to slide.  */
   cx = sx + x - r;
   fx = x + sx + r + 1;

   /* Make sure everything between (x-r+sx) and fx is clear below */
   while(cx <= fx) {
      /* If translation fails, assume tank cannot be moved. */
      if(!_sc_land_passable_point(c, NULL, cx, y)) return(0);
      ++cx;
   }

   /* Everything was clear; tank may shift over sx pixels to the right */
   return(sx);

}



static int _sc_land_support_shift_left(const sc_config *c, const sc_land *l, int x, int y, int r, int s) {
/* sc_land_support_shift_left
   Returns the offset the tank may shift to the LEFT if it is not properly
   supported in that direction.  The tank is at coordinates (x,y+1) with a
   base radius of r.  If the tank is sufficiently supported to the left,
   then 0 is returned.  (x, y) may be virtual coordinates.  */

   int sx;     /* Start X -- first X index/coord which is clear below */
   int fx;     /* Final X -- last X coord which must be clear to slide */
   int cx;     /* Current X index/coordinate */

   /* Make sure Y translates properly. */
   if(!_sc_land_translate_y(l, &y)) return(0);

   /* Check where the "edge" of the land is -- hopefully it
      is within the first SC_TANK_MIN_SHELF_SIZE pixels of
      the left edge; if not, then the tank cannot slide down. */
   cx = 0;     /* 0 is rightmost edge of tank, advancing to the left */
   sx = 0;     /* Set to one before the right edge (0=no land below) */
   while(cx < s - 1) {
      /* If translation fails, assume tank is supported. */
      if(!_sc_land_passable_point(c, NULL, x + r - cx, y)) sx = cx + 1;
      ++cx;
   }

   /* CX is now an actual X coordinate, starting at the
      first suspected clear tile from the right edge,
      and decrementing down to FX.  */
   cx = x + r - sx;
   fx = x - sx - r - 1;

   /* Make sure everything between fx and (x+r-sx) is clear below */
   while(cx >= fx) {
      /* If translation fails, assume tank cannot be moved. */
      if(!_sc_land_passable_point(c, NULL, cx, y)) return(0);
      --cx;
   }

   /* Everything was clear; tank may shift over sx pixels to the right */
   return(sx);

}



int sc_land_support(const sc_config *c, const sc_land *l, int x, int y, int r, int s) {
/* sc_land_support
   Returns the offset the tank may be shifted over if it is not properly
   supported on one side.  The tank is at coordinates (x,y) with a base
   radius of r.  If the tank is sufficiently supported on both sides, then 0
   is returned.  Otherwise, a positive value implies the tank will slide
   down the slope to the right, and a negative value implies that the tank
   will slide down the slope to the left.  */

   int delta;  /* Distance to slide over */

   /* If already on the floor, the tank cannot slide any further */
   if(l == NULL || --y < 0) return(0);

   /* Check if the tank can slide in either direction */
   delta = _sc_land_support_shift_right(c, l, x, y, r, s);
   if(delta != 0) return(delta);/* Slide to the right */
   delta = _sc_land_support_shift_left(c, l, x, y, r, s);
   if(delta != 0) return(-delta);/* Slide to the left */

   /* Tank is sufficiently supported. */
   return(0);

}



static bool _sc_land_drop_column(const sc_config *c, sc_land *l, int x) {
/* sc_land_drop_column
   Drop the land in the current column, x. This is an internal function
   will no sanity checking; caution, X may be a virtual coordinate.  */

   const int *sky;   /* Sky indices for gradient */
   int height;       /* Land height */
   int skyflag;      /* Sky type flag */
   int maxdropallow; /* maximum drop allowed */
   int maxdrop;      /* Distance the land is being dropped */
   bool dither;      /* Enable land dithering? */
   int *lpdown;      /* Pointer to lower position; "falling to" */
   int *lpup;        /* Pointer to upper position; "falling from" */
   int miny;         /* Minimum height that needs to be redrawn */
   int maxy;         /* Maximum height that needs to be redrawn */
   int dy;           /* Amount of land that has actually fallen */
   int y;            /* Current Y coordinate */

   /* Make sure X coordinate is valid */
   if(!_sc_land_translate_x(l, &x)) return(false);

   /* Get and cache the sky attributes */
   sky = sc_land_sky_index(c);
   skyflag = sc_land_sky_flag(c);
   dither = c->graphics.gfxdither;

   /* Set initial Y coodinate and pointers */
   y = 0;
   height = l->height;
   maxdrop = 0;
   lpup = SC_LAND_XY(l, x, 0);

   /* Track down the screen until we find a cell that is not ground. */
   while(y < height && SC_LAND_IS_GROUND(*lpup)) {
      ++lpup;
      ++y;
   }
   lpdown = lpup; /* This is the point where land will fall _to_ */
   miny = y;      /* Minimum coordinate that will need redrawing */
   maxy = y;      /* Assume this is also the maximum coordinate. */

   /* Scan up until we find ground; note, that if everything
      from here on is sky, then there is actually no land to drop.  */
   while(y < height && !SC_LAND_IS_GROUND(*lpup)) {
      ++lpup;
      ++y;
   }

   /* If Y coordinate is out of range, then nothing to be done. */
   if(y >= height) return(false);

   /* Determine if we attempted to drop too far */
   maxdrop = lpup - lpdown;
   if(SC_CONFIG_NO_ANIM(c)) maxdropallow = c->fieldheight;
   else maxdropallow = SC_LAND_MAX_DROP_PER_CYCLE;
   if(maxdrop >= maxdropallow) {
      lpdown = lpup - maxdropallow;
      miny = y - maxdrop;
   }

   /* Start letting the land fall... */
   dy = SC_LAND_MAX_AMOUNT;
   while(y < height && dy >= 0) {
      if(SC_LAND_IS_GROUND(*lpup)) {
         *lpdown = *lpup;
         *lpup = skyflag | sc_color_gradient_index(dither, sky, y);
         maxy = y;
      }
      ++lpdown;
      ++lpup;
      --dy;
      ++y;
   }

   /* Repaint the range */
   sc_window_paint(c->window, x, miny, x, maxy, SC_REGENERATE_LAND | SC_PAINT_EVERYTHING);
   return(maxdrop >= maxdropallow || y < height);

}



bool sc_land_drop_zone(const sc_config *c, sc_land *l, int x1, int x2) {
/* sc_land_drop
   Drop the land in the columns x1 to x2 (inclusive).  If land still needs
   to be dropped after this call, then nonzero is returned.  X is a virtual
   coordinate.  */

   bool needrecurse; /* Nonzero if we need to recurse */
   int cx;           /* Current column we are operating on */

   /* Sanity check. */
   if(c == NULL || l == NULL) return(false);

   needrecurse = false;
   for(cx = x1; cx <= x2; ++cx) {
      /* CX may be virtual when given to drop_column. */
      needrecurse = _sc_land_drop_column(c, l, cx) || needrecurse;
   } /* End interval loop */

   /* Will user need to call this method again? */
   return(needrecurse);

}



bool sc_land_drop(const sc_config *c, sc_land *l, int x, int w) {
/* sc_land_drop
   Drop the land in the columns x-w to x+w.  If land still needs to be
   dropped after this call, then nonzero is returned.  X is a virtual
   coordinate.  */

   return(sc_land_drop_zone(c, l, x - w, x + w));

}



bool sc_land_line_of_sight(const sc_config *c, const sc_land *l, int x1, int y1, int x2, int y2) {
/* sc_land_line_of_sight
   Returns nonzero if (x1,y1) can see (x2,y2).  This function will take
   wrapping boundaries and solid walls into account, if they are available.
   x1, y1, x2, y2 are all virtual coordinates.  */

   double x;               /* Current X coordinate */
   double y;               /* Current Y coordinate */
   double stepx;           /* Step X, per iteration */
   double stepy;           /* Step Y, per iteration */
   int numsteps;           /* Total number of iterations */
   int deltax;             /* Total change in X */
   int deltay;             /* Total change in Y */
   int cx;                 /* Current X (rounded) */
   int cy;                 /* Current Y (rounded) */

   /* Sanity checks */
   if(c == NULL || l == NULL) return(false);

   /* Determine the distance between points */
   deltax = x2 - x1;
   deltay = y2 - y1;

   /* If (x1,y1) == (x2,y2), nothing to do! */
   if(deltax == 0 && deltay == 0) return(true);

   /* Which axis is the major axis?  The major axis will determine
      the total number of steps we will need to take.  */
   if(abs(deltax) > abs(deltay)) {
      /* X axis is the major axis. */
      numsteps = abs(deltax);
   } else {
      /* Y axis is the major axis. */
      numsteps = abs(deltay);
   } /* Which axis is major? */

   /* Start at (x1,y1). Setup stepping in each direction. */
   x = x1;
   y = y1;
   stepx = (double)deltax / numsteps;
   stepy = (double)deltay / numsteps;

   /* Iterate! If this iteration completes, then we have
      a direct line of sight between (x1,y1) and (x2,y2). */
   while(numsteps > 0) {
      /* Round current coordinates to nearest integer. */
      cx = rint(x);
      cy = rint(y);

      /* Translate the X, Y coordinates */
      if(_sc_land_translate_x(l, &cx) && _sc_land_translate_y(l, &cy)) {

         /* If we passed the ground, we're going through solid. */
         if(cy < 0) return(false);
         if(!SC_LAND_IS_SKY(*SC_LAND_XY(l, cx, cy))) {
            return(false);
         }

      } /* Translation valid? */

      /* Advance to the next coordinate to be checked. */
      x += stepx;
      y += stepy;
      --numsteps;
   }

   /* WE have a direct line-of-sight! */
   return(true);

}




void sc_land_clear_smoke(const sc_config *c, sc_land *l) {
/* sc_land_clear_smoke
   Clears the smoke off the land given. */

   const int *sky;   /* Sky indices for gradient */
   int skyflag;      /* Sky type flag */
   int dither;       /* Dither flags */
   int height;       /* Land height */
   int width;        /* Land width */
   int *lp;          /* Land pointer */
   int x;            /* Current real X */
   int y;            /* Current real Y */

   /* Sanity check */
   if(c == NULL || l == NULL) return;

   /* Obtain dithering information */
   sky = sc_land_sky_index(c);
   skyflag = sc_land_sky_flag(c);
   dither = c->graphics.gfxdither;

   /* Cache height and width locally, for efficiency */
   height = l->height;
   width  = l->width;

   /* Scan through all land, replacing smoke with sky */
   for(x = 0; x < width; ++x) {
      lp = SC_LAND_XY(l, x, 0);
      for(y = 0; y < height; ++lp, ++y) {
         if(SC_LAND_IS_SMOKE(*lp)) *lp = skyflag | sc_color_gradient_index(dither, sky, y);
      } /* Iterating thru Y */
   } /* Iterating thru X */

   /* Redraw the playfield */
   sc_window_paint(c->window, 0, 0, l->width, l->height, SC_REGENERATE_LAND | SC_PAINT_EVERYTHING);

}



void sc_land_create_dirt(const sc_config *c, sc_land *l, const int *xlist, const int *ylist, int size) {
/* sc_land_create_dirt
   This function creates dirt at each of the virtual coordinates in the
   lists of coordinates given.  xlist and ylist are list of (x, y) virtual
   coordinate pairs, and size indicates the size of the lists.  */

   bool dither;                     /* Dither flag */
   const int *gnd;                  /* Ground gradient */
   int boundx1 = c->fieldwidth;     /* Bound that was altered X1 */
   int boundy1 = c->fieldheight;    /* Bound that was altered Y1 */
   int boundx2 = 0;                 /* Bound that was altered X2 */
   int boundy2 = 0;                 /* Bound that was altered Y2 */
   int x;                           /* Current translated X */
   int y;                           /* Current translated Y */

   /* Sanity checks */
   if(c == NULL || l == NULL || xlist == NULL || ylist == NULL) return;

   /* Obtain dither information */
   dither = c->graphics.gfxdither;
   gnd = c->colors->gradindex[SC_GRAD_GROUND];

   /* Scan through the list of coordinates */
   while(size > 0) {
      /* translate this coordinate */
      x = *xlist;
      y = *ylist;
      if(_sc_land_translate_x(l, &x) && _sc_land_translate_y(l, &y)) {
         /* Create dirt at this coordinate */
         *SC_LAND_XY(l, x, y) = SC_LAND_GROUND | sc_color_gradient_index(dither, gnd, y);
         /* Update the real bounding box */
         if(x < boundx1) boundx1 = x;
         if(x > boundx2) boundx2 = x;
         if(y < boundy1) boundy1 = y;
         if(y > boundy2) boundy2 = y;
      } /* translation valid? */
      ++xlist;
      ++ylist;
      --size;
   } /* loop through list */

   /* Repaint the affected region */
   sc_window_paint(c->window, boundx1, boundy1, boundx2, boundy2, SC_REGENERATE_LAND | SC_PAINT_EVERYTHING);

}



void sc_land_clear_profile(const sc_config *c, sc_land *l, const sc_player *p) {
/* sc_land_clear_profile
   This function is a sanity check used by players, to make certain
   there is sky behind their profile.  */

   const unsigned char *data; /* Profile data */
   const int *sky;   /* Sky indices for gradient */
   int skyflag;      /* Sky type flag */
   int dither;       /* Dither flags */
   int radius;       /* profile radius */
   int width;        /* profile width */
   int tx;           /* Translated X */
   int ty;           /* Translated Y */
   int x;            /* Current offset X */
   int y;            /* Current offset Y */

   /* Sanity check */
   if(c == NULL || l == NULL || p == NULL) return;

   /* Obtain dithering information */
   sky = sc_land_sky_index(c);
   skyflag = sc_land_sky_flag(c);
   dither = c->graphics.gfxdither;

   /* Cache height and width locally, for efficiency */
   radius = p->tank->radius;
   width  = radius + radius + 1;

   /* Scan through all land, replacing profile points with sky */
   data = p->tank->data;
   for(y = radius; y >= 0; --y) {
      for(x = 0; x < width; ++x, ++data) {
         if(*data != SC_TANK_PROFILE_CLEAR) {
            tx = p->x + x - radius;
            ty = p->y + y;
            if(_sc_land_translate_x(l, &tx) && _sc_land_translate_y(l, &ty)) {
               *SC_LAND_XY(l, tx, ty) = skyflag | sc_color_gradient_index(dither, sky, ty);
            } /* Translation ok */
         } /* Point is in profile */
      } /* Loop X */
   } /* Loop Y */

}



const int *sc_land_sky_index(const sc_config *c) {
/* sc_land_sky_index
   Returns the list of sky gradient indices for the sky selected by
   the user.  Do not attempt to modify this array directly.  NULL is
   returned if an error occurred.  */

   /* Sanity check */
   if(c == NULL) return(NULL);

   /* Lookup the sky type. */
   switch(c->land->realsky) {
      case SC_SKY_NIGHT:
         return(c->colors->gradindex[SC_GRAD_NIGHT_SKY]);
      case SC_SKY_FIRE:
         return(c->colors->gradindex[SC_GRAD_FIRE_SKY]);
      case SC_SKY_RANDOM:
         return(NULL);
   } /* End switch */

   return(NULL);

}



int sc_land_sky_flag(const sc_config *c) {
/* sc_land_sky_flag
   Returns the sky flags for the sky selected by the user.  */

   /* Sanity check */
   if(c == NULL) return(0);

   /* Lookup the sky flags */
   switch(c->land->realsky) {
      case SC_SKY_NIGHT:
         return(SC_LAND_NIGHT_SKY);
      case SC_SKY_FIRE:
         return(SC_LAND_FIRE_SKY);
      case SC_SKY_RANDOM:
         return(0);
   } /* End switch */

   return(0);

}



/*** List of skies ***/
static const char *_sc_land_sky_names[] = {
   "Night",
   "Fire",
   "Random Sky",
   NULL
};
static const unsigned int _sc_land_sky_types[] = {
   SC_SKY_NIGHT,
   SC_SKY_FIRE,
   SC_SKY_RANDOM,
   0
};

const char **sc_land_sky_names(void) {

   return(_sc_land_sky_names);

}

const unsigned int *sc_land_sky_types(void) {

   return(_sc_land_sky_types);

}



/*** List of land generators ***/
static const char *_sc_land_generator_names[] = {
   "None",
   "Canyon",
   "Double Mountain",
   "Hillside",
   "Mountain",
   "Plains",
   "Traditional",
   "Valley",
   "Random Generator",
   NULL
};
static const unsigned int _sc_land_generator_types[] = {
   SC_LAND_GEN_NONE,
   SC_LAND_GEN_CANYON,
   SC_LAND_GEN_DOUBLE_MOUNTAIN,
   SC_LAND_GEN_HILLSIDE,
   SC_LAND_GEN_MOUNTAIN,
   SC_LAND_GEN_PLAINS,
   SC_LAND_GEN_TRADITIONAL,
   SC_LAND_GEN_VALLEY,
   SC_LAND_GEN_RANDOM,
   0
};

const char **sc_land_generator_names(void) {

   return(_sc_land_generator_names);

}

const unsigned int *sc_land_generator_types(void) {

   return(_sc_land_generator_types);

}



