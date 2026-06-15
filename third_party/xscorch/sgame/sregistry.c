/* $Header: /fridge/cvs/xscorch/sgame/sregistry.c,v 1.8 2011-08-01 00:01:41 jacob Exp $ */
/*

   xscorch - sregistry.c      Copyright(c) 2003-2004 Jacob Luna Lundberg
   jacob(at)gnifty.net        http://www.gnifty.net/

   Scorched runtime fast data registry


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
#include <sregistry.h>

#include <sutil/shashlist.h>



sc_registry *sc_registry_new(void) {
/* sc_registry_new
   Allocate and initialize a new registry. */

   sc_registry *registry;

   /* Allocate the registry. */
   registry = (sc_registry *)malloc(sizeof(sc_registry));
   if(registry == NULL)
      return(NULL);

   /* Allocate the storage hashlist. */
   registry->hashlist = shashlist_new(SC_REGISTRY_SIZE);
   if(registry->hashlist == NULL) {
      free(registry);
      return(NULL);
   }

   /* Start numbering custom classes at the first available integer. */
   registry->nextclass = SHASHLIST_FIRST_CLASS;

   /* Start numbering valid registry keys at 0 (-1 means bad key). */
   registry->nextkey = 0;

   return(registry);

}


void sc_registry_free(sc_registry **registry) {
/* sc_registry_free
   Deallocate a registry (whew). */

   if(registry == NULL || *registry == NULL) return;

   /* This will also deallocate any registry entries, but not their data... */
   shashlist_free(&(*registry)->hashlist);

   free(*registry);
   *registry = NULL;

}



int sc_registry_get_new_class_id(sc_registry *registry) {
/* sc_registry_get_new_class_id
   Return the next available class ID. */

   if(registry == NULL) return(-1);
   return(registry->nextclass++);

}


int sc_registry_get_next_new_key(sc_registry *registry) {
/* sc_registry_get_next_new_key
   Return the next available unique integer key. */

   if(registry == NULL) return(-1);
   return(registry->nextkey++);

}



bool sc_registry_add_by_int(sc_registry *registry, void *data, int class, int key) {
/* sc_registry_add_by_int
   Add data to the registry by integer key. */

   if(registry == NULL) return(false);
   return(shashlist_insert_by_int(registry->hashlist, data, class, key) != NULL);

}


bool sc_registry_add_by_string(sc_registry *registry, void *data, int class, const char *key) {
/* sc_registry_add_by_string
   Add data to the registry by string key. */

   if(registry == NULL) return(false);
   return(shashlist_insert_by_string(registry->hashlist, data, class, key) != NULL);

}



void *sc_registry_del_by_int(sc_registry *registry, int key) {
/* sc_registry_del_by_int
   Delete data from the registry by integer key. */

   shashlist_item *item;
   void *data = NULL;

   if(registry == NULL) return(NULL);
   item = shashlist_remove_by_int(registry->hashlist, key);
   if(item != NULL) {
      data = item->data;
      shashlist_item_free(&item);
   }

   return(data);

}


void *sc_registry_del_by_string(sc_registry *registry, const char *key) {
/* sc_registry_del_by_string
   Delete data from the registry by string key. */

   shashlist_item *item;
   void *data = NULL;

   if(registry == NULL) return(NULL);
   item = shashlist_remove_by_string(registry->hashlist, key);
   if(item != NULL) {
      data = item->data;
      shashlist_item_free(&item);
   }

   return(data);

}



void *sc_registry_find_by_int(const sc_registry *registry, int key) {
/* sc_registry_find_by_int
   Locate a specific registry item by integer key. */

   shashlist_item *item;

   if(registry == NULL) return(NULL);
   item = shashlist_find_by_int(registry->hashlist, key);
   if(item == NULL)
      return(NULL);
   else
      return(item->data);

}


void *sc_registry_find_by_string(const sc_registry *registry, const char *key) {
/* sc_registry_find_by_string
   Locate a specific registry item by string key. */

   shashlist_item *item;

   if(registry == NULL) return(NULL);
   item = shashlist_find_by_string(registry->hashlist, key);
   if(item == NULL)
      return(NULL);
   else
      return(item->data);

}



inline shashlist_item *_sc_registry_internal_iter(shashlist *list, shashlist_item *item, int class, sc_registry_direction direction, sc_registry_test_func function, long arg) {
/* _sc_registry_internal_iter
   Local function to find the next or prev entry in a set. */

   switch(direction) {
      case SC_REGISTRY_FORWARD:
         do {
            item = shashlist_find_next(list, item, class);
         } while(item != NULL && function != NULL && !function(item->data, arg));
         break;
      case SC_REGISTRY_REVERSE:
         do {
            item = shashlist_find_prev(list, item, class);
         } while(item != NULL && function != NULL && !function(item->data, arg));
         break;
   }

   return(item);

}



void *sc_registry_find_first(const sc_registry *registry, int class, sc_registry_direction direction, sc_registry_test_func function, long arg) {
/* sc_registry_find_first
   Find the first item of a given class. */

   shashlist_item *item = NULL;

   if(registry == NULL) return(NULL);

   /* Search for the first item meeting all the criteria. */
   item = _sc_registry_internal_iter(registry->hashlist, item,
                                     class, direction, function, arg);

   if(item == NULL)
      return(NULL);
   else
      return(item->data);

}


void *sc_registry_find_next(const sc_registry *registry, int class, int key, sc_registry_direction direction, sc_registry_test_func function, long arg) {
/* sc_registry_find_next
   Find the next item of a given class, given an item. */

   shashlist_item *item;

   if(registry == NULL) return(NULL);
   item = shashlist_find_by_int(registry->hashlist, key);

   /* Search for the next item meeting all the criteria. */
   item = _sc_registry_internal_iter(registry->hashlist, item,
                                     class, direction, function, arg);

   if(item == NULL)
      return(NULL);
   else
      return(item->data);

}



sc_registry_iter *sc_registry_iter_new(const sc_registry *registry, int class, sc_registry_direction direction, sc_registry_test_func function, long arg) {
/* sc_registry_iter_new
   Allocate an sc_registry_iter struct for fast registry set iteration. */

   sc_registry_iter *iter;

   if(registry == NULL) return(NULL);

   iter = (sc_registry_iter *)malloc(sizeof(sc_registry_iter));
   if(iter == NULL) return(NULL);

   /* Prep for the iteration. */
   iter->arg = arg;
   iter->class = class;
   iter->current = NULL;
   iter->direction = direction;
   iter->function = function;
   iter->registry = registry;
   iter->running = true;

   return(iter);

}


void sc_registry_iter_free(sc_registry_iter **iter) {
/* sc_registry_iter_free
   Free a registry iteration struct. */

   if(iter == NULL || *iter == NULL) return;

   (*iter)->running = false;
   free(*iter);
   *iter = NULL;

}


void *sc_registry_iterate(sc_registry_iter *iter) {
/* sc_registry_iterate
   Find and return the next item in the set, if any. */

   if(iter == NULL) return(NULL);

   /* Make sure we're still iterating happily along. */
   if(!iter->running)
      return(NULL);

   /* Perform the search for the next iteration. */
   iter->current = _sc_registry_internal_iter(iter->registry->hashlist, iter->current,
                                              iter->class, iter->direction,
                                              iter->function, iter->arg);

   if(iter->current == NULL) {
      /* We've reached the end of the set. */
      iter->running = false;
      return(NULL);
   } else {
      return(iter->current->data);
   }

}
