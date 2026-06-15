/* $Header: /fridge/cvs/xscorch/sgame/scolor.c,v 1.5 2009-04-26 17:39:38 jacob Exp $ */
/*
   
   xscorch - scolor.c         Copyright(c) 2000 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Scorched color manipulation
    

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
#include <scolor.h>        /* get color header */
#include <sconfig.h>       /* Need for field height */



static unsigned long _sc_grand_val;



static inline unsigned short _sc_color_gradient_rand(void) {

   _sc_grand_val = _sc_grand_val * 1664525L + 1013904223L;
   return((unsigned short)(_sc_grand_val ^ 0xffff));

}



int sc_color_gradient_index(bool dither, const int *gradient, int y) {

   const int *gp;
   int low;
   int wid;
   int probdither;

   /*** This is the dither subsystem ***/   
   if(dither) {
      gp = gradient + 1;
      while(*gp < y) ++gp;
      low = *(gp - 1);
      wid = *gp - low;
      if(wid > 0) probdither = (0x10000 * (y - low)) / wid;
      else        probdither = 0x10000;
      if(_sc_color_gradient_rand() < probdither) return(gp - gradient);
      return(gp - gradient - 1);
   } else {
      gp = gradient + 1;
      while(*gp < y) ++gp;
      return(gp - gradient - 1);
   }

}



void sc_color_gradient_init(const sc_config *c, sc_color *color) {

   struct timeval tv;
   int i;
   int j;

   gettimeofday(&tv, NULL);
   _sc_grand_val = tv.tv_usec / 1000 + tv.tv_sec * 1000;
   for(j = 0; j < SC_NUM_GRADIENTS; ++j) {
      for(i = 0; i < SC_MAX_GRADIENT_SIZE; ++i) {
         color->gradindex[j][i] = i * c->fieldheight / (color->gradsize[j] - 1);
      }
   }

}



sc_color *sc_color_new(void) {

   sc_color *color;
   
   color = (sc_color *)malloc(sizeof(sc_color));
   if(color == NULL) return(NULL);

   return(color);

}



void sc_color_free(sc_color **color) {

   if(color == NULL || *color == NULL) return;
   free(*color);
   *color = NULL;

}
