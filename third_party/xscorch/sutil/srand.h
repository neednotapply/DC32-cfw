/* $Header: /fridge/cvs/xscorch/sutil/srand.h,v 1.6 2009-04-26 17:40:02 jacob Exp $ */
/*

   xscorch - srand.h          Copyright(c) 2000 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Scorched random numbers


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
#ifndef __srand_h_included
#define __srand_h_included

#include <xscorch.h>


/* Random functions */
void     sys_randomize(void);
long     sys_rand(void);
double   sys_drand(void);
long     sys_lrand(long max);

#define  GAME_RAND_MAX  RAND_MAX
void     game_randomize(dword seed);
dword    game_rand(void);
dword    game_rand_peek(void);
double   game_drand(void);
dword    game_lrand(dword max);


#endif /* __srand_h_included */
