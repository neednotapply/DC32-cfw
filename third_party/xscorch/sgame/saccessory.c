/* $Header: /fridge/cvs/xscorch/sgame/saccessory.c,v 1.34 2011-08-01 00:01:40 jacob Exp $ */
/*

   xscorch - saccessory.c     Copyright(c) 2000-2004 Jacob Luna Lundberg
                              Copyright(c) 2000-2003 Justin David Smith
   jacob(at)gnifty.net        http://www.gnifty.net/
   justins(at)chaos2.org      http://chaos2.org/

   Scorched tank accessories


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
#include <assert.h>

#include <saccessory.h>    /* Get accessory data */
#include <saddconf.h>      /* Adding accessories */
#include <sconfig.h>       /* Config constants */
#include <sinventory.h>    /* Need inventory constants */
#include <sregistry.h>     /* Access to the registry */

#include <libj/jstr/libjstr.h>



/***  Accessory Data    ***/



typedef struct _sc_accessory_test_arg {
   int counter;
   int flags;
   const char *name;
   const sc_accessory_config *ac;
} sc_accessory_test_arg;



static inline bool _sc_accessory_viewable(const sc_accessory_config *ac, const sc_accessory_info *info, int flags) {
/* _sc_accessory_viewable
   We return true if the accessory is viewable under the given rules. */

   if( (!(flags & SC_ACCESSORY_LIMIT_ARMS)     || info->armslevel <= ac->armslevel)   &&
       (!(flags & SC_ACCESSORY_LIMIT_USELESS)  || ac->uselessitems || !info->useless) &&
       (!(flags & SC_ACCESSORY_LIMIT_INDIRECT) || !info->indirect)                    ) {
      return(true);
   } else {
      return(false);
   }

}



static bool _sc_accessory_test_count(void *data, long arg) {
/* _sc_accessory_test_count
   This is an sc_registry_test function.
   We will iterate, incrementing a counter, but always returning false. */

   sc_accessory_info *info    = (sc_accessory_info *)data;
   sc_accessory_test_arg *ata = (sc_accessory_test_arg *)arg;

   /* We don't validate args; please do so in the caller! */

   const sc_accessory_config *ac = ata->ac;
   int flags = ata->flags;

   if(_sc_accessory_viewable(ac, info, flags)) ++ata->counter;
   return(false);

}


int sc_accessory_count(const sc_accessory_config *ac, int flags) {
/* sc_accessory_count
   Counts the number of accessories that have been registered with the game's data registry.
   This is optimized but of course if you're calling it a lot you should cache the data.
   The only exception is that the count may change if a new accessory list file is appended. */

   sc_registry_iter *iter;
   sc_accessory_test_arg ata;

   if(ac == NULL) return(0);

   /* Set up for iteration. */
   ata.counter = 0;
   ata.flags   = flags;
   ata.ac      = ac;
   iter = sc_registry_iter_new(ac->registry, ac->registryclass, SC_REGISTRY_FORWARD,
                               _sc_accessory_test_count, (long)(&ata));
   if(iter == NULL) return(0);

   /* Iterate using the fast registry iterators. */
   sc_registry_iterate(iter);

   /* Clean up. */
   sc_registry_iter_free(&iter);

   return(ata.counter);

}



sc_accessory_info *sc_accessory_lookup(const sc_accessory_config *ac, int id, int flags) {
/* sc_accessory_lookup
   Pass along a registry request for the accessory. */

   sc_accessory_info *info;

   if(ac == NULL || id < 0) return(NULL);

   /* Find the accessory in the registry. */
   info = (sc_accessory_info *)sc_registry_find_by_int(ac->registry, id);

   /* Verify that the rules in place allow viewing the accessory. */
   if(info != NULL && _sc_accessory_viewable(ac, info, flags))
      return(info);
   else
      return(NULL);

}



