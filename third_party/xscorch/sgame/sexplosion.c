/* $Header: /fridge/cvs/xscorch/sgame/sexplosion.c,v 1.18 2011-08-01 00:01:41 jacob Exp $ */
/*

   xscorch - sexplosion.c     Copyright(c) 2000-2003 Justin David Smith
                              Copyright(c) 2000-2003 Jacob Luna Lundberg
                              Copyright(c) 2003 Jason House (wedge code)
   justins(at)chaos2.org      http://chaos2.org/
   jacob(at)gnifty.net        http://www.gnifty.net/

   Scorched zone explosions


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

#include <sexplosion.h>    /* Explosion header */
#include <scolor.h>        /* Need colormap information */
#include <sconfig.h>       /* Need to get colormap, land */
#include <sland.h>         /* We clear the land */
#include <sphysics.h>      /* Need current physics model */
#include <sspill.h>        /* Constructs spillage/napalm */
#include <swindow.h>       /* For expl_cache drawing functions */



sc_explosion *sc_expl_new(int centerx, int centery, int radius, int force,
                          int playerid, sc_explosion_type type) {
/* sc_expl_new
   Create a new explosion.  */

   sc_explosion *e;        /* A newly created explosion */

   /* Make sure explosion has a radius */
   if(radius <= 0) return(NULL);

   /* Allocate memory for a new explosion */
   e = (sc_explosion *)malloc(sizeof(sc_explosion));
   if(e == NULL) return(NULL);

   /* Initialise variables */
   e->centerx        = centerx;
   e->centery        = centery;
   e->direction      = 0;
   e->angular_width  = 0;
   e->radius         = radius;
   e->force          = force;
   e->playerid       = playerid;
   e->type           = type;
   e->counter        = 0;

   /* By default, no chain on this weapon */
   e->chain = NULL;
   return(e);

}



sc_explosion *sc_expl_new_with_angle(int centerx, int centery, int radius, int force,
                                     double direction, double angular_width,
                                     int playerid, sc_explosion_type type) {
/* sc_expl_new_with_angle
   Create a new explosion, oriented at the specified direction (in radians;
   0 = right, M_PI/2 = up), and with the specified angular width (radians).
   If the angular width is 0, then direction is ignored, and normal circular
   explosions are used.  */

   sc_explosion *e;        /* A newly created explosion */

   /* Allocate and initialize the explosion */
   e = sc_expl_new(centerx, centery, radius, force, playerid, type);
   if(e == NULL) return(NULL);

   /* Setup the angular attributes */
   e->direction      = direction;
   e->angular_width  = angular_width;

   /* Return the directed explosion. */
   return(e);

}



sc_explosion *sc_expl_add(sc_explosion **e, sc_explosion *add) {
/* sc_expl_add
   Adds explosion add to the _end_ of e.  */

   sc_explosion *insert_after;

   if(e == NULL) return(NULL);

   insert_after = *e;
   if(insert_after == NULL) {
      *e = add;
   } else {
      while(insert_after->chain != NULL) insert_after = insert_after->chain;
      insert_after->chain = add;
   }

   return(add);

}



sc_explosion *sc_expl_index(sc_explosion *e, int index) {
/* sc_expl_index */

   if(e == NULL || index < 0) return(NULL);
   while(index > 0 && e != NULL) {
      e = e->chain;
      --index;
   }
   return(e);

}



int sc_expl_count(const sc_explosion *e) {
/* sc_expl_count */

   int count = 0;
   while(e != NULL) {
      ++count;
      e = e->chain;
   }
   return(count);

}



void sc_expl_free(sc_explosion **e) {
/* sc_expl_free
   Releases the explosion at the head of the list,
   and sets *e to its original chain pointer.  */

   sc_explosion *del;      /* Explosion to be deleted */

   /* Sanity checking */
   if(e == NULL || *e == NULL) return;

   /* Get the explosion to be deleted, and reassign *e */
   del = *e;
   *e = del->chain;

   /* Delete this explosion */
   free(del);

}



void sc_expl_free_chain(sc_explosion **e) {
/* sc_expl_free_chain
   Releases all explosions on this chain, at once.  */

   /* Release the whole chain */
   if(e == NULL) return;
   while(*e != NULL) sc_expl_free(e);

}



typedef struct _sc_expl_cache {
   int cacheid;
   int eradius;
} sc_expl_cache;



/***  Lowlevel Screen Updates -- COLUMNS  ***/



static inline void _sc_expl_annihilate_fill_column(sc_config *c, sc_land *l,
                                                   int x, int y1, int y2) {
/* sc_expl_annihilate_fill_column
   Fills a column with land.  This fills any clear tiles in column x
   with GROUND tiles, from y1 up to y2 (y1 < y2) (inclusive of both
   endpoints).  This is an internal function only.  */

   const int *gradient; /* Sky gradient */
   bool dither;         /* Enable dithering? */
   int *lp;             /* Pointer into land structure at (x, y) */
   int y;               /* Current Y coordinate */

   /* Get the sky gradient */
   gradient = c->colors->gradindex[SC_GRAD_GROUND];
   dither = c->graphics.gfxdither;

   /* Boundary checks */
   if(!sc_land_translate_x(l, &x)) return;
   if(y1 < 0) y1 = 0;
   if(y2 >= c->fieldheight) y2 = c->fieldheight - 1;

   /* Boundary checks have already been performed */
   y = y1;
   lp = SC_LAND_XY(l, x, y);
   while(y <= y2) {
      if(SC_LAND_IS_SKY(*lp)) {
         *lp = SC_LAND_GROUND | sc_color_gradient_index(dither, gradient, y);
      } /* Was the tile originally sky? */
      ++lp;
      ++y;
   }

}



