/* $Header: /fridge/cvs/xscorch/sgame/sinfo.h,v 1.10 2009-05-24 23:59:04 jacob Exp $ */
/*

   xscorch - sinfo.h          Copyright(c) 2000-2004 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Scorched information header


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
#ifndef __sinfo_h_included
#define __sinfo_h_included


#include <xscorch.h>


/* Copyright notice -- 
   Principle maintainers of this program.  */
#define  SC_COPYRIGHT_NOTICE  \
         "Copyright(c) 2000-2004 Justin David Smith\n"\
         "Copyright(c) 2000-2009 Jacob Luna Lundberg\n"\
         "Licensed under the GNU General Public License, version 2\n"\


/* Contributors notice -- 
   Add your name to the contributors list if you
   make changes that you have copyrighted.  */
#define  SC_CONTRIBUTORS_NOTICE  \
         "Matti Hänninen - land generation, (c)2000\n"\
         "Jason House - shielding, economy file, etc, (c)2003\n"\
         "Jake Post - plasma fractals, debugging, (c)2000-2003\n"\
         "Nickolai Zeldovich - random player locations, (c)2000\n"\


void sc_info(void);


#endif /* __sinfo_h_included */
