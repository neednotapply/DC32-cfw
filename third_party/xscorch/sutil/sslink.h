/* $Header: /fridge/cvs/xscorch/sutil/sslink.h,v 1.6 2011-08-01 00:01:44 jacob Exp $ */
/*

   xscorch - sslink.h         Copyright(c) 2000 Jacob Luna Lundberg
   jacob(at)gnifty.net        http://www.gnifty.net/

   A very slim slink list library for xscorch.


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
#ifndef __sslink_h_included
#define __sslink_h_included



/* The basic slink list prototype. */
typedef struct _slinklist {
   struct _slinklist  *next;    /* Place the next pointer first, folks */
   void               *data;    /* I recommend placing the data second */
} slinklist;



/* Predefined operations on slink lists. */
slinklist *slink_last(slinklist *head);
slinklist *slink_insert(slinklist *head, slinklist *item);
slinklist *slink_append(slinklist *head, slinklist *item);



#endif /* __sslink_h_included */