static bool _sc_accessory_test_lookup_name(void *data, long arg) {
/* _sc_accessory_test_lookup_name
   This is an sc_registry_test function.
   We will seek, search, discover the accessory by name. */

   sc_accessory_info *info    = (sc_accessory_info *)data;
   sc_accessory_test_arg *ata = (sc_accessory_test_arg *)arg;

   /* We don't validate args; please do so in the caller! */

   const char *name = ata->name;

   /* Use sloppy string comparison on the name (true if similar). */
   return(strsimilar(info->name, name));

}


sc_accessory_info *sc_accessory_lookup_by_name(const sc_accessory_config *ac, const char *name, int flags) {
/* sc_accessory_lookup_by_name
   Tries to find an accessory by roughly the requested name.
   This is much slower than sc_accessory_lookup. */

   sc_registry_iter *iter;
   sc_accessory_test_arg ata;
   sc_accessory_info *info;

   if(ac == NULL || name == NULL) return(NULL);

   /* Set up for iteration. */
   ata.name = name;
   iter = sc_registry_iter_new(ac->registry, ac->registryclass,
                               (flags & SC_ACCESSORY_SCAN_REVERSE) ? SC_REGISTRY_REVERSE : SC_REGISTRY_FORWARD,
                               _sc_accessory_test_lookup_name, (long)(&ata));
   if(iter == NULL) return(NULL);

   /* Iterate using the fast registry iterators. */
   info = (sc_accessory_info *)sc_registry_iterate(iter);

   /* Clean up. */
   sc_registry_iter_free(&iter);

   /* Verify that the rules in place allow viewing the accessory. */
   if(info != NULL && _sc_accessory_viewable(ac, info, flags))
      return(info);
   else
      return(NULL);

}



static bool _sc_accessory_test_viewable(void *data, long arg) {
/* _sc_accessory_test_viewable
   This is an sc_registry_test function.
   We will return true for the first viewable accessory. */

   sc_accessory_info *info    = (sc_accessory_info *)data;
   sc_accessory_test_arg *ata = (sc_accessory_test_arg *)arg;

   /* We don't validate args; please do so in the caller! */

   const sc_accessory_config *ac = ata->ac;
   int flags = ata->flags;

   return(_sc_accessory_viewable(ac, info, flags));

}


sc_accessory_info *sc_accessory_first(const sc_accessory_config *ac, int flags) {
/* sc_accessory_first
   Return the first/last viable accessory in the accessory config space. */

   sc_accessory_test_arg ata;
   sc_accessory_info *info;

   if(ac == NULL) return(NULL);

   ata.ac    = ac;
   ata.flags = flags;

   /* Try and find the first/last accessory that meets our criteria. */
   info = sc_registry_find_first(ac->registry, ac->registryclass,
                                 (flags & SC_ACCESSORY_SCAN_REVERSE) ? SC_REGISTRY_REVERSE : SC_REGISTRY_FORWARD,
                                 _sc_accessory_test_viewable, (long)(&ata));

   return(info);

}


sc_accessory_info *sc_accessory_next(const sc_accessory_config *ac, const sc_accessory_info *info, int flags) {
/* sc_accessory_next
   Advance to the next/prev accessory in the list (with wrapping).  */

   sc_accessory_test_arg ata;
   sc_accessory_info *nxtnfo;

   if(ac == NULL || info == NULL) return(NULL);

   /* Set up the flags and args for the mission. */
   ata.ac    = ac;
   ata.flags = flags;

   /* Try and find the first/last accessory that meets our criteria. */
   nxtnfo = sc_registry_find_next(ac->registry, ac->registryclass, info->ident,
                                  (flags & SC_ACCESSORY_SCAN_REVERSE) ? SC_REGISTRY_REVERSE : SC_REGISTRY_FORWARD,
                                  _sc_accessory_test_viewable, (long)(&ata));

   /* In case we iterated off the end of the list, wrap to the beginning. */
   if(nxtnfo == NULL)
      nxtnfo = sc_accessory_first(ac, flags);

   return(nxtnfo);

}