static inline void _sc_expl_annihilate_clear_column(sc_config *c, sc_land *l,
                                                    int x, int y1, int y2) {
/* sc_expl_annihilate_clear_column
   Clears a column of everything in a column; everything in the column
   is set to SKY tiles.  This clears column x, from y1 up to y2 (y1 < y2)
   (inclusive of both endpoints).  This is an internal function only.  */

   const int *gradient; /* Sky gradient */
   int gradientflag;    /* Sky gradient flag */
   bool dither;         /* Enable dithering? */
   int *lp;             /* Pointer into land structure at (x, y) */
   int y;               /* Current Y coordinate */

   /* Get the sky gradient */
   gradient = sc_land_sky_index(c);
   gradientflag = sc_land_sky_flag(c);
   dither = c->graphics.gfxdither;

   /* Boundary checks */
   if(!sc_land_translate_x(l, &x)) return;
   if(y1 < 0) y1 = 0;
   if(y2 >= c->fieldheight) y2 = c->fieldheight - 1;

   /* Boundary checks have already been performed */
   y = y1;
   lp = SC_LAND_XY(l, x, y);
   while(y <= y2) {
      *lp = gradientflag | sc_color_gradient_index(dither, gradient, y);
      ++lp;
      ++y;
   }

}



static inline void _sc_expl_annihilate_column(sc_config *c, sc_land *l,
                                              int x, int y1, int y2, bool fill) {
/* sc_expl_annihilate_clear_column
   Clears a column to GROUND (if fill == true), or SKY (if fill == false).
   This clears column x, from y1 up to y2 (y1 < y2).  This is an internal
   function only.  */

   if(fill) {
      _sc_expl_annihilate_fill_column(c, l, x, y1, y2);
   } else {
      _sc_expl_annihilate_clear_column(c, l, x, y1, y2);
   }
}



/***  Lowlevel Screen Updates -- WEDGE code  ***/



#define  SC_WEDGE_SECTION_TOP     (1<<0)
#define  SC_WEDGE_SECTION_MIDDLE  (1<<1)
#define  SC_WEDGE_SECTION_BOTTOM  (1<<2)
#define  SC_WEDGE_SECTION_UPPER   (SC_WEDGE_SECTION_TOP    | SC_WEDGE_SECTION_MIDDLE)
#define  SC_WEDGE_SECTION_LOWER   (SC_WEDGE_SECTION_MIDDLE | SC_WEDGE_SECTION_BOTTOM)
#define  SC_WEDGE_SECTION_ENDS    (SC_WEDGE_SECTION_TOP    | SC_WEDGE_SECTION_BOTTOM)
#define  SC_WEDGE_SECTION_ALL     (SC_WEDGE_SECTION_UPPER  | SC_WEDGE_SECTION_LOWER)
#define  SC_WEDGE_SECTION_NONE    (0)


typedef struct _sc_wedge_boundaries {
   int    left_x,      right_x;
   int    left_y,      right_y;
   double left_slope,  right_slope;
   bool   inside, thirds;
   int    section;
} sc_wedge_boundaries;



static void _sc_expl_annihilate_half_column(sc_config *c, sc_land *l,
                                            int cx, int cy, int dx, int dy,
                                            const sc_wedge_boundaries *wedge,
                                            int critical_y, bool fill) {

   #if SC_EXPL_DEBUG_WEDGES
      printf("expl_annihilate_half_column: dx=%04d, 2 sections, %04d -> %04d -> %04d\n",
             dx, cy - dy, cy + critical_y, cy + dy);
   #endif /* SC_EXPL_DEBUG_WEDGES */

   /* The middle section is illegal in half_column. */
   assert(!(wedge->section & SC_WEDGE_SECTION_MIDDLE));

   switch(wedge->section) {
      case SC_WEDGE_SECTION_ENDS:
         _sc_expl_annihilate_column(c, l, cx + dx, cy - dy, cy + dy, fill);
         break;
      case SC_WEDGE_SECTION_TOP:
         _sc_expl_annihilate_column(c, l, cx + dx, cy + critical_y, cy + dy, fill);
         break;
      case SC_WEDGE_SECTION_BOTTOM:
         _sc_expl_annihilate_column(c, l, cx + dx, cy - dy, cy + critical_y, fill);
         break;
      case SC_WEDGE_SECTION_NONE:
         #if SC_EXPL_DEBUG_WEDGES
            printf("expl_annihilate_half_column: draw none (too thin?)\n");
         #else
            assert(false);
         #endif /* SC_EXPL_DEBUG_WEDGES */
         break;
   }

}



