/* $Header: /fridge/cvs/xscorch/sgame/sspill.h,v 1.5 2009-04-26 17:39:44 jacob Exp $ */
/*

   xscorch - sspill.h         Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Scorched spillage functions (napalm, liquid dirt)


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
#ifndef __sspill_h_included
#define __sspill_h_included


/* Includes */
#include <xscorch.h>


/* Forward type definitions */
struct _sc_config;
struct _sc_land;


/* Type definitions */
typedef struct _sc_spill {
   int size;            /* Size of spillage (number of coordinates) */
   int *spillx;         /* Array of X coordinates describing spill */
   int *spilly;         /* Array of Y coordinates describing spill */
   int index;           /* Spill index variable */
   int count;           /* Final draw count */
} sc_spill;


/* Spill data management */
sc_spill *sc_spill_new(const struct _sc_config *c, const struct _sc_land *l,
                       int size, int centerx, int centery);
void sc_spill_free(sc_spill **sp);


#endif /* __sspill_h_included */