void sc_accessory_info_line(const sc_accessory_config *ac, const sc_accessory_info *info, char *buffer, int buflen) {
/* sc_accessory_info_line
   Create a line of information about the accessory. */

   int moving;

   assert(ac != NULL);
   assert(buflen >= 0);

   if(buffer == NULL || buflen == 0)
      return;

   if(info == NULL) {
      buffer[0] = '\0';
      return;
   }

   /* Clear the buffer. */
   memset(buffer, 0, buflen * sizeof(char));
   /* Terminating NULL character. */
   --buflen;

   /* Add the name to the buffer. */
   sbprintf(buffer, buflen, "%s:", info->name);
   moving = strblenn(info->name, SC_INVENTORY_MAX_NAME_LEN) + 1;
   buffer += moving;
   buflen -= moving;

   /* Add spaces out to the end of the name area. */
   while(++moving < 20 && --buflen > 0)
      *(buffer++) = ' ';

   /* Display the accessory's arms level. */
   sbprintf(buffer, buflen, "arms = %1i, ", info->armslevel);
   moving = 10;
   buffer += moving;
   buflen -= moving;

   /* We display much different stuff for shields than for other accessories. */
   if(SC_ACCESSORY_IS_SHIELD(info)) {
      /* Display the shield level. */
      moving = info->shield;
      sbprintf(buffer, buflen, "shield = %-5i", moving);
      moving = 9 + (moving ? (int)log10(moving) : 1);
      buffer += moving;
      buflen -= moving;

      /* Add the comma. */
      if(buflen-- > 0)
         *(buffer++) = ',';

      /* Add spaces out to the end of the yield area, plus two. */
      while(++moving < 16 && --buflen > 0)
         *(buffer++) = ' ';

      /* Write out the shield short name. */
      sbprintf(buffer, buflen, "%3i%1c ", info->shield / 100, SC_ACCESSORY_SHIELD_CHAR(info));
      moving = 5;
      buffer += moving;
      buflen -= moving;

      /* And the shield type, of course. */
      sbprintf(buffer, buflen, "%s",
               SC_ACCESSORY_SHIELD_IS_FORCE(info) ? "force" :
               (SC_ACCESSORY_SHIELD_IS_MAGNETIC(info) ? "magnetic" :
                (SC_ACCESSORY_SHIELD_IS_STANDARD(info) ? "standard" : "unknown")));
      moving = 8;
      buffer += moving;
      buflen -= moving;

      /* If magnetic, also give repulsion. */
      if(SC_ACCESSORY_SHIELD_IS_MAGNETIC(info))
         sbprintf(buffer, buflen, " (repulsion = %i)", info->repulsion);
   } else {
      /* Write out some accessory info flags. */
      sbprintf(buffer, buflen, " %10s %8s %9s %7s",
               SC_ACCESSORY_IS_CONSUMABLE(info) ? "consumable" : "",
               SC_ACCESSORY_IS_INFINITE(info) ? "infinite" : "",
               SC_ACCESSORY_IS_PERMANENT(info) ? "permanent" : "",
               info->useless ? "useless" : "");
   }

}



sc_accessory_config *sc_accessory_config_create(const sc_config *c) {
/* sc_accessory_config_create
   Allocate space and set defaults on a new sc_accessory_config struct. */

   sc_accessory_config *ac;
   const char *filename;

   assert(c != NULL && c->registry != NULL);

   ac = (sc_accessory_config *)malloc(sizeof(sc_accessory_config));
   if(ac == NULL) return(NULL);

   /* Default settings for weapon config. */
   ac->armslevel = SC_ARMS_LEVEL_DEF;
   ac->uselessitems = true;

   /* Get a class ID for this accessory config. */
   ac->registryclass = sc_registry_get_new_class_id(c->registry);
   ac->registry      = c->registry;

   /* Read in the accessory info list */
   filename = SC_GLOBAL_DIR "/" SC_ACCESSORY_FILE;
   if(!sc_addconf_append_file(SC_ADDCONF_ACCESSORIES, filename, ac) ||
      sc_accessory_count(ac, SC_ACCESSORY_LIMIT_INDIRECT) <= 0) {
      /* This is the root accessory list...  Die! */
      free(ac);
      return(NULL);
   }

   return(ac);

}



