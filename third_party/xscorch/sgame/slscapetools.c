/* $Header: /fridge/cvs/xscorch/sgame/slscapetools.c,v 1.4 2009-04-26 17:39:41 jacob Exp $ */
/*--------------------------------------------------------------------
 *
 *  slscapetools.c
 *  Copyright (C) 2000 Matti Hänninen
 *
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to 
 *
 *    The Free Software Foundation, Inc.
   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 * 
 */
#include <math.h>
#include <stdlib.h>

#include <slscapetools.h>

#include <sutil/srand.h>


/*====================================================================
 *  I n t e r n a l   d e f i n i t i o n s   a n d   m a c r o s
 */

#define SC_TOOLS_RANDTABLE_SIZE  4096
#define SC_TOOLS_LOOKUP_SIZE     4096

/*====================================================================
 *  I n t e r n a l   v a r i a b l e s
 */

static double r_table[SC_TOOLS_RANDTABLE_SIZE];
static double p_table[SC_TOOLS_RANDTABLE_SIZE];
static double p1_mult_lookup[SC_TOOLS_LOOKUP_SIZE];
static double p2_mult_lookup[SC_TOOLS_LOOKUP_SIZE];
static double r1_mult_lookup[SC_TOOLS_LOOKUP_SIZE];
static double r2_mult_lookup[SC_TOOLS_LOOKUP_SIZE];

/*====================================================================
 *  I n t e r n a l   f u n c t i o n s
 */

/*--------------------------------------------------------------------
 *  _sc_tools_hermite(p1, p2, r1, r2, t)
 *
 *  Description:
 *     Evaluates the Hermite blending function at time t given end
 *     points p1 and p2 and corresponding tangents r1 and r2.
 */

double _sc_tools_hermite(double p1, double p2, double r1, double r2, double t)
{
  long int i = (long int)(t * (SC_TOOLS_LOOKUP_SIZE - 1.0));

  return p1 * p1_mult_lookup[i] + p2 * p2_mult_lookup[i] +
         r1 * r1_mult_lookup[i] + r2 * r2_mult_lookup[i];
}

/*--------------------------------------------------------------------
 *  _sc_tools_phermite(p1, p2, t)
 *
 *  Description:
 *     Evaluates the Hermite blending function at time t given end
 *     points p1 and p2 and assuming that corresponding tangents both
 *     are equal to zero.
 */

double _sc_tools_phermite(double p1, double p2, double t)
{
  long int i = (long int)(t * (SC_TOOLS_LOOKUP_SIZE - 1.0));

  return p1 * p1_mult_lookup[i] + p2 * p2_mult_lookup[i];
}

/*--------------------------------------------------------------------
 *  _sc_tools_rhermite(r1, r2, t)
 *
 *  Description:
 *     Evaluates the Hermite blending functions at time t given 
 *     tangents r1 and r2 corresponding to end points p1 and p2,
 *     respectively, and assuming that the end points are both
 *     equal to zero.
 */

double _sc_tools_rhermite(double r1, double r2, double t)
{
  long int i = (long int)(t * (SC_TOOLS_LOOKUP_SIZE - 1.0));

  return r1 * r1_mult_lookup[i] + r2 * r2_mult_lookup[i];
}

/*====================================================================
 *  F u n c t i o n s
 */

/*--------------------------------------------------------------------
 *  sc_tools_init()
 *
 *  Description:
 *     Initializes the module.  This function must be called before
 *     calling any other functions in this module.
 */

void sc_tools_init(void)
{
  long int i;
  double t;
  double t_squared;
  double t_cubed;

#ifdef SC_TOOLS_DEBUG
  printf("sc_tools_init: Computing random points and tangents ..\n");
#endif /* SC_TOOLS_DEBUG */

  /* Compute tables for random points and tangents */
  for ( i = 0; i < SC_TOOLS_RANDTABLE_SIZE; i++ ) {
    p_table[i] = 2.0 * (double)game_rand() / (double)GAME_RAND_MAX - 1.0;
    r_table[i] = 8.0 * (double)game_rand() / (double)GAME_RAND_MAX - 4.0;
  }

#ifdef SC_TOOLS_DEBUG
  printf("sc_tools_init: Computing look-up tables for Hermite blending function ..\n", i);
#endif /* SC_TOOLS_DEBUG */

  /* Compute look-up tables for the multipliers of p1, p2, t1, and t2 
   * in the Hermite blending function */
  for ( i = 0; i < SC_TOOLS_LOOKUP_SIZE; i++ ) {
    t = i / (SC_TOOLS_LOOKUP_SIZE - 1.0);
    t_squared = t * t;
    t_cubed = t * t_squared;
    p1_mult_lookup[i] = 2.0 * t_cubed - 3.0 * t_squared + 1;
    p2_mult_lookup[i] = -2.0 * t_cubed + 3.0 * t_squared;
    r1_mult_lookup[i] = t_cubed - 2.0 * t_squared + t;
    r2_mult_lookup[i] = t_cubed - t_squared;
  }
}