static void _sc_expl_annihilate_third_column(sc_config *c, sc_land *l,
                                             int cx, int cy, int dx, int dy,
                                             const sc_wedge_boundaries *wedge,
                                             bool fill) {

   /* Fills in 1, 2, or 3 of the sliced sections.
      3 section case only occurs at border cases. */
   int ldy = (int)(wedge->left_slope  * dx);
   int rdy = (int)(wedge->right_slope * dx);
   int min_boundary_y = min(ldy, rdy);
   int max_boundary_y = max(ldy, rdy);
   min_boundary_y = max(-dy, min_boundary_y);
   max_boundary_y = min( dy, max_boundary_y);

   #if SC_EXPL_DEBUG_WEDGES
      printf("expl_annihilate_third_column: dx=%04d, 3 sections, %04d -> %04d -> %04d -> %04d\n",
             dx, cy - dy, cy + min_boundary_y, cy + max_boundary_y, cy + dy);
   #endif /* SC_EXPL_DEBUG_WEDGES */

   switch(wedge->section) {
      case SC_WEDGE_SECTION_ALL:
         _sc_expl_annihilate_column(c, l, cx + dx, cy - dy, cy + dy, fill);
         break;
      case SC_WEDGE_SECTION_UPPER:
         _sc_expl_annihilate_column(c, l, cx + dx, cy + min_boundary_y, cy + dy, fill);
         break;
      case SC_WEDGE_SECTION_ENDS:
         _sc_expl_annihilate_column(c, l, cx + dx, cy + max_boundary_y, cy + dy, fill);
         _sc_expl_annihilate_column(c, l, cx + dx, cy - dy, cy + min_boundary_y, fill);
         break;
      case SC_WEDGE_SECTION_TOP:
         _sc_expl_annihilate_column(c, l, cx + dx, cy + max_boundary_y, cy + dy, fill);
         break;
      case SC_WEDGE_SECTION_LOWER:
         _sc_expl_annihilate_column(c, l, cx + dx, cy - dy, cy + max_boundary_y, fill);
         break;
      case SC_WEDGE_SECTION_MIDDLE:
         _sc_expl_annihilate_column(c, l, cx + dx, cy + min_boundary_y, cy + max_boundary_y, fill);
         break;
      case SC_WEDGE_SECTION_BOTTOM:
         _sc_expl_annihilate_column(c, l, cx + dx, cy - dy, cy + min_boundary_y, fill);
         break;
      case SC_WEDGE_SECTION_NONE:
         #if SC_EXPL_DEBUG_WEDGES
            printf("expl_annihilate_third_column: draw none (too thin?)\n");
         #else
            assert(false);
         #endif /* SC_EXPL_DEBUG_WEDGES */
         break;
   }

}



static void _sc_expl_annihilate_column_cautious(sc_config *c, sc_land *l,
                                                int cx, int cy, int dx, int dy,
                                                const sc_wedge_boundaries *wedge,
                                                bool fill) {

   if(wedge->thirds) {
      _sc_expl_annihilate_third_column(c, l, cx, cy, dx, dy, wedge, fill);
   } else if(dx >= 0) {
      _sc_expl_annihilate_half_column(c, l, cx, cy, dx, dy, wedge, (int)rint(wedge->right_slope * dx), fill);
   } else {
      _sc_expl_annihilate_half_column(c, l, cx, cy, dx, dy, wedge, (int)rint(wedge->left_slope * dx), fill);
   }

}



static inline void _sc_expl_annihilate_column_pair(sc_config *c, sc_land *l,
                                                   int cx, int cy, int dx, int dy,
                                                   const sc_wedge_boundaries *wedge,
                                                   bool fill, int min_x, int max_x) {

   if(-dx >= min_x && -dx <= max_x) {
      _sc_expl_annihilate_column_cautious(c, l, cx, cy, -dx, dy, wedge, fill);
   } else if(!wedge->inside) {
      _sc_expl_annihilate_column(c, l, cx - dx, cy - dy, cy + dy, fill);
   }
   if(dx >= min_x && dx <= max_x) {
      _sc_expl_annihilate_column_cautious(c, l, cx, cy, dx, dy, wedge, fill);
   } else if(!wedge->inside) {
      _sc_expl_annihilate_column(c, l, cx + dx, cy - dy, cy + dy, fill);
   }

}



static double _sc_expl_clockwise_closest(double angle_1, double angle_2) {
/* sc_expl_clockwise_closest
   Produces angle_1 >= (return value) > angle_1 - 2*pi
                     ^ equality is important.
   mod(return_value,2*pi)=mod(angle_2,2*pi)  */

   while(angle_2 > angle_1) {
      /* too big? */
      angle_2 -= 2 * M_PI;
   }
   while(angle_2 <= (angle_1 - 2 * M_PI)) {
      /* too big? */
      angle_2 += 2 * M_PI;
   }
   return(angle_2);

}



static bool _sc_expl_pixel_test(int dy, const sc_wedge_boundaries *wedge) {
/* sc_expl_pixel_test */

   int dx            = 0;  /* currently this is never called with any other dx */
   int left_cross_r  = (wedge->left_y  * dx - wedge->left_x  * dy);
   int right_cross_r = (wedge->right_y * dx - wedge->right_x * dy);
   bool is_inside    = (left_cross_r > 0) && (right_cross_r < 0);
   bool border       =
      ((left_cross_r == 0) && (right_cross_r <  0)) ||
      ((left_cross_r >  0) && (right_cross_r == 0));

   if(border) {
      return(true);
   }
   if(wedge->inside) {
      return(is_inside);
   } else {
      return(!is_inside);
   }

}



