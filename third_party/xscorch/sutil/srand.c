/* $Header: /fridge/cvs/xscorch/sutil/srand.c,v 1.7 2009-04-26 17:40:02 jacob Exp $ */
/*

   xscorch - srand.c          Copyright(c) 2000 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Scorched random number generator


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

#include <srand.h>



void sys_randomize(void) {

   struct timeval t;
   gettimeofday(&t, NULL);
   srand(t.tv_sec * 1000 + t.tv_usec / 1000);
   game_randomize(sys_rand());

}



long sys_rand(void) {

   return(rand());

}



double sys_drand(void) {

   return(sys_rand() / (RAND_MAX + 1.0));

}



long sys_lrand(long max) {

   return((long)(sys_drand() * max));

}



static dword _sc_game_random_value;



void game_randomize(dword seed) {

   _sc_game_random_value = seed;

}



dword game_rand_peek(void) {

   dword result;

   result = _sc_game_random_value * 1664525L + 1013904223L;
   result = result % GAME_RAND_MAX;
   return(result);

}



dword game_rand(void) {

   _sc_game_random_value = game_rand_peek();
   return(_sc_game_random_value);

}



double game_drand(void) {

   return(game_rand() / (GAME_RAND_MAX + 1.0));

}



dword game_lrand(dword max) {

   return((dword)(game_drand() * max));

}
