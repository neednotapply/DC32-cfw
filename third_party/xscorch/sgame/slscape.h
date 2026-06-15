/* $Header: /fridge/cvs/xscorch/sgame/slscape.h,v 1.3 2009-04-26 17:39:41 jacob Exp $ */
/*
 
  slscape.h
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


#ifndef __slscape_h_included
#define __slscape_h_included


/* Our includes */
#include <xscorch.h>


/* Some fixed landscape parameters */
#define SC_LSCAPE_BUMP_HEIGHT_IN_PIXELS  30.0
#define SC_LSCAPE_BUMP_WIDTH_IN_PIXELS    5.0


/* Forward definitions */
struct _sc_config;
struct _sc_land;


/* Landscape generator types */
typedef enum _sc_lscape_generator {
   SC_LSCAPE_GENERATOR_NONE,
   SC_LSCAPE_GENERATOR_CANYON,
   SC_LSCAPE_GENERATOR_DOUBLE_MOUNTAIN,
   SC_LSCAPE_GENERATOR_HILLSIDE,
   SC_LSCAPE_GENERATOR_MOUNTAIN,
   SC_LSCAPE_GENERATOR_PLAINS,
   SC_LSCAPE_GENERATOR_TRADITIONAL,
   SC_LSCAPE_GENERATOR_VALLEY
} sc_lscape_generator;


/* Initialization and setting things up for each round */
void sc_lscape_init(void);
void sc_lscape_explicit_setup(int w, int h, double b, sc_lscape_generator g);
void sc_lscape_setup(const struct _sc_config *c, const struct _sc_land *l);


/* Interface to the profile evaluating function */
#define sc_lscape_eval(x)  ((*_sc_lscape_eval)(x))
double (*_sc_lscape_eval)(double x);


#endif /* __slscape_h_included */

/* slscape.h ends */