/*
   Jason's explanation of the logic governing the following loop:
   (The idea is to avoid potential confusion as to why this method is used)
   The most obvious method is to use a for loop over x (from -r to r)
   (actually from symetry, just dx=0 to r, and use -dx and dx...)
   And then clear/fill from -sqrt(r^2 - x^2) to sqrt(r^2 - x^2)
   Sqrt's are slow and doing something like "while (x*x+y*y>r*r) y--;"
   would work, that can become slow, especially for extreme values of x.
   Near the middle of the circle, the for loop would run 0-1 times.
   Near the outside of the circle, it could run many times.
   (Of course one could bound it, and claim it isn't that bad...)

   The alternative approach:
   The region of y reducing by zero or one "near the middle" is similar to
   x reducing by zero or one "near the edges"... In fact, they meet at 45
   degrees...  So, the knowledge of x,y values near the middle can be
   transposed to get the x,y (y,x) values near the outside...
   so, start out at dx = 0, dy = r, and move out increasing dx, decreasing dy
   Bascially, you want to maintain dx*dx + dy*dy = r*r
   Now when you increase dx, new_dx*new_dx - old_dx*old_dx
   = new_dx*new_dx - (new_dx - 1)*(new_dx - 1) = 2*new_dx - 1
   And when you decrease dy, new_dy*new_dy - old_dy*old_dy
   = new_dy*new_dy - (new_dy + 1)*(new_dy + 1) =-2*new_dy - 1;

   So, if rad2 = r*r, then your dx,dy pair approximate rad2...
   basically dx*dx + dy*dy = rad2_approx.
   Initially, dx = 0, dy = r, so rad2_approx = rad2.
   Then you modify dx or dy, and get a new rad2_approx
   That's how I would do it... when first thinking about it...

   There's a small problem with that... dy will decrease as soon
   as dx increases... resulting in a lone dot at all 4 edges.

   In the code below, rad2major2 = r*r - dx*dx;
   So then, I would think the comparison would be with dy*dy...
   Of course, there's a small problem with comparing against
   dy*dy; you'll end up with lone dots at 0,+/-r and +/-r,0

   It looks like the code compares with dy*dy-dy or dy*(dy-1)
   using the math from before, dy*(dy-1) decreases by 2*new_dy
   This is a slight variant, on (dy-.5)*(dy-.5)=dy*dy-dy+.25
   and will make everything look smoother.  Theoretically,
   (dy-1/2)*(dy-1/2) as a comparison could be supported by
   the argument that you are rounding to the nearest pixel.

   Maybe the reasoning for dy*(dy-1) will be shared by the
   original authors :)  There could be finer details, like
   rounding arguments nears 45 degrees...
 */



