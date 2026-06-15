/* $Header: /fridge/cvs/xscorch/sutil/shash.h,v 1.9 2011-08-01 00:01:44 jacob Exp $ */
/*

   xscorch - shash.h          Copyright(c) 2001-2003 Jacob Luna Lundberg
   jacob(at)gnifty.net        http://www.gnifty.net/

   A very slim hashing implementation for xscorch.


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
#ifndef __shash_h_included
#define __shash_h_included



/* Maximum size we've evaluated this hash function's fairness to. */
#define  SHASH_MAX_BITS         31



/* Support functions that we export. */
int shash_string_to_int(const char *item);



/* Predefined operations on shashes. */
int shash(int bits, unsigned int item);
int shash_string(int bits, const char *item);



#endif /* __shash_h_included */
