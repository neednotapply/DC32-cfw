/* $Header: /fridge/cvs/xscorch/sutil/sfractal.h,v 1.4 2009-04-26 17:40:01 jacob Exp $ */
/*
   
   xscorch - sfractal.h       Copyright(c) 2001,2000 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/
    
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
#ifndef __sfractal_h_included
#define __sfractal_h_included


/* Includes */
#include <xscorch.h>


/* Fractal constants */
#define  SC_FRACTAL_DEFAULT_DEPTH      4
#define  SC_FRACTAL_DEFAULT_ROUGH      6


/* Functions */
unsigned char *sc_fractal_create(int *size, int requestedsize);


#endif /* __sfractal_h_included */