static void _sc_expl_annihilate_wedge(sc_config *c, const sc_explosion *e,
                                      int radius, bool fill) {
/* sc_expl_annihilate_wedge
   Clears (if fill == false) or fills (if fill == true) dirt in the
   explosion specified.  This updates the land buffer but it will NOT
   update the display screen.

   This function is used for code that is performing wedged explosions.
   It may be slightly less efficient than annihilate_circular; once I
   convince myself that this code actually works, I may go ahead and
   delete annihilate_circular.  */

   int cx=e->centerx;/* Center X of explosion */
   int cy=e->centery;/* Center Y of explosion */
   int dx;           /* Delta X (distance away from cx) - iterator variable */
   int dy;           /* Delta Y (distance away from cy) for _edge_ of circle */
   int rad2;         /* Radius squared */
   int rad2major2;   /* Radius^2 + the major_distance^2 */
   int min2thresh;   /* Minimum threshold to avoid redrawing columns where dx>dy */
   sc_wedge_boundaries wedge; /* Wedge boundary support */
   int min_x, max_x;          /* Affected range of dx */
   double left_angle;         /* Leftmost angle of wedge */
   double right_angle;        /* Rightmost angle of wedge */

   /* Figure out the minimum and maximum angles for the wedge */
   right_angle = e->direction - e->angular_width / 2;
   left_angle  = e->direction + e->angular_width / 2;

   /* By default, assume we are drawing "inside" the wedge, not
      outside of it.  We might change our mind on this once we
      figure out how wide the arc is...  */
   wedge.inside = true;

   /* Coding for detecting a wedge requires < 180 degree separation
      If > 180 degree seperation, reverse say draw outside the region
      If < 180 degree seperation, then we're okay.  */
   if((_sc_expl_clockwise_closest(left_angle, right_angle) - left_angle < -M_PI)) {
      left_angle     = left_angle + right_angle;
      right_angle    = left_angle - right_angle;
      left_angle     = left_angle - right_angle;
      wedge.inside   = false;
   }

   /* Make sure nothing is odd here with 360 degrees wrapping to 0 degrees...
    * Also skip the trig */
   if(e->angular_width > M_PI * 2 - 0.00001) {
      /* We're not using the wedge code in this case */
      wedge.inside   = false;

      /* 360, Draw everything! */
      wedge.section  = SC_WEDGE_SECTION_ALL;
      wedge.left_x   = 0;
      wedge.right_x  = 0;
      min_x          = 0;
      max_x          = 0;
      wedge.left_y   = radius;
      wedge.right_y  = radius;
      /* left_slope and right_slope are ignored */
   } else {
      wedge.left_x   = rint(radius * cos(left_angle));
      wedge.left_y   = rint(radius * sin(left_angle));
      wedge.left_slope =             tan(left_angle);
      wedge.right_x  = rint(radius * cos(right_angle));
      wedge.right_y  = rint(radius * sin(right_angle));
      wedge.right_slope =            tan(right_angle);
      min_x          = min(wedge.left_x, wedge.right_x);
      max_x          = max(wedge.left_x, wedge.right_x);

      if(min_x * max_x >= 0) {
         /* Both wedge boundaries are on the same side... */
         wedge.thirds = true;
         wedge.section = (wedge.inside ? SC_WEDGE_SECTION_MIDDLE
                                       : SC_WEDGE_SECTION_TOP | SC_WEDGE_SECTION_BOTTOM);
         min_x       = min(min_x, 0);
         max_x       = max(max_x, 0);
      } else {
         /* wedge boundaries on opposite sides */
         wedge.thirds = false;
         wedge.section = (_sc_expl_pixel_test(radius, &wedge) ? SC_WEDGE_SECTION_TOP
                                                              : SC_WEDGE_SECTION_BOTTOM);
      }

      #if SC_EXPL_DEBUG_WEDGES
         printf("**** New Explosion (expl_annihilate_wedge) ****\n");
         printf("center x=%04, y=%04, r=%04\n", cx, cy, radius);
         printf("left: %03dD right: %03dD, %s\n",
                (int)(left_angle * 180 / M_PI),
                (int)(right_angle * 180 / M_PI),
                wedge.inside ? "inside" : "outside");
         printf("dx_min = %04d, dx_max = %04d\n\n", min_x, max_x);
      #endif /* SC_EXPL_DEBUG_WEDGES */
   }

   /* DX = major axis, DY = minor axis */
   dx = 0;           /* DX starts at zero (iterator) */
   dy = radius;      /* DY is one radius away (edge of circle at cx+dx) */
   rad2 = radius * radius; /* Calculate Radius Squared */
   rad2major2 = rad2;      /* Radius^2 + major^2, running total */
   min2thresh = rad2 - dy; /* Minimum threshold before need to redraw edges */

   /* Should know that, we are incrementing DX every time.  However,
      if we call the transpose method every time as well, then we will
      be filling parts of the circle multiple times.  Hence the
      min2thresh variable. */
   do {
      /* Column pair is columns at cx-dx and cx+dx */
      _sc_expl_annihilate_column_pair(c, c->land,
                                      cx, cy, dx, dy,
                                      &wedge, fill,
                                      min_x, max_x);
      ++dx;
      rad2major2 -= dx + dx - 1;
      if(rad2major2 <= min2thresh) {
         /* swap dx and dy */
         _sc_expl_annihilate_column_pair(c, c->land,
                                         cx, cy, dy, dx,
                                         &wedge, fill,
                                         min_x, max_x);
         --dy;
         min2thresh -= dy + dy;
      }
   } while(dx <= dy);

}



/***  Lowlevel Screen Updates -- NON-WEDGE code  ***/



static void _sc_expl_annihilate_circular(sc_config *c, const sc_explosion *e,
                                         int radius, bool fill) {
/* sc_expl_annihilate_circular
   Clears (if fill == false) or fills (if fill == true) dirt in the
   explosion specified.  This updates the land buffer but it will NOT
   update the display screen.

   This function assumes there is no "wedge" being computed.  It
   bypasses pretty much all of the wedge-specific computations as
   a result.  */

   int cx=e->centerx;/* Center X of explosion */
   int cy=e->centery;/* Center Y of explosion */
   int dx;           /* Delta X (distance away from cx) - iterator variable */
   int dy;           /* Delta Y (distance away from cy) for _edge_ of circle */
   int rad2;         /* Radius squared */
   int rad2major2;   /* Radius^2 + the major_distance^2 */
   int min2thresh;   /* Minimum threshold to avoid redrawing columns where dx>dy */

   /* DX = major axis, DY = minor axis */
   dx = 0;           /* DX starts at zero (iterator) */
   dy = radius;      /* DY is one radius away (edge of circle at cx+dx) */
   rad2 = radius * radius; /* Calculate Radius Squared */
   rad2major2 = rad2;      /* Radius^2 + major^2, running total */
   min2thresh = rad2 - dy; /* Minimum threshold before need to redraw edges */

   /* Should know that, we are incrementing DX every time.  However,
      if we call the transpose method every time as well, then we will
      be filling parts of the circle multiple times.  Hence the
      min2thresh variable. */
   do {
      _sc_expl_annihilate_column(c, c->land, cx - dx, cy - dy, cy + dy, fill);
      _sc_expl_annihilate_column(c, c->land, cx + dx, cy - dy, cy + dy, fill);
      ++dx;
      rad2major2 -= dx + dx - 1;
      if(rad2major2 <= min2thresh) {
         /* Swap dx and dy */
         _sc_expl_annihilate_column(c, c->land, cx - dy, cy - dx, cy + dx, fill);
         _sc_expl_annihilate_column(c, c->land, cx + dy, cy - dx, cy + dx, fill);
         --dy;
         min2thresh -= dy + dy;
      }
   } while(dx <= dy);

}



