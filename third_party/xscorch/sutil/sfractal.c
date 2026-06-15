/* $Header: /fridge/cvs/xscorch/sutil/sfractal.c,v 1.5 2009-04-26 17:40:01 jacob Exp $ */
/*
   
   xscorch - sfractal.c       Copyright(c) 2000 Jake Post
    
   Creates fractal plasma clouds
    

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
#include <stdlib.h>

#include <sfractal.h>
#include <srand.h>



/***    Now entering the fractal plasma subsystem    ***/



/* Current version of this code based (somewhat) on an algorithm that is
   discussed at http://patate.virtualave.net/clouds/index.html */


                                 
/*
** Should probably just be stuck in sc_fractal_create
*/
static void _sc_fractal_seeds(unsigned char *data, int size) 
{
  int x;
  int y;
  int spacing = (1 << SC_FRACTAL_DEFAULT_DEPTH);
  int ys = size * spacing;
  int yl = size * size;
  int xl = size;

  for(y = 0; y < yl; y += ys)
  {
    for(x = y; x < xl; x += spacing)
      data[x] = sys_lrand(256);
    xl += ys;
  }
}

/*
** Note - Slightly different from the original in that the horiz/vert
** only use 2 neighbors instead of four; this simplifies things.  If
** it's not good, I can change it to the 4 point avg.; it takes more
** computation (obviously) and needs a special case for the borders
** (as there's only 3 neighbors), but isn't difficult in any sense.
** Would still be faster than original due to 2^n + 1 board.
**
** May 13 2000
** Noticed I was being stupid by taking 2D indexes and using multiplication
** to translate them into 1D index within loop - 2D indexes needed as we're
** making a 2D effect but 1D index now used directly, there is only addition
** within loops now
*/
static void _sc_fractal_average(unsigned char *data, int size, int spacing)
{
  int spacing2 = (spacing >> 1); /*used as offset*/
  int basey;                     /*1D offset of current row*/
  int basex;                     /*...and x/y combo*/
  int bases = spacing2 * size;   /*distance spacing2 rows away*/
  int yinc  = (bases << 1);      /*similar but with spacing*/
  int size2 = size * size;       /*first illegal index*/
  int xsize = size;              /*Last index of current row + 1*/

  /*Average horizonal; use "parent" values to left and right*/
  for(basey = 0; basey < size2; basey += yinc)
  {
    for(basex = basey + spacing2; basex < xsize; basex += spacing)
      data[basex] = (data[basex - spacing2] + data[basex + spacing2]) >> 1;
    xsize += yinc;
  }
  
  /*Average vertical; use "parent" values above and below*/
  xsize = bases + size;
  for(basey = bases; basey < size2; basey += yinc)
  {
    for(basex = basey; basex < xsize; basex += spacing)
      data[basex] = (data[basex + bases] + data[basex - bases]) >> 1;
    xsize += yinc;
  }

  /*Average center; use above, below, left, and right*/
  xsize = bases + size;
  for(basey = bases; basey < size2; basey += yinc)
  {
    for(basex = basey + spacing2; basex < xsize; basex += spacing)
    {
      data[basex] = (data[basex + bases]    + data[basex - bases] +
                     data[basex + spacing2] + data[basex - spacing2]) >> 2;
    }
    xsize += yinc;
  }
}

unsigned char *sc_fractal_create(int *size, int requestedsize)
{
  /*** WARNING:::  SIZE MUST BE A POWER OF _2_ + 1 ***/
  
  unsigned char *data;
  int spacing = *size = (1 << SC_FRACTAL_DEFAULT_DEPTH);
  
  /*SC_PROFILE_BEGIN("fr")*/

  while(requestedsize > *size + 1) 
    *size = (*size << 1);

  (*size) |= 1;

  data = (unsigned char *)malloc((*size) * (*size));

  if(data == NULL) 
    return(NULL);
  
  _sc_fractal_seeds(data, *size);
  
  while(spacing > 1) 
  {
    _sc_fractal_average(data, *size, spacing);
    spacing >>= 1;
  }
  
  /*SC_PROFILE_END*/
  
  return(data);
}
