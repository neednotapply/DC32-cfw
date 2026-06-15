/* $Header: /fridge/cvs/xscorch/sgame/seconomy.c,v 1.17 2011-08-01 00:01:41 jacob Exp $ */
/*
   
   xscorch - seconomy.c       Copyright(c) 2000      Justin David Smith
                              Copyright(c) 2003-2004 Jacob Luna Lundberg
   justins(at)chaos2.org      http://chaos2.org/
   jacob(at)gnifty.net        http://www.gnifty.net/

   Scorched economy
    

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

#include <seconomy.h>      /* A very economical header */
#include <saddconf.h>      /* Conf file parsing */
#include <sconfig.h>       /* Need to init player moneys */
#include <splayer.h>       /* More player cash init */
#include <sregistry.h>     /* Registry access */

#include <sutil/srand.h>   /* Random interest rate */

#include <libj/jstr/libjstr.h>



sc_economy_config *sc_economy_config_create(sc_config *c) {
/* sc_economy_config_create
   Allocate space and read in economy scoring definitions. */

   sc_economy_config *ec;
   const char *filename;

   ec = (sc_economy_config *)malloc(sizeof(sc_economy_config));
   if(ec == NULL) return(NULL);

   ec->registry        = c->registry;
   ec->interestrate    = SC_ECONOMY_DEF_INTEREST;
   ec->dynamicinterest = false;
   ec->initialcash     = SC_ECONOMY_DEF_CASH;
   ec->computersbuy    = true;
   ec->computersaggressive = false;
   ec->freemarket      = false;
   ec->lottery         = false;
   ec->scoringname[0]  = '\0';

   /* Get a class ID for this economy config. */
   ec->registryclass   = sc_registry_get_new_class_id(c->registry);
   ec->registry        = c->registry;

   /* Read in the scoring info list */
   filename = SC_GLOBAL_DIR "/" SC_SCORING_FILE;
   if(!sc_addconf_append_file(SC_ADDCONF_SCORINGS, filename, ec)) {
      /* This is the root scoring types list...  Die! */
      free(ec);
      return(NULL);
   }

   return(ec);

}



void sc_economy_config_destroy(sc_economy_config **ec) {
/* sc_economy_config_destroy
   Obliterate a poor innocent economy config that was doing nothing wrong. */

   sc_scoring_info *info, *temp;

   if(ec == NULL || *ec == NULL) return;

   /* Delete all of our registry entries. */
   info = (sc_scoring_info *)sc_registry_find_first((*ec)->registry, (*ec)->registryclass,
                                                    SC_REGISTRY_FORWARD, SC_REGISTRY_TEST_NULL, 0);
   while(info != NULL) {
      temp = info;
      info = (sc_scoring_info *)sc_registry_find_next((*ec)->registry, (*ec)->registryclass, info->ident,
                                                      SC_REGISTRY_FORWARD, SC_REGISTRY_TEST_NULL, 0);
      sc_registry_del_by_int((*ec)->registry, temp->ident);
      sc_scoring_info_free(&temp);
   }

   /* And delete ourself. */
   free(*ec);
   *ec = NULL;

}



void sc_scoring_info_free(sc_scoring_info **info) {
/* sc_scoring_info_free
   Invalidate memory used by an sc_scoring_info. */

   /* Make sure there is an item to free */
   if(info == NULL || *info == NULL) return;
   /* Free the item's name if it has one */
   if((*info)->name != NULL) free((*info)->name);
   /* Free the item */
   free(*info);
   *info = NULL;

}



