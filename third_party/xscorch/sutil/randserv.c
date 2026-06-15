/* $Header: /fridge/cvs/xscorch/sutil/randserv.c,v 1.4 2009-04-26 17:40:00 jacob Exp $ */
/*--------------------------------------------------------------------
 *
 *  randserv.c -- Random deviate generator
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

#include <randserv.h>
#include <srand.h>



static double normal_deviates[2];
static int    normal_deviates_left = 0;

/*--------------------------------------------------------------------
 *  rand_uniform() 
 *
 *  Description:
 *     Returns an uniform deviate drawn from the closed 
 *     interval [0, 1].
 */

inline double rand_uniform(void) 
{
  return (double)game_rand() / (double)GAME_RAND_MAX;
}

/*--------------------------------------------------------------------
 *  rand_discrete(min, max) 
 *
 *  Description:
 *     Return a discrete deviate bounded between min and max.
 */

long rand_discrete(long min, long max)
{
  long temp;

  /* Make sure min is less than max */
  if ( min > max ) {
    temp = min;
    min = max;
    max = temp;
  }
  
  return min + (long)((max - min + 1.0) * game_rand() / (GAME_RAND_MAX + 1.0));
}

/*--------------------------------------------------------------------
 *  generate_normal_deviates() <internal>
 *
 *  Description:
 *     Generates two independent standard normal deviates and 
 *     stores them in the normal_deviates[] table.
 *
 *  Note:
 *     This function utilizes the ``polar method'' for generating
 *     normal deviations due to Box, Muller, and Marsaglia as 
 *     described in Knuth, D. E. (1981) The Art of Computer
 *     Programming, Vol. 2, pp. 117-8.  
 */

static void generate_normal_deviates(void) 
{
  double v_1, v_2, s, t;

  do {
    v_1 = 2.0 * (double)game_rand() / (double)GAME_RAND_MAX - 1.0;
    v_2 = 2.0 * (double)game_rand() / (double)GAME_RAND_MAX - 1.0;
    s = v_1 * v_1 + v_2 * v_2;
  } while ( s >= 1 );
  t = sqrt((-2.0 * log(s)) / s);
  normal_deviates[0] = v_1 * t;
  normal_deviates[1] = v_2 * t;
  normal_deviates_left = 2;
}

/*--------------------------------------------------------------------
 *  rand_normal() 
 *  
 *  Description:
 *     Returns a standard normal deviate.
 */

double rand_normal(void) 
{
  if ( !normal_deviates_left )
    generate_normal_deviates();

  return normal_deviates[--normal_deviates_left]; 
}

/* randserv.c ends */

