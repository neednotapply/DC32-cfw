/* $Header: /fridge/cvs/xscorch/sgame/stankpro.h,v 1.8 2009-04-26 17:39:45 jacob Exp $ */
/*
   
   xscorch - stankpro.h       Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/
    
   Tank profiles
    

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
#ifndef __stank_profile_included_h
#define __stank_profile_included_h



/* Includes */
#include <xscorch.h>



#define  SC_TANK_PROFILE_NAME_SIZE  32
#define  SC_TANK_PROFILE_SOLID      0xff
#define  SC_TANK_PROFILE_CLEAR      0x00
#define  SC_TANK_PROFILE_MIN_AREA   48

#define  SC_TANK_NORMAL_HARDNESS    100
#define  SC_TANK_NORMAL_EFFICIENCY  100



typedef struct _sc_tank_profile {
   char name[SC_TANK_PROFILE_NAME_SIZE];
   int radius;
   int turretradius;
   int shelfsize;
   int efficiency;
   int hardness;
   bool mobile;
   unsigned char *data;
   struct _sc_tank_profile *next;
} sc_tank_profile;



bool sc_tank_profile_add(sc_tank_profile **plist, const char *datafile);
void sc_tank_profile_free(sc_tank_profile **profile);
const sc_tank_profile *sc_tank_profile_lookup(const sc_tank_profile *plist, int index);
int sc_tank_profile_index_of(const sc_tank_profile *plist, const sc_tank_profile *profile);
int sc_tank_profile_size(const sc_tank_profile *plist);



#endif /* __stank_profile_included_h */
