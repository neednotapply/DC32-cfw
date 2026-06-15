/* $Header: /fridge/cvs/xscorch/sgame/slscape.c,v 1.6 2009-04-26 17:39:41 jacob Exp $ */
/*

  slscape.c
  Copyright (C) 2000 Matti Hänninen
  e-mail: matti@mvillage.u-net.com
 
 
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.
 
   You should have received a copy of the GNU General Public License along
  along with this program; if not, write to 
 
     The Free Software Foundation, Inc.
   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 
*/
#include <math.h>
#include <stdlib.h>

#include <slscape.h>
#include <sconfig.h>
#include <sland.h>
#include <slscapetools.h>

#include <sutil/srand.h>


/* Internal definition(s) */
#define SC_LSCAPE_RANDOM_PARAMETERS  16


/* Internal variables */
static double sc_lscape_screen_w;     /* Screen width and height in pixels */
static double sc_lscape_screen_h;
static double sc_lscape_bump_height;  /* Bump height (not in pixels) */
static double sc_lscape_bump_freq;    /* Number of bumps in the unit interval */
static long int ri[SC_LSCAPE_RANDOM_PARAMETERS];  /* Random (integer) parameters */
static double rd[SC_LSCAPE_RANDOM_PARAMETERS];    /* Random (real) parameters */


/* Function declarations */
static double _sc_lscape_eval_none(double x);
static double _sc_lscape_eval_canyon(double x);
static double _sc_lscape_eval_hillside(double x);
static double _sc_lscape_eval_mountain(double x);
static double _sc_lscape_eval_plains(double x);
static double _sc_lscape_eval_traditional(double x);
static double _sc_lscape_eval_valley(double x);



static double _sc_lscape_eval_none(__libj_unused double x) {
/* _sc_lscape_eval_none
   Evaluates a null profile. */
   
   return(0);

}



static double _sc_lscape_eval_canyon(double x) {
/* _sc_lscape_eval_canyon
   Evaluates the ``canyon'' profile function at point x. */

   double y;
   double delta;

   /* Coerse the domain into the unit interval */
   x = sc_tools_clamp(0.0, 1.0, x);

   delta = 1.0 - (0.2 + 2.0 * sc_lscape_bump_height);
   y = sc_tools_smoothstep(-0.25, 0.25, sc_tools_pnoise_periodic(ri[0], 7, 7.0 * x + rd[0])) * delta;
   y += sc_tools_pnoise_periodic(ri[1], 7, 7.0 * x + rd[1]) * 0.1;
   y += sc_tools_rnoise_periodic(ri[2], (int)sc_lscape_bump_freq, sc_lscape_bump_freq * x + rd[2]) * sc_lscape_bump_height;
   y += sc_lscape_bump_height + 0.1;

   y = sc_tools_clamp(0.0, 1.0, y);

   return(y);
}



static double _sc_lscape_eval_hillside(double x) {
/* _sc_lscape_eval_hillside
   Evaluates the ``hillside'' profile function at point x. */

   double alpha;
   double beta;
   double y;

   /* Coerse the domain into the unit interval */
   x = sc_tools_clamp(0.0, 1.0, x);
   /* Figure out coefficients */
   alpha = rd[0] > 0.5 ? 0.1 + sc_lscape_bump_height : 0.9 - sc_lscape_bump_height;
   beta  = 1 - 2.0 * alpha;
   /* Compute height along the line */
   y = alpha + beta * x;
   /* Add some randomness */
   y += sc_tools_pnoise(ri[0], 7.0 * x + rd[0]) * 0.1;
   y += sc_tools_rnoise(ri[1], sc_lscape_bump_freq * x + rd[1]) * sc_lscape_bump_height;
   /* Coerse the range into the unit iterval (paranoid) */
   y = sc_tools_clamp(0.0, 1.0, y);

   return(y);
}