void sc_accessory_config_destroy(sc_accessory_config **ac) {
/* sc_accessory_config_destroy
   Invalidate memory used by an accessory config struct. */

   sc_accessory_info *info, *temp;

   if(ac == NULL || *ac == NULL) return;

   /* Delete all of our registry entries. */
   info = (sc_accessory_info *)sc_registry_find_first((*ac)->registry, (*ac)->registryclass,
                                                      SC_REGISTRY_FORWARD, SC_REGISTRY_TEST_NULL, 0);
   while(info != NULL) {
      temp = info;
      info = (sc_accessory_info *)sc_registry_find_next((*ac)->registry, (*ac)->registryclass, info->ident,
                                                        SC_REGISTRY_FORWARD, SC_REGISTRY_TEST_NULL, 0);
      sc_registry_del_by_int((*ac)->registry, temp->ident);
      sc_accessory_info_free(&temp);
   }

   /* And delete ourself. */
   free(*ac);
   *ac = NULL;

}



void sc_accessory_info_free(sc_accessory_info **ai) {
/* sc_accessory_info_free
   Invalidate memory used by an sc_accessory_info. */

   /* Make sure there is an item to free */
   if(ai == NULL || *ai == NULL) return;
   /* Free the item's name if it has one */
   if((*ai)->name != NULL) free((*ai)->name);
   /* Free the item */
   free(*ai);
   *ai = NULL;

}



static bool _sc_accessory_inventory_clear_func(void *data, __libj_unused long arg) {
/* _sc_accessory_inventory_clear_func
   Clear accessory inventory data, in registry fast iterators. */

   sc_accessory_info *info;

   if(data == NULL) return(false);
   info = (sc_accessory_info *)data;

   /* Clear the accessory's inventory. */
   if(!SC_ACCESSORY_IS_INFINITE(info))
      memset(info->inventories, 0, SC_MAX_PLAYERS * sizeof(int));

   /* We never "find the right accessory" ... it's faster this way. */
   return(false);

}


void sc_accessory_inventory_clear(sc_accessory_config *ac) {
/* sc_accessory_inventory_clear
   Clear out the player accessory inventories. */

   sc_registry_iter *iter;

   if(ac == NULL) return;

   /* Prepare a registry iterator. */
   iter = sc_registry_iter_new(ac->registry, ac->registryclass, SC_REGISTRY_FORWARD,
                               _sc_accessory_inventory_clear_func, 0);

   /* Iterate the entire accessories registry,
      with the side effect of erasing inventories. */
   sc_registry_iterate(iter);

   /* Clean up. */
   sc_registry_iter_free(&iter);

}



/* Summary of the accessory state bits, for use in saddconf.c */
static const char *_sc_accessory_state_bit_names[] = {
   "force",
   "magnetic",
   "standard",
   "auto_defense",
   "battery",
   "consumable",
   "contact_trigger",
   "fuel",
   "null",
   "permanent",
   "recharge",
   "shield",
   "triple_turret",
   NULL
};
static const unsigned int _sc_accessory_state_bit_items[] = {
   SC_ACCESSORY_SHIELD_FORCE,
   SC_ACCESSORY_SHIELD_MAGNETIC,
   SC_ACCESSORY_SHIELD_STANDARD,
   SC_ACCESSORY_STATE_AUTO_DEF,
   SC_ACCESSORY_STATE_BATTERY,
   SC_ACCESSORY_STATE_CONSUMABLE,
   SC_ACCESSORY_STATE_TRIGGER,
   SC_ACCESSORY_STATE_FUEL,
   SC_ACCESSORY_STATE_NULL,
   SC_ACCESSORY_STATE_PERMANENT,
   SC_ACCESSORY_STATE_RECHARGE,
   SC_ACCESSORY_STATE_SHIELD,
   SC_ACCESSORY_STATE_TRIPLE,
   0
};



const char **sc_accessory_state_bit_names(void) {

   return(_sc_accessory_state_bit_names);

}



const unsigned int *sc_accessory_state_bit_items(void) {

   return(_sc_accessory_state_bit_items);

}
