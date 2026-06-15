/* $Header: /fridge/cvs/xscorch/sutil/shashlist.c,v 1.14 2011-08-01 00:01:44 jacob Exp $ */
/*

   xscorch - shashlist.c      Copyright(c) 2003 Jacob Luna Lundberg
   jacob(at)gnifty.net        http://www.gnifty.net/

   Doubly linked hash chain library for xscorch.


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

#include <xscorch.h>

#include <shashlist.h>
#include <shash.h>

#include <libj/jstr/libjstr.h>



shashlist_item *shashlist_item_new(void *data, int key, const char *keystr, int class) {
/* shashlist_item_new
   Create and initialize a new shashlist_item. */

   shashlist_item *item;
   int len;

   item = (shashlist_item *)malloc(sizeof(shashlist_item));
   if(item == NULL)
      return(NULL);

   /* Set the string key, if any. */
   if(keystr == NULL) {
      item->keystr = NULL;
   } else {
      len = strblenn(keystr, SHASHLIST_MAX_KEYSTR_LEN) + 1;
      item->keystr = (char *)malloc(len);
      if(item->keystr == NULL) {
         free(item);
         return(NULL);
      }
      strcopyb(item->keystr, keystr, len);
   }

   item->class  = class;
   item->data   = data;
   item->key    = key;

   item->chain  = NULL;
   item->next   = NULL;
   item->prev   = NULL;

   return(item);

}



void shashlist_item_free(shashlist_item **item) {
/* shashlist_item_free
   Deallocate a shashlist_item. */

   if(item == NULL || *item == NULL) return;

   (*item)->data = (void *)0xdeadbeef;

   if((*item)->keystr != NULL)
      free((*item)->keystr);

   free(*item);
   *item = NULL;

}



shashlist *shashlist_new(unsigned int hashsize) {
/* shashlist_new
   Create and initialize a new shashlist. */

   shashlist *list;
   int roundup = 0;

   list = (shashlist *)malloc(sizeof(shashlist));
   if(list == NULL)
      return(NULL);

   /* The current implementation requires a hash size in 2^n. */
   list->hashbits = 0;
   while(hashsize > 1) {
      ++list->hashbits;
      if(hashsize % 2)
         ++roundup;
      hashsize = hashsize >> 1;
   }
   if(roundup) ++list->hashbits;

   /* SHASH_MAX_BITS comes from shash.h */
   if(list->hashbits > SHASH_MAX_BITS) list->hashbits = SHASH_MAX_BITS;

   /* Allocate the (possibly large) array of item pointers for the hash. */
   list->hash = (shashlist_item **)malloc((1 << list->hashbits) * sizeof(shashlist_item *));
   if(list->hash == NULL) {
      free(list);
      return(NULL);
   }

   list->head = NULL;
   list->tail = NULL;

   return(list);

}



void shashlist_free(shashlist **list) {
/* shashlist_free
   Destroy and deallocate an entire hash list. */

   if(list == NULL || *list == NULL) return;

   /* Destroy the individual items first. */
   while((*list)->head != NULL) {
      (*list)->tail = (*list)->head;
      (*list)->head = (*list)->head->next;
      shashlist_item_free(&(*list)->tail);
   }

   free((*list)->hash);
   (*list)->hash = (shashlist_item **)0xdeadbeef;

   free(*list);
   *list = NULL;

}



shashlist_item *shashlist_insert_by_int(shashlist *list, void *data, int class, int key) {
/* shashlist_append_by_int
   Append a new data item onto a hash dll.
   This will return NULL if the item is a duplicate or if allocation fails. */

   int hashindex;
   shashlist_item *item, *temp;

   if(list == NULL || data == NULL || key < 0)
      return(NULL);

   item = shashlist_item_new(data, key, NULL, class);
   if(item == NULL)
      return(NULL);

   /* Add the def to the dll */
   if(list->head == NULL) {
      /* Empty list */
      list->tail = list->head = item;
   } else {
      /* Find the proper place in the list. */
      temp = list->head;
      while(temp->next != NULL && temp->next->key <= key)
         temp = temp->next;
      /* Cannot insert duplicate keys. */
      if(temp->key == key) {
         shashlist_item_free(&item);
         return(NULL);
      }
      /* Fix up the case where we are appending instead of inserting. */
      if(temp == list->tail) {
         list->tail = item;
      } else {
         temp->next->prev = item;
         item->next = temp->next;
      }
      temp->next = item;
      item->prev = temp;
   }

   /* Add the def to the hash chains */
   hashindex = shash(list->hashbits, key);
   if(list->hash[hashindex] == NULL) {
      /* Nothing on hash chain */
      list->hash[hashindex] = item;
   } else {
      /* Already an item on the hash chain */
      temp = list->hash[hashindex];
      while(temp->chain != NULL)
         temp = temp->chain;
      temp->chain = item;
   }

   /* Return the fully formed shashlist entry. */
   return(item);

}