static double _sc_lscape_eval_mountain(double x) {
/* _sc_lscape_eval_mountain
   Evaluates the ``mountain'' profile function at point x */

   double alpha;
   double gamma;
   double y;

   /* Coerse the domain into the unit interval */
   x = sc_tools_clamp(0.0, 1.0, x);
   /* Compute the quadratic */
   alpha = 4.0 * (2.0 * sc_lscape_bump_height - 0.8);
   gamma = 0.1 + sc_lscape_bump_height;
   y = alpha * x * x - alpha * x + gamma;
   /* Add some randomness */
   y += sc_tools_pnoise_periodic(ri[0],  7,  7.0 * x + rd[0]) * 0.1;
   y += sc_tools_rnoise_periodic(ri[1], (int)sc_lscape_bump_freq, sc_lscape_bump_freq * x + rd[1]) * sc_lscape_bump_height;
   /* Coerse the range into the unit iterval (paranoid) */
   y  = sc_tools_clamp(0.0, 1.0, y);

   return(y);
}



static double _sc_lscape_eval_plains(double x) {
/* _sc_lscape_eval_plains
   Evaluates the ``plains'' profile function at point x. */

   double y;

   /* Coerse the domain into the unit interval */
   x = sc_tools_clamp(0.0, 1.0, x);

   y  = sc_tools_pnoise_periodic(ri[0],  7,  7.0 * x + rd[0]);
   y  = (y + 1.0) / 10.0 + 0.3;
   y += sc_tools_rnoise_periodic(ri[1], (int)sc_lscape_bump_freq, sc_lscape_bump_freq * x + rd[1]) * sc_lscape_bump_height;

   /* Coerse the range into the unit iterval (paranoid) */
   y  = sc_tools_clamp(0.0, 1.0, y);

   return(y);
}




static double _sc_lscape_eval_traditional(double x) {
/* _sc_lscape_eval_traditional
   Evaluates the ``traditional'' profile function at point x. */

   double y;

   /* Coerse the domain into the unit interval */
   x = sc_tools_clamp(0.0, 1.0, x);
   y  = sc_tools_pnoise_periodic(ri[0],   7,   7.0 * x + rd[0]) * 2.5;
   y += sc_tools_pnoise_periodic(ri[1],  17,  17.0 * x + rd[1]);
   y  = (1.0 - sc_lscape_bump_height) * y / 3.5;
   y += sc_tools_pnoise_periodic(ri[2], (int)sc_lscape_bump_freq, sc_lscape_bump_freq * x + rd[2]) * sc_lscape_bump_height;
   y  = (y + 1.0) / 2.0;
   /* Coerse the range into the unit iterval (paranoid) */
   y  = sc_tools_clamp(0.0, 1.0, y);

   return(y);
}




static double _sc_lscape_eval_valley(double x) {
/* _sc_lscape_eval_valley
   Evaluates the ``valley'' profile function at point x. */

   double alpha;
   double gamma;
   double y;

   /* Coerse the domain into the unit interval */
   x = sc_tools_clamp(0.0, 1.0, x);
   /* Compute the quadratic */
   alpha = 4.0 * (0.8 - 2.0 * sc_lscape_bump_height);
   gamma = 0.9 - sc_lscape_bump_height;
   y = alpha * x * x - alpha * x + gamma;
   /* Add some randomness */
   y += sc_tools_pnoise_periodic(ri[0],  7,  7.0 * x + rd[0]) * 0.1;
   y += sc_tools_rnoise_periodic(ri[1], (int)sc_lscape_bump_freq, sc_lscape_bump_freq * x + rd[1]) * sc_lscape_bump_height;
   /* Coerse the range into the unit iterval (paranoid) */
   y  = sc_tools_clamp(0.0, 1.0, y);

   return(y);
}



void sc_lscape_init(void) {
/* sc_lscape_init 
   Initializes lscape module for use.  This function must be called
   before calling any other functions of the module. */

   /* Initialize landscape tools */
   sc_tools_init();

   /* Set some arbitrary defaults */
   _sc_lscape_eval = _sc_lscape_eval_traditional;
   sc_lscape_screen_w = 768.0;
   sc_lscape_screen_h = 512.0;
   sc_lscape_bump_height = 0.0;
}



