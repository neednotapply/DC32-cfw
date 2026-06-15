/* $Header: /fridge/cvs/xscorch/sutil/shashlist.h,v 1.6 2011-08-01 00:01:44 jacob Exp $ */
/*

   xscorch - shashlist.h      Copyright(c) 2003 Jacob Luna Lundberg
   jacob(at)gnifty.net        http://www.gnifty.net/

   Doubly linked hash chain implementation for xscorch.


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
#ifndef __shashlist_h_included
#define __shashlist_h_included



#define  SHASHLIST_ALL_CLASSES     0
#define  SHASHLIST_UNCLASSIFIED    1
#define  SHASHLIST_FIRST_CLASS     2


#define  SHASHLIST_MAX_KEYSTR_LEN  (1<<8)



typedef struct _shashlist_item {
   struct _shashlist_item *prev;
   struct _shashlist_item *next;
   struct _shashlist_item *chain;
   int class, key;
   char *keystr;
   void *data;
} shashlist_item;


typedef struct _shashlist {
   shashlist_item **hash;
   shashlist_item *head;
   shashlist_item *tail;
   unsigned int hashbits;
} shashlist;



/* Hash DLL creation and obliteration. */
shashlist_item *shashlist_item_new(void *data, int key, const char *keystr, int class);
void shashlist_item_free(shashlist_item **item);
shashlist *shashlist_new(unsigned int hashsize);
void shashlist_free(shashlist **list);


/* Hash DLL item manipulations. */
shashlist_item *shashlist_insert_by_int(shashlist *list, void *data, int class, int key);
shashlist_item *shashlist_insert_by_string(shashlist *list, void *data, int class, const char *key);
shashlist_item *shashlist_remove_by_int(shashlist *list, int key);
shashlist_item *shashlist_remove_by_string(shashlist *list, const char *key);
shashlist_item *shashlist_find_by_int(const shashlist *list, int key);
shashlist_item *shashlist_find_by_string(const shashlist *list, const char *key);
shashlist_item *shashlist_find_next(const shashlist *list, shashlist_item *item, int class);
shashlist_item *shashlist_find_prev(const shashlist *list, shashlist_item *item, int class);



#endif /* __shashlist_h_included */
