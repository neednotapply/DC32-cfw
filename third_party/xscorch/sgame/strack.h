/* $Header: /fridge/cvs/xscorch/sgame/strack.h,v 1.5 2009-04-26 17:39:45 jacob Exp $ */
/*

   xscorch - strack.h         Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Scorched basic weapon tracking


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
#ifndef __strack_h_included
#define __strack_h_included


/* Includes */
#include <xscorch.h>
#include <sweapon.h>


/* Forward declarations */
struct _sc_explosion;
struct _sc_weapon;
struct _sc_config;


/* Return values for weapon tracking */
typedef enum _sc_weapon_track_result {
   SC_WEAPON_TRACK_NO_ACTION = 0,   /* No action; tracking complete */
   SC_WEAPON_TRACK_NEED_RECURSE,    /* Need to recursively call */
   SC_WEAPON_TRACK_DETONATE,        /* Detonating a weapon */
   SC_WEAPON_TRACK_SIZZLE           /* Weapon sizzled */
} sc_weapon_track_result;


/* Create and track all weapons */
sc_weapon_track_result sc_weapon_track_all(struct _sc_config *c, struct _sc_explosion **e);


#endif /* __strack_h_included */

