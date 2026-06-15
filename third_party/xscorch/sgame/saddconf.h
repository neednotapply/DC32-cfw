/* $Header: /fridge/cvs/xscorch/sgame/saddconf.h,v 1.14 2011-08-01 00:01:40 jacob Exp $ */
/*

   xscorch - saddconf.h       Copyright(c) 2001 Jacob Luna Lundberg
   jacob(at)gnifty.net        http://www.gnifty.net/

   Scorched basic read files on the fly stuff


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
#ifndef __saddconf_h_included
#define __saddconf_h_included


/* Includes */
#include <xscorch.h>


/* Forward structure decl's */
struct _sc_weapon_config;
struct _sc_accessory_config;


typedef enum _sc_addconf_type {
   SC_ADDCONF_ACCESSORIES,
   SC_ADDCONF_SCORINGS,
   SC_ADDCONF_WEAPONS
} sc_addconf_type;


/* Append to a conf list. */
bool sc_addconf_append_file(sc_addconf_type type, const char *filename, void *container);


#endif /* __saddconf_h_included */