void sc_lscape_explicit_setup(int w, int h, double b, sc_lscape_generator g) {
/* sc_lscape_explicit_setup
   Performs all the stuff that needs to be done before generating a
   new landscape. */
   
   int i;

   /* Set screen dimensions */
   sc_lscape_screen_w = w;
   sc_lscape_screen_h = h;

   /* Compute bump height */
   sc_lscape_bump_height = (double)SC_LSCAPE_BUMP_HEIGHT_IN_PIXELS / (2.0 * (double)h) * b;

   /* Draw random parameters */
   for(i = 0; i < SC_LSCAPE_RANDOM_PARAMETERS; i++) {
      ri[i] = (long int)game_rand();
      rd[i] = (double)game_rand() / (double)GAME_RAND_MAX;
   }

   /* Choose the landscape profile function */
   switch(g) {
   case SC_LSCAPE_GENERATOR_NONE:
      _sc_lscape_eval = _sc_lscape_eval_none;
      break;
   case SC_LSCAPE_GENERATOR_CANYON:
      _sc_lscape_eval = _sc_lscape_eval_canyon;
      break;
   case SC_LSCAPE_GENERATOR_DOUBLE_MOUNTAIN :
      _sc_lscape_eval = _sc_lscape_eval_mountain;
      break;
   case SC_LSCAPE_GENERATOR_HILLSIDE :
      _sc_lscape_eval = _sc_lscape_eval_hillside;
      break;
   case SC_LSCAPE_GENERATOR_MOUNTAIN :
      _sc_lscape_eval = _sc_lscape_eval_mountain;
      break;
   case SC_LSCAPE_GENERATOR_PLAINS :
      _sc_lscape_eval = _sc_lscape_eval_plains;
      break;
   case SC_LSCAPE_GENERATOR_TRADITIONAL :
      _sc_lscape_eval = _sc_lscape_eval_traditional;
      break;
   case SC_LSCAPE_GENERATOR_VALLEY :
      _sc_lscape_eval = _sc_lscape_eval_valley;
      break;
   }
}



void sc_lscape_setup(const struct _sc_config *c, const struct _sc_land *l) {
/* sc_lscape_setup
   Front-end to sc_lscape_explicit_setup.  Note: the internals of this
   function are bound to change quite a bit. */

   int i;
   sc_land_generator gen;
   
   if(l != NULL) {
      /* Choose the lanscape profile function */
      gen = l->generator;
      if(gen == SC_LAND_GEN_RANDOM) gen = game_lrand(SC_LAND_GEN_RANDOM);
      switch(gen) {
      case SC_LAND_GEN_NONE :
         _sc_lscape_eval = _sc_lscape_eval_none;
         break;
      case SC_LAND_GEN_CANYON :
         _sc_lscape_eval = _sc_lscape_eval_canyon;
         break;
      case SC_LAND_GEN_DOUBLE_MOUNTAIN :
         _sc_lscape_eval = _sc_lscape_eval_mountain;
         break;
      case SC_LAND_GEN_HILLSIDE :
         _sc_lscape_eval = _sc_lscape_eval_hillside;
         break;
      case SC_LAND_GEN_MOUNTAIN :
         _sc_lscape_eval = _sc_lscape_eval_mountain;
         break;
      case SC_LAND_GEN_PLAINS :
         _sc_lscape_eval = _sc_lscape_eval_plains;
         break;
      case SC_LAND_GEN_RANDOM :
      case SC_LAND_GEN_TRADITIONAL :
         _sc_lscape_eval = _sc_lscape_eval_traditional;
         break;
      case SC_LAND_GEN_VALLEY :
         _sc_lscape_eval = _sc_lscape_eval_valley;
         break;
      }

      /* Get screen dimensions */
      sc_lscape_screen_w = c->fieldwidth;
      sc_lscape_screen_h = c->maxheight;

      /* Compute sc_lscape_bump_height */
      sc_lscape_bump_height = SC_LSCAPE_BUMP_HEIGHT_IN_PIXELS / (2.0 * sc_lscape_screen_h) * sc_tools_clamp(0.0, 1.0, l->bumpiness / 100.0);
      sc_lscape_bump_freq = floor(sc_lscape_screen_w / SC_LSCAPE_BUMP_WIDTH_IN_PIXELS);

      /* Draw new random parameters */
      for(i = 0; i < SC_LSCAPE_RANDOM_PARAMETERS; i++) {
         ri[i] = game_rand();
         rd[i] = (double)game_rand() / (double)GAME_RAND_MAX;
      }
   }
}

/* slscape.c ends */
