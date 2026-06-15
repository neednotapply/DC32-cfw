/* $Header: /fridge/cvs/xscorch/sgame/sregistry.h,v 1.6 2011-08-01 00:01:41 jacob Exp $ */
/*

   xscorch - sregistry.h      Copyright(c) 2003-2004 Jacob Luna Lundberg
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
#ifndef __sregistry_h_included
#define __sregistry_h_included


/* Includes */
#include <xscorch.h>


/* Forward structure decl's */
struct _shashlist_item;
struct _shashlist;


/* This is a hash size.  Of course there are practical limits here.  :) */
#define  SC_REGISTRY_SIZE         0x1000


/* Footprint of registry iteration test functions. */
typedef bool(*sc_registry_test_func)(void *, long);
#define  SC_REGISTRY_TEST_NULL    (sc_registry_test_func)NULL


/* Direction of iteration through the registry. */
typedef enum _sc_registry_direction {
   SC_REGISTRY_FORWARD = 0,
   SC_REGISTRY_REVERSE
} sc_registry_direction;


/* The registry itself.  We'll probably only have one instance, heh. */
typedef struct _sc_registry {
   struct _shashlist *hashlist;
   int nextclass;
   int nextkey;
} sc_registry;


/*
 * This struct is used to iterate through registry sets.
 * A set is defined by selecting some items from the registry.
 * The basic level of selection is the class of the data item,
 * which is specified when it is entered into the registry.
 * However, the iterator functions also take a function in
 * their args.  The function, and one long int argument, will
 * be called on the data being tested for set membership.
 * If the function returns true, the data will be admitted.
 */
typedef struct _sc_registry_iter {
   long arg;
   int class;
   struct _shashlist_item *current;
   sc_registry_direction direction;
   sc_registry_test_func function;
   const sc_registry *registry;
   bool running;
} sc_registry_iter;


/* Create and destroy the registry. */
sc_registry *sc_registry_new(void);
void sc_registry_free(sc_registry **registry);


/* Identification values the registry coordinates. */
int sc_registry_get_new_class_id(sc_registry *registry);
int sc_registry_get_next_new_key(sc_registry *registry);


/* Add and remove registered items. */
bool sc_registry_add_by_int(sc_registry *registry, void *data, int class, int key);
bool sc_registry_add_by_string(sc_registry *registry, void *data, int class, const char *key);
void *sc_registry_del_by_int(sc_registry *registry, int key);
void *sc_registry_del_by_string(sc_registry *registry, const char *key);


/* Searching in the registry... */
void *sc_registry_find_by_int(const sc_registry *registry, int key);
void *sc_registry_find_by_string(const sc_registry *registry, const char *key);
void *sc_registry_find_first(const sc_registry *registry, int class, sc_registry_direction direction, sc_registry_test_func function, long arg);
void *sc_registry_find_next(const sc_registry *registry, int class, int key, sc_registry_direction direction, sc_registry_test_func function, long arg);


/* Fast iteration through the registry. */
sc_registry_iter *sc_registry_iter_new(const sc_registry *registry, int class, sc_registry_direction direction, sc_registry_test_func function, long arg);
void sc_registry_iter_free(sc_registry_iter **iter);
void *sc_registry_iterate(sc_registry_iter *iter);


#endif /* __sregistry_h_included */
