/* $Header: /fridge/cvs/xscorch/sgame/scolor.h,v 1.6 2009-04-26 17:39:38 jacob Exp $ */
/*
   
   xscorch - scolor.h         Copyright(c) 2000 Justin David Smith
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
#ifndef __scolor_h_included
#define __scolor_h_included


#include <xscorch.h>


/* Forward structure declarations */
struct _sc_window;
struct _sc_config;


/* General, useful constants */
#define  SC_MAX_GRADIENT_SIZE    128      /* Maximum size of the land gradient. */

/* Definitions of specific, useful gradients */
typedef enum _sc_gradient_list {
   SC_GRAD_GROUND = 0,     /* Ground */
   SC_GRAD_NIGHT_SKY,      /* Night sky */
   SC_GRAD_EXPLOSION,      /* Explosion */
   SC_GRAD_FUNKY_EXPLOSION,/* Funky explosions */
   SC_GRAD_FIRE_SKY,       /* Fiery sky */
   SC_GRAD_MAGNETIC,       /* Gradient for magnetic shields */
   SC_GRAD_SHIELD,         /* Gradient for standard shields */
   SC_GRAD_FORCE,          /* Gradient for force shields */
   SC_GRAD_FLAMES,         /* Flaming idiots! */
   SC_NUM_GRADIENTS        /* Total number of gradients */
} sc_gradient_list;


typedef struct _sc_color {
   /* Size of each gradient */
   int gradsize[SC_NUM_GRADIENTS];     /* Size of all gradients */

   /* These constants are useful for dithering; what height does each color begin? */
   int gradindex[SC_NUM_GRADIENTS][SC_MAX_GRADIENT_SIZE];
} sc_color;


sc_color *sc_color_new(void);
void sc_color_free(sc_color **color);


void sc_color_gradient_init(const struct _sc_config *c, sc_color *color);
int sc_color_gradient_index(bool dither, const int *gradient, int y);


#endif /* __scolor_h_included */