static void _sc_expl_annihilate(sc_config *c, const sc_explosion *e,
                                int radius, bool fill) {
/* sc_expl_annihilate
   Clears (if fill == false) or fills (if fill == true) dirt in the
   explosion specified.  This updates the land buffer but it will NOT
   update the display screen.  */

   /* Dispatch to either wedge code or non-wedge code. */
   if(!SC_EXPL_DEBUG_WEDGES && e->angular_width == 0) {
      _sc_expl_annihilate_circular(c, e, radius, fill);
   } else {
      _sc_expl_annihilate_wedge(c, e, radius, fill);
   }

   /* Update the land buffer */
   sc_window_paint(c->window,
                   e->centerx - radius, e->centery - radius,
                   e->centerx + radius, e->centery + radius,
                   SC_REGENERATE_LAND);

}



static void _sc_expl_annihilate_clear(sc_config *c, const sc_explosion *e, int radius) {
/* sc_expl_annihilate_clear
   Clears the explosion specified.  This updates the land
   buffer but it will NOT update the display screen.  */

   _sc_expl_annihilate(c, e, radius, false);

}



static void _sc_expl_annihilate_fill(sc_config *c, const sc_explosion *e, int radius) {
/* sc_expl_annihilate_fill
   Fills the area specified with land.  This regenerates the
   land buffer but it will NOT update the display screen.  */

   _sc_expl_annihilate(c, e, radius, true);

}



/***  Interface for Annihilation (explosion draw) and Clear (explosion undraw)  ***/



bool sc_expl_annihilate(sc_config *c, sc_explosion *e) {
/* sc_expl_annihilate
   Annihilate a section of the screen by drawing a huge explosion to it.
   This function is called to initiate animation of the explosion and
   update the landmass, indicating the area destroyed by the explosion.
   If it returns "true", then the caller should call annihilate_continue
   at some point to continue processing the explosion in an "animated"
   manner.  If it returns "false", then the explosion has been completely
   dealt with, and you are ready to call annihilate_clear.  */

   sc_expl_cache *ca;   /* Explosion cache (standard explosions) */
   sc_spill *sp;        /* Spill information (liquid explosions) */

   /* Sanity check */
   if(c == NULL || e == NULL) return(false);

   /* Action depends on weapon type */
   switch(e->type) {
      case SC_EXPLOSION_SPIDER:
         /* if there is an arc to draw, draw it */
         if(e->data != NULL)
            sc_window_draw_arc(c->window, e->data, e->playerid);
         else
            fprintf(stderr, "BUG: spider with null leg arc.\n");
         /* fall through to explosion drawing */

      case SC_EXPLOSION_NORMAL:
      case SC_EXPLOSION_PLASMA:
         /* Get a new explosion cache ID, and draw it */
         if(!SC_CONFIG_GFX_FAST(c)) {
            ca = (sc_expl_cache *)malloc(sizeof(sc_expl_cache));
            if(ca == NULL) return(false);
            if(SC_CONFIG_ANIM(c)) ca->eradius = 0;
            else ca->eradius = e->radius;
            ca->cacheid = sc_expl_cache_new(c->window, e->radius, e->type);
            e->cache = ca;
            return(true);
         } /* Only executed if not in fast mode */
         return(false);

      case SC_EXPLOSION_NAPALM:
         /* Retrieve the napalm spill */
         sp = (sc_spill *)e->data;
         if(!SC_CONFIG_GFX_FAST(c) && sp != NULL) {
            if(SC_CONFIG_ANIM(c)) {
               sc_window_draw_napalm_frame(c->window,
                                           sp->spillx, sp->spilly,
                                           min(sp->size, SC_EXPL_LIQUID_STEP));
               sp->index = SC_EXPL_LIQUID_STEP;
            } else {
               sc_window_draw_napalm_frame(c->window, sp->spillx, sp->spilly, sp->size);
               sp->index = sp->size + 1;
            }
            return(true);
         } /* Only attempt the draw if spill pointer valid */
         return(false);

      case SC_EXPLOSION_LIQ_DIRT:
         /* Retrieve the liquid dirt spill */
         sp = (sc_spill *)e->data;
         if(!SC_CONFIG_GFX_FAST(c) && sp != NULL) {
            if(SC_CONFIG_ANIM(c)) {
               sc_land_create_dirt(c, c->land,
                                   sp->spillx, sp->spilly,
                                   min(sp->size, SC_EXPL_LIQUID_STEP));
               sp->index = SC_EXPL_LIQUID_STEP;
            } else {
               sc_land_create_dirt(c, c->land, sp->spillx, sp->spilly, sp->size);
               sp->index = sp->size + 1;
            }
            return(true);
         } /* Only attempt the draw if spill pointer valid */
         return(false);

      case SC_EXPLOSION_DIRT:
         /* Animate a circular explosion that is filling up */
         _sc_expl_annihilate_fill(c, e, e->radius);
         if(!SC_CONFIG_GFX_FAST(c)) {
            /* Only do slow animation if not in fast mode */
            e->idraw = 0;
            return(true);
         } else {
            /* Just update everything at once */
            sc_window_paint(c->window,
                            e->centerx - e->radius,
                            e->centery - e->radius,
                            e->centerx + e->radius,
                            e->centery + e->radius,
                            SC_PAINT_EVERYTHING);
         } /* Only executed if not in fast mode */
         break;

      case SC_EXPLOSION_RIOT:
         /* Animate a circular explosion that is clearing out */
         _sc_expl_annihilate_clear(c, e, e->radius);
         if(!SC_CONFIG_GFX_FAST(c)) {
            /* Only do slow animation if not in fast mode */
            e->idraw = 0;
            return(true);
         } else {
            /* Just update everything at once */
            sc_window_paint(c->window,
                            e->centerx - e->radius,
                            e->centery - e->radius,
                            e->centerx + e->radius,
                            e->centery + e->radius,
                            SC_PAINT_EVERYTHING);
         } /* Only executed if not in fast mode */
         break;

   } /* End switch on explosion type */

   return(false);

}