/*--------------------------------------------------------------------
 *  sc_tools_pnoise(offset, x)
 *
 *  Description:
 *     Evaluates a continuous noise function at point x.
 */

double sc_tools_pnoise(long int offset, double x)
{
  long int i_lower;
  long int i_upper;
  double t;

  t = x - floor(x);
  i_lower = ((long int)x + offset) % SC_TOOLS_RANDTABLE_SIZE;
  i_upper = (i_lower + 1) % SC_TOOLS_RANDTABLE_SIZE;
  
  return _sc_tools_phermite(p_table[i_lower], p_table[i_upper], t);
}

/*--------------------------------------------------------------------
 *  sc_tools_pnoise_periodic(offset, period, x)
 *
 *  Description:
 *     Evaluates a periodical continuous noise function at point x.
 */

double sc_tools_pnoise_periodic(long int offset, long int period, double x)
{
  long int i_lower;
  long int i_upper;
  double t;

  t = x - floor(x);
  i_lower = (long int)x % period;
  i_upper = (i_lower + 1) % period;
  i_lower = (i_lower + offset) % SC_TOOLS_RANDTABLE_SIZE;
  i_upper = (i_upper + offset) % SC_TOOLS_RANDTABLE_SIZE;
  
  return _sc_tools_phermite(p_table[i_lower], p_table[i_upper], t);
}

/*--------------------------------------------------------------------
 *  sc_tools_rnoise(offset, x)
 *
 *  Description:
 *     Evaluates a continuous noise function at point x.
 */

double sc_tools_rnoise(long int offset, double x)
{
  long int i_lower;
  long int i_upper;
  double t;

  t = x - floor(x);
  i_lower = ((long int)x + offset) % SC_TOOLS_RANDTABLE_SIZE;
  i_upper = (i_lower + 1) % SC_TOOLS_RANDTABLE_SIZE;
  
  return _sc_tools_rhermite(r_table[i_lower], r_table[i_upper], t);
}

/*--------------------------------------------------------------------
 *  sc_tools_rnoise_periodic(offset, period, x)
 *
 *  Description:
 *     Evaluates a periodical continuous noise function at point x.
 */

double sc_tools_rnoise_periodic(long int offset, long int period, double x)
{
  long int i_lower;
  long int i_upper;
  double t;

  t = x - floor(x);
  i_lower = (long int)x % period;
  i_upper = (i_lower + 1) % period;
  i_lower = (i_lower + offset) % SC_TOOLS_RANDTABLE_SIZE;
  i_upper = (i_upper + offset) % SC_TOOLS_RANDTABLE_SIZE;
  
  return _sc_tools_rhermite(r_table[i_lower], r_table[i_upper], t);
}

/*--------------------------------------------------------------------
 *  sc_tools_clamp(a, b, x)
 *
 *  Description:
 *     Crops the value of x if it falls outside the interval defined
 *     by a and b.
 */

double sc_tools_clamp(double a, double b, double x)
{
  if ( x > a && x > b )
    return a > b ? a : b;
  if ( x < a && x < b )
    return a < b ? a : b;

  return x;
}

/*--------------------------------------------------------------------
 *  sc_tools_mod(a, x)
 *
 *  Description:
 *     Returns the (real) modulus a of x.
 */

double sc_tools_mod(double a, double x)
{
  double b = x / a;

  return (b - floor(b)) * a;
}

/*--------------------------------------------------------------------
 *  sc_tools_step(a, x)
 *
 *  Description:
 *     Returns zero for all x below a and unity otherwise.
 */

double sc_tools_step(double a, double x)
{
  return (double)(x > a);
}

/*--------------------------------------------------------------------
 *  sc_tools_smoothstep(a, b, x)
 *
 *  Description:
 *     Returns zero for all x below a and b and unity for all x above
 *     a and b.  For intermediate values returns a ``smooth'' 
 *     interpolation from zero to unity.
 */

double sc_tools_smoothstep(double a, double b, double x)
{
  if ( x > a && x > b )
    return 1.0;
  if ( x < a && x < b )
    return 0.0;

  return _sc_tools_phermite(0.0, 1.0, (x - a) / (b - a));
}

/* slscapetools.c ends */
