/* $Header: /fridge/cvs/xscorch/sutil/sslink.c,v 1.6 2011-08-01 00:01:44 jacob Exp $ */
/*

   xscorch - sslink.c         Copyright(c) 2000 Jacob Luna Lundberg
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
#include <stdlib.h>

#include <sslink.h>



slinklist *slink_last(slinklist *head) {
/* slink_last()
   Return a pointer to the last item in an slink list.
   Please, please, folks, don't call this on a ring buffer... */

   if(head == NULL) return(NULL);
   while(head->next != NULL) head = head->next;
   return(head);

}



slinklist *slink_insert(slinklist *head, slinklist *item) {
/* slink_insert()
   Insert a list item at the head of the list. */

   if(item == NULL) return(head);
   item->next = head;
   return(item);

}



slinklist *slink_append(slinklist *head, slinklist *item) {
/* slink_append()
   Append a list or item to the tail of the list. */

   if(head == NULL) return(item);
   slink_last(head)->next = item;
   return(head);

}