bool sc_expl_annihilate_continue(sc_config *c, sc_explosion *e) {
/* sc_expl_annihilate_continue
   Continue to annihilate a section of the screen by drawing a huge explosion
   to it.  This function returns "true" if it needs to be called again to
   continue.  If it returns "false", then the explosion has been completely
   processed and you may call annihilate_clear.  */

   sc_expl_cache *ca;   /* Explosion cache (standard explosions) */
   sc_spill *sp;        /* Spill information (liquid explosions) */

   /* Sanity check */
   if(c == NULL || e == NULL) return(false);

   /* Action depends on weapon type */
   switch(e->type) {
      case SC_EXPLOSION_SPIDER:
         /* nothing special to animate spider explosions */

      case SC_EXPLOSION_NORMAL:
      case SC_EXPLOSION_PLASMA:
         /* Get a new explosion cache ID, and draw it */
         ca = e->cache;
         if(ca != NULL) {
            ca->eradius += SC_EXPL_EXPLOSION_STEP;
            if(ca->eradius >= e->radius) ca->eradius = e->radius;
            sc_expl_cache_draw(c->window, ca->cacheid,
                               e->centerx, e->centery,
                               ca->eradius);
            if(c->physics->walls == SC_WALL_WRAP) {
               if(e->centerx + e->radius >= c->fieldwidth) {
                  sc_expl_cache_draw(c->window, ca->cacheid,
                                     e->centerx - c->fieldwidth,
                                     e->centery, ca->eradius);
               } else if(e->centerx - e->radius < 0) {
                  sc_expl_cache_draw(c->window, ca->cacheid,
                                     e->centerx + c->fieldwidth,
                                     e->centery, ca->eradius);
               } /* Did the explosion wrap off-screen? */
            } /* Were boundaries wrap-around? */

            /* Done animating? */
            if(ca->eradius >= e->radius) {
               free(ca);
               return(false);
            }

            /* We still need to animate */
            return(true);
         } /* Only executed if not in fast mode */
         return(false);

      case SC_EXPLOSION_NAPALM:
         /* Construct the napalm spill */
         if(SC_CONFIG_NO_ANIM(c)) return(false);
         sp = (sc_spill *)e->data;
         if(sp != NULL) {
            if(sp->count > 0) {
               sc_window_draw_napalm_final(c->window, sp->spillx, sp->spilly, sp->size);
               ++sp->count;
               if(sp->count > SC_EXPL_NAPALM_FLAMES) return(false);
            } else if(sp->index < sp->size) {
               sc_window_draw_napalm_frame(c->window,
                                           sp->spillx + sp->index,
                                           sp->spilly + sp->index,
                                           min(sp->size - sp->index, SC_EXPL_LIQUID_STEP));
               sp->index += SC_EXPL_LIQUID_STEP;
            } else {
               sp->count = 1;
            } /* Do we still have business to take care of? */
         } /* Only attempt the draw if spill pointer valid */
         return(true);

      case SC_EXPLOSION_LIQ_DIRT:
         /* Construct the liquid dirt spill */
         if(SC_CONFIG_NO_ANIM(c)) return(false);
         sp = (sc_spill *)e->data;
         if(sp != NULL) {
            if(sp->index < sp->size) {
               sc_land_create_dirt(c, c->land,
                                   sp->spillx + sp->index,
                                   sp->spilly + sp->index,
                                   min(sp->size - sp->index, SC_EXPL_LIQUID_STEP));
               sp->index += SC_EXPL_LIQUID_STEP;
            } else {
               return(false);
            } /* Do we still have business to take care of? */
         } /* Only attempt the draw if spill pointer valid */
         return(true);

      case SC_EXPLOSION_DIRT:
      case SC_EXPLOSION_RIOT:
         /* Continue to clear a circular explosion */
         e->idraw += SC_EXPL_EXPLOSION_STEP;
         if(e->idraw >= e->radius) {
            sc_window_paint(c->window,
                            e->centerx - e->radius,
                            e->centery - e->radius,
                            e->centerx + e->radius,
                            e->centery + e->radius,
                            SC_PAINT_EVERYTHING);
            return(false);
         } else {
            sc_window_paint_circular(c->window,
                                     e->centerx, e->centery,
                                     e->idraw,
                                     SC_PAINT_EVERYTHING);
            return(true);
         }
         break;

   } /* End switch on explosion type */

   return(false);

}