void sc_economy_init(sc_economy_config *ec) {
/* sc_economy_init
   Initialize the economics for start of each game. */

   const sc_scoring_info *info;
   double ratio;

   assert(ec != NULL);

   /* If no scoring selected, use the default. */
   if(strnlenn(ec->scoringname, SC_ECONOMY_MAX_NAME_LEN)) {
      info = sc_scoring_lookup_by_name(ec, ec->scoringname);
   } else {
      info = (sc_scoring_info *)sc_registry_find_first(ec->registry, ec->registryclass,
                                                       SC_REGISTRY_FORWARD, SC_REGISTRY_TEST_NULL, 0);
      strcopyb(ec->scoringname, info->name, SC_ECONOMY_MAX_NAME_LEN);
   }

   /* We *must* have have a scoring method.
      Should be impossible to trigger though. */
   assert(info != NULL);

   /* Scale by initial cash (for fairness)? */
   if(info->fixed) {
      ec->damagebonus   = info->damagebonus;
      ec->damageloss    = info->damageloss;
      ec->deathloss     = info->deathloss;
      ec->killbonus     = info->killbonus;
      ec->suicideloss   = info->suicideloss;
      ec->survivalbonus = info->survivalbonus;
   } else {
      ratio = ec->initialcash / SC_ECONOMY_DEF_CASH;
      ec->damagebonus   = info->damagebonus   * ratio;
      ec->damageloss    = info->damageloss    * ratio;
      ec->deathloss     = info->deathloss     * ratio;
      ec->killbonus     = info->killbonus     * ratio;
      ec->suicideloss   = info->suicideloss   * ratio;
      ec->survivalbonus = info->survivalbonus * ratio;
   }

   /* Start at the default interest rate. */
   ec->currentinterest  = ec->interestrate;

}



void sc_economy_interest(sc_config *c, sc_economy_config *ec) {
/* sc_economy_interest
   Adjust the floating interest rate.
   This is run before each round. */

   int i;

   assert(c != NULL && ec != NULL);

   for(i = 0; i < c->numplayers; ++i) {
      c->players[i]->money *= (1 + ec->currentinterest);
   }
   if(ec->dynamicinterest) {
      ec->currentinterest += ((game_lrand(7) - 3) / 100.0);
      if(ec->currentinterest < 0) ec->currentinterest = 0;
      if(ec->currentinterest > SC_ECONOMY_MAX_INTEREST) ec->currentinterest = SC_ECONOMY_MAX_INTEREST;
   }

}



sc_scoring_info *sc_scoring_lookup(const sc_economy_config *ec, int id) {
/* sc_scoring_lookup
   Pass along a registry request for the economy scoring. */

   sc_scoring_info *info;

   if(ec == NULL || id < 0) return(NULL);

   /* Find the economy scoring in the registry. */
   info = (sc_scoring_info *)sc_registry_find_by_int(ec->registry, id);

   return(info);

}



bool _sc_scoring_test_lookup_name(void *data, long arg) {
/* _sc_scoring_test_lookup_name
   This is an sc_registry_test function.
   We will select from the registry by name. */

   sc_scoring_info *info = (sc_scoring_info *)data;
   const char *name      = (const char *)arg;

   /* We don't validate args; please do so in the caller! */

   /* Use sloppy string comparison on the name (true if similar). */
   return(strsimilar(info->name, name));

}


sc_scoring_info *sc_scoring_lookup_by_name(const sc_economy_config *ec, const char *name) {
/* sc_scoring_lookup_by_name
   Tries to find an economy scoring by roughly the requested name.
   This is much slower than sc_scoring_lookup. */

   sc_registry_iter *iter;
   sc_scoring_info *info;

   /* Don't make these into asserts without fixing
      sgtk/seconomy-gtk.c:_sc_economy_init_names_gtk() first. */
   if(ec == NULL || name == NULL) return(NULL);

   /* Set up for iteration. */
   iter = sc_registry_iter_new(ec->registry, ec->registryclass, SC_REGISTRY_FORWARD,
                               _sc_scoring_test_lookup_name, (long)(name));
   if(iter == NULL) return(NULL);

   /* Iterate using the fast registry iterators. */
   info = (sc_scoring_info *)sc_registry_iterate(iter);

   /* Clean up. */
   sc_registry_iter_free(&iter);

   return(info);

}



#ifdef __BUTTERFLIES_HAVE_STOLEN_YOUR_FAVORITE_CAR__
static const char *_sc_economy_scoring_names[] = {
   "Basic",
   "Standard",
   "Greedy",
   NULL
};
static const int _sc_economy_scoring_types[] = {
   SC_ECONOMY_SCORING_BASIC,
   SC_ECONOMY_SCORING_STANDARD,
   SC_ECONOMY_SCORING_GREEDY,
   0
};
const char **sc_economy_scoring_names(void) {

   return(_sc_economy_scoring_names);

}
const int *sc_economy_scoring_types(void) {

   return(_sc_economy_scoring_types);

}
#endif /* def DEPRECATED */