shashlist_item *shashlist_insert_by_string(shashlist *list, void *data, int class, const char *key) {
/* shashlist_insert_by_string
   Append a new data item onto a hash dll, keyed by string.
   This will return NULL if the item is a duplicate or if allocation fails. */

   int hashindex;
   shashlist_item *item, *temp;

   if(list == NULL || data == NULL || key == NULL)
      return(NULL);

   item = shashlist_item_new(data, 0, key, class);
   if(item == NULL)
      return(NULL);

   /* Add the def to the dll */
   if(list->head == NULL) {
      /* Empty list */
      list->tail = list->head = item;
   } else {
      /* Find the proper place in the list. */
      temp = list->head;
      while(temp->next != NULL && strcomp(temp->next->keystr, key) <= 0)
         temp = temp->next;
      /* Cannot insert duplicate keys. */
      if(strcomp(temp->keystr, key) == 0) {
         shashlist_item_free(&item);
         return(NULL);
      }
      /* Fix up the case where we are appending instead of inserting. */
      if(temp == list->tail) {
         list->tail = item;
      } else {
         temp->next->prev = item;
         item->next = temp->next;
      }
      temp->next = item;
      item->prev = temp;
   }

   /* Add the def to the hash chains */
   hashindex = shash_string(list->hashbits, key);
   if(list->hash[hashindex] == NULL) {
      /* Nothing on hash chain */
      list->hash[hashindex] = item;
   } else {
      /* Already an item on the hash chain */
      temp = list->hash[hashindex];
      while(temp->chain != NULL)
         temp = temp->chain;
      temp->chain = item;
   }

   /* Return the fully formed shashlist entry. */
   return(item);

}



shashlist_item *shashlist_remove_by_int(shashlist *list, int key) {
/* shashlist_remove_item_by_int
   Remove the shashlist data item referenced by this key. */

   int hashindex;
   shashlist_item *item, *temp;

   if(list == NULL || key < 0)
      return(NULL);

   /* Locate the item using hash lookup. */
   hashindex = shash(list->hashbits, key);
   item = list->hash[hashindex];
   if(item == NULL)
      return(NULL);

   /* Scan the collision chain as needed and remove the item from the hash. */
   if(item->key == key) {
      list->hash[hashindex] = item->chain;
   } else {
      temp = item;
      while(temp->chain != NULL && temp->chain->key != key)
         temp = temp->chain;
      if(temp->chain == NULL)
         return(NULL);
      item = temp->chain;
      temp->chain = temp->chain->chain;
   }

   /* Remove the item from the linked list. */
   if(item->prev == NULL) {
      list->head = item->next;
   } else {
      item->prev->next = item->next;
   }
   if(item->next == NULL) {
      list->tail = item->prev;
   } else {
      item->next->prev = item->prev;
   }

   /* Return the orphaned item. */
   return(item);

}



shashlist_item *shashlist_remove_by_string(shashlist *list, const char *key) {
/* shashlist_remove_item_by_string
   Remove the shashlist data item referenced by this key string. */

   int hashindex;
   shashlist_item *item, *temp;

   if(list == NULL || key == NULL)
      return(NULL);

   /* Locate the item using hash lookup. */
   hashindex = shash_string(list->hashbits, key);
   item = list->hash[hashindex];
   if(item == NULL)
      return(NULL);

   /* Scan the collision chain as needed and remove the item from the hash. */
   if(strcomp(item->keystr, key) == 0) {
      list->hash[hashindex] = item->chain;
   } else {
      temp = item;
      while(temp->chain != NULL && strcomp(temp->chain->keystr, key) != 0)
         temp = temp->chain;
      if(temp->chain == NULL)
         return(NULL);
      item = temp->chain;
      temp->chain = temp->chain->chain;
   }

   /* Remove the item from the linked list. */
   if(item->prev == NULL) {
      list->head = item->next;
   } else {
      item->prev->next = item->next;
   }
   if(item->next == NULL) {
      list->tail = item->prev;
   } else {
      item->next->prev = item->prev;
   }

   /* Return the orphaned item. */
   return(item);

}



shashlist_item *shashlist_find_by_int(const shashlist *list, int key) {
/* shashlist_find_by_int
   Find a shashlist item by integer key. */

   int hashindex;
   shashlist_item *item;

   if(list == NULL || key < 0)
      return(NULL);

   /* Locate the item using hash lookup. */
   hashindex = shash(list->hashbits, key);
   item = list->hash[hashindex];

   /* Scan the hash chain, as needed. */
   while(item != NULL && item->key != key)
      item = item->chain;

   /* item will be either the right item or NULL, at this point. */
   return(item);

}



shashlist_item *shashlist_find_by_string(const shashlist *list, const char *key) {
/* shashlist_find_by_string
   Find a shashlist item by string key. */

   int hashindex;
   shashlist_item *item;

   if(list == NULL || key == NULL)
      return(NULL);

   /* Locate the item using hash lookup. */
   hashindex = shash_string(list->hashbits, key);
   item = list->hash[hashindex];

   /* Scan the hash chain, as needed. */
   while(item != NULL && strcomp(item->keystr, key) != 0)
      item = item->chain;

   /* item will be either the right item or NULL, at this point. */
   return(item);

}



shashlist_item *shashlist_find_next(const shashlist *list, shashlist_item *item, int class) {
/* shashlist_find_next
   Find the next shashlist_item after item that is in the specified class.
   Passing item == NULL is a slang way of asking for the first item. */

   if(list == NULL)
      return(NULL);

   /* Get the first candidate. */
   if(item == NULL)
      item = list->head;
   else
      item = item->next;

   /* Find the first item meeting class requirements. */
   while(item != NULL && !(class == SHASHLIST_ALL_CLASSES || item->class == class))
      item = item->next;

   return(item);

}



shashlist_item *shashlist_find_prev(const shashlist *list, shashlist_item *item, int class) {
/* shashlist_find_prev
   Find the previous shashlist_item after item in the specified class.
   Passing item == NULL is a slang way of asking for the last item. */

   if(list == NULL)
      return(NULL);

   /* Get the first candidate. */
   if(item == NULL)
      item = list->tail;
   else
      item = item->prev;

   /* Find the first item meeting class requirements. */
   while(item != NULL && !(class == SHASHLIST_ALL_CLASSES || item->class == class))
      item = item->prev;

   return(item);

}