bool sc_expl_annihilate_clear(sc_config *c, sc_explosion *e) {
/* sc_expl_annihilate_clear
   Clears someone else's explosions.  If this function returns "true", then
   you need to call annihilate_clear_continue to continue processing the
   clearing of the explosion; if it returns "false", then the explosion is
   completely done.  */

   sc_trajectory *tr;
   sc_spill *sp;        /* Spill information (liquid explosions) */

   /* Sanity check */
   if(c == NULL || e == NULL) return(false);

   /* Action depends on weapon type */
   switch(e->type) {
      case SC_EXPLOSION_SPIDER:
        /* if there is an arc to clear, clear it */
        if(e->data != NULL) {
           sc_window_clear_arc(c->window, e->data);
           tr = (sc_trajectory *)e->data;
           sc_traj_landfall(c, tr);
           sc_traj_free(&tr);
        }
        /* fall through to explosion clearing */

      case SC_EXPLOSION_NORMAL:
      case SC_EXPLOSION_PLASMA:
         /* Clear the circular explosion */
         _sc_expl_annihilate_clear(c, e, e->radius);
         if(!SC_CONFIG_GFX_FAST(c)) {
            /* Only do slow animation if not in fast mode */
            e->idraw = 0;
            return(true);
         } else {
            /* Just update everything at once */
            sc_window_paint(c->window,
                            e->centerx - e->radius,
                            e->centery - e->radius,
                            e->centerx + e->radius,
                            e->centery + e->radius,
                            SC_PAINT_EVERYTHING);
         } /* Only executed if not in fast mode */
         break;

      case SC_EXPLOSION_NAPALM:
         /* Clean up the napalm spill */
         sp = (sc_spill *)e->data;
         if(!SC_CONFIG_GFX_FAST(c)) {
            sc_window_clear_napalm(c->window, sp->spillx, sp->spilly, sp->size);
         }
         sc_spill_free((sc_spill **)&e->data);
         break;

      case SC_EXPLOSION_LIQ_DIRT:
      case SC_EXPLOSION_RIOT:
      case SC_EXPLOSION_DIRT:
         /* Nothing to do */
         break;

   } /* End switch on explosion type */

   return(false);

}



bool sc_expl_annihilate_clear_continue(sc_config *c, sc_explosion *e) {
/* sc_expl_annihilate_clear_continue
   Continue to clear someone else's explosions.  If this function returns
   "true", then you need to call annihilate_clear_continue to continue
   processing the clearing of the explosion; if it returns "false", then
   the explosion is completely done.  */

   /* Sanity check */
   if(c == NULL || e == NULL) return(false);

   /* Action depends on weapon type */
   switch(e->type) {
      case SC_EXPLOSION_SPIDER:
      case SC_EXPLOSION_NORMAL:
      case SC_EXPLOSION_PLASMA:
         /* Continue to clear the circular explosion */
         e->idraw += SC_EXPL_EXPLOSION_STEP;
         if(e->idraw >= e->radius) {
            sc_window_paint(c->window,
                            e->centerx - e->radius,
                            e->centery - e->radius,
                            e->centerx + e->radius,
                            e->centery + e->radius,
                            SC_PAINT_EVERYTHING);
            return(false);
         } else {
            sc_window_paint_circular(c->window,
                                     e->centerx, e->centery,
                                     e->idraw,
                                     SC_PAINT_EVERYTHING);
            return(true);
         }
         break;

      case SC_EXPLOSION_NAPALM:
      case SC_EXPLOSION_LIQ_DIRT:
      case SC_EXPLOSION_DIRT:
      case SC_EXPLOSION_RIOT:
         /* Nothing to do */
         break;

   } /* End switch on explosion type */

   return(false);

}



/***  Assessment of Damage Caused by an Explosion  ***/



int sc_expl_damage_at_point(const sc_land *l, const sc_explosion *e, int x, int y) {
/* sc_expl_damage_at_point
   Returns the amount of damage (in units comparable to life), done by the
   specified explosion to an object centered at (x, y).  If no damage is
   done, zero is returned.  */

   sc_spill *sp;     /* Spill data (for napalm damages) */
   int distance;     /* Distance between explosion center and (x,y) (squared) */
   int damage;       /* Actual amount of damage done */
   int deltax;       /* Change in X */
   int deltay;       /* Change in Y */
   int i;            /* Iterator */

   /* Determine the damaging effect of the weapon */
   damage = 0;
   switch(e->type) {
      case SC_EXPLOSION_SPIDER:
      case SC_EXPLOSION_NORMAL:
      case SC_EXPLOSION_PLASMA:
         /* Calculate distance */
         sc_land_calculate_deltas(l, &deltax, &deltay, x, y, e->centerx, e->centery);
         distance = SQR(deltax) + SQR(deltay);

         /* Calculate the overall damage */
         damage = rint(e->force * (1 - distance / (double)SQR(e->radius)));
         break;

      case SC_EXPLOSION_NAPALM:
         /* Integrate over all points... */
         sp = (sc_spill *)e->data;
         for(i = 0; i < sp->size; ++i) {
            sc_land_calculate_deltas(l, &deltax, &deltay, sp->spillx[i], sp->spilly[i], x, y);
            distance = SQR(deltax) + SQR(deltay);
            distance = rint(e->force * (1 - distance / (double)SQR(e->radius)));
            if(distance < 0) distance = 0;
            damage += distance;
         } /* Iterate through all points in damage zone */
         break;

      case SC_EXPLOSION_LIQ_DIRT:
      case SC_EXPLOSION_DIRT:
      case SC_EXPLOSION_RIOT:
         /* Dirt balls/riots do no damage to tanks */
         break;

   } /* End switch to select radius */

   if(damage < 0) damage = 0;
   return(damage);

}
