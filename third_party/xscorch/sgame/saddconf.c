/* $Header: /fridge/cvs/xscorch/sgame/saddconf.c,v 1.49 2011-08-01 00:01:40 jacob Exp $ */
/*

   xscorch - saddconf.c       Copyright(c) 2001-2003 Jacob Luna Lundberg
   jacob(at)gnifty.net        http://www.gnifty.net/

   Scorched read/append config files


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

#include <saddconf.h>
#include <saccessory.h>
#include <seconomy.h>
#include <sinventory.h>
#include <sphoenix.h>
#include <sregistry.h>
#include <sweapon.h>

#include <libj/jreg/libjreg.h>
#include <libj/jstr/libjstr.h>



static const reg_class_default_data _reg_accessories_class[] = {
   REG_CLASS_DEFAULT_STRING  ("accessoryName", ""    ),
   REG_CLASS_DEFAULT_STRING  ("description",   ""    ),
   REG_CLASS_DEFAULT_INTEGER ("armsLevel",     0     ),
   REG_CLASS_DEFAULT_INTEGER ("price",         1     ),
   REG_CLASS_DEFAULT_INTEGER ("bundle",        1     ),
   REG_CLASS_DEFAULT_INTEGER ("shielding",     0     ),
   REG_CLASS_DEFAULT_INTEGER ("repulsion",     0     ),
   REG_CLASS_DEFAULT_INTEGER ("fuel",          0     ),
   REG_CLASS_DEFAULT_BOOLEAN ("useless",       false ),
   REG_CLASS_DEFAULT_BOOLEAN ("indirect",      false ),
   REG_CLASS_DEFAULT_BITFIELD("stateFlags",    ""    ),
   REG_CLASS_DEFAULT_END
};

static const reg_class_default_list _reg_class_accessories_parent[] = {
   REG_CLASS_DEFAULT_LIST("accessories_class", _reg_accessories_class),
   REG_CLASS_DEFAULT_LIST_END
};



bool _sc_accessory_read_item(sc_accessory_config *ac, reg *reader, reg_var *item) {
/* _sc_accessory_read_item
   Read an accessory info and insert it into the registry. */

   int iterator;
   sc_accessory_info *info;
   char desc[SC_INVENTORY_MAX_DESC_LEN];
   char name[SC_INVENTORY_MAX_NAME_LEN];

   assert(ac != NULL);
   assert(reader != NULL);
   assert(item != NULL);

   /* Make sure we're loading an accessory definition here. */
   if(strcomp(reg_get_var_class(item), "accessories_class")) goto out;

   /* Set defaults for this item. */
   if(!reg_set_var_class_defaults(reader, item, NULL, _reg_accessories_class)) goto out;

   /* Allocate the new info struct. */
   info = (sc_accessory_info *)malloc(sizeof(sc_accessory_info));
   if(info == NULL) goto out;

   /* Find out what it'll be named... */
   if(!reg_get_string(reader, item, "accessoryName", name, SC_INVENTORY_MAX_NAME_LEN) || name[0] == '\0')
      goto out_info;

   /* Check the name for duplication */
   if(sc_accessory_lookup_by_name(ac, name, SC_ACCESSORY_LIMIT_NONE))
      goto out_info;

   /* Allocate space for the accessory's name. */
   info->name = (char *)malloc(strlenn(name) + 1);
   if(info->name == NULL) goto out_info;

   /* Copy the name from the temp buffer. */
   strcopyn(info->name, name, strlenn(name));

   /* Ask the registry for a unique ID for this accessory. */
   info->ident = sc_registry_get_next_new_key(ac->registry);
   if(info->ident < 0) goto out_name;

   /* Set in the other fields in the accessory_info struct */
   reg_get_integer(reader, item, "armsLevel", &info->armslevel);
   reg_get_integer(reader, item, "price",     &info->price);
   reg_get_integer(reader, item, "bundle",    &info->bundle);
   reg_get_integer(reader, item, "shielding", &info->shield);
   reg_get_integer(reader, item, "repulsion", &info->repulsion);
   reg_get_integer(reader, item, "fuel",      &info->fuel);

   reg_get_boolean(reader, item, "useless",   &info->useless);
   reg_get_boolean(reader, item, "indirect",  &info->indirect);

   info->state = reg_get_bitmask_from_values(reader, item, "stateFlags", 0,
                                             sc_accessory_state_bit_names(),
                                             sc_accessory_state_bit_items());

   /* Read in the accessory description if there is one. */
   if(!reg_get_string(reader, item, "description", desc, SC_INVENTORY_MAX_DESC_LEN) || desc[0] == '\0') {
      info->description = NULL;
   } else {
      info->description = (char *)malloc(strlenn(desc) + 1);
      /* If the description allocation fails, let it slide... */
      if(info->description != NULL)
         strcopyn(info->description, desc, strlenn(desc));
   }

   /* Clean up minor details such as player inventories. */
   for(iterator = 0; iterator < SC_MAX_PLAYERS; ++iterator) {
      if(SC_ACCESSORY_IS_INFINITE(info))
         info->inventories[iterator] = SC_INVENTORY_MAX_ITEMS;
      else
         info->inventories[iterator] = 0;
   }

   /* Register the accessory with the data registry. */
   if(!sc_registry_add_by_int(ac->registry, info, ac->registryclass, info->ident))
      goto out_desc;

   /* Successful exit, return true! */
   return(true);

   /* Error paths... */
   out_desc:
      free(info->description);
   out_name:
      free(info->name);
   out_info:
      free(info);
   out:
      return(false);

}



static const reg_class_default_data _reg_scorings_class[] = {
   REG_CLASS_DEFAULT_STRING  ("scoringName",   ""    ),
   REG_CLASS_DEFAULT_STRING  ("description",   ""    ),
   REG_CLASS_DEFAULT_INTEGER ("survivalBonus", 100000),
   REG_CLASS_DEFAULT_INTEGER ("damageBonus",   0     ),
   REG_CLASS_DEFAULT_INTEGER ("killBonus",     40000 ),
   REG_CLASS_DEFAULT_INTEGER ("damageLoss",    0     ),
   REG_CLASS_DEFAULT_INTEGER ("deathLoss",     20000 ),
   REG_CLASS_DEFAULT_INTEGER ("suicideLoss",   100000),
   REG_CLASS_DEFAULT_BOOLEAN ("fixed",         false ),
   REG_CLASS_DEFAULT_END
};

static const reg_class_default_list _reg_class_scorings_parent[] = {
   REG_CLASS_DEFAULT_LIST("scorings_class", _reg_scorings_class),
   REG_CLASS_DEFAULT_LIST_END
};


bool _sc_scoring_read_item(sc_economy_config *ec, reg *reader, reg_var *item) {
/* _sc_scoring_read_item
   Read an economy scoring info and insert it into the registry. */

   sc_scoring_info *info;
   char desc[SC_ECONOMY_MAX_DESC_LEN];
   char name[SC_ECONOMY_MAX_NAME_LEN];

   assert(ec != NULL);
   assert(reader != NULL);
   assert(item != NULL);

   /* Make sure we're loading an scoring definition here. */
   if(strcomp(reg_get_var_class(item), "scorings_class")) goto out;

   /* Set defaults for this item. */
   if(!reg_set_var_class_defaults(reader, item, NULL, _reg_scorings_class)) goto out;

   /* Allocate the new info struct. */
   info = (sc_scoring_info *)malloc(sizeof(sc_scoring_info));
   if(info == NULL) goto out;

   /* Find out what it'll be named... */
   if(!reg_get_string(reader, item, "scoringName", name, SC_ECONOMY_MAX_NAME_LEN) || name[0] == '\0')
      goto out_info;

   /* Check the name for duplication */
   if(sc_scoring_lookup_by_name(ec, name))
      goto out_info;

   /* Allocate space for the scoring's name. */
   info->name = (char *)malloc(strlenn(name) + 1);
   if(info->name == NULL) goto out_info;

   /* Copy the name from the temp buffer. */
   strcopyn(info->name, name, strlenn(name));

   /* Ask the registry for a unique ID for this economy scoring. */
   info->ident = sc_registry_get_next_new_key(ec->registry);
   if(info->ident < 0) goto out_name;

   /* Set in the other fields in the scoring_info struct. */
   reg_get_integer(reader, item, "survivalBonus", &info->survivalbonus);
   reg_get_integer(reader, item, "damageBonus",   &info->damagebonus);
   reg_get_integer(reader, item, "killBonus",     &info->killbonus);
   reg_get_integer(reader, item, "damageLoss",    &info->damageloss);
   reg_get_integer(reader, item, "deathLoss",     &info->deathloss);
   reg_get_integer(reader, item, "suicideLoss",   &info->suicideloss);
   reg_get_boolean(reader, item, "fixed",         &info->fixed);

   /* Read in the scoring description if there is one. */
   if(!reg_get_string(reader, item, "description", desc, SC_ECONOMY_MAX_DESC_LEN) || desc[0] == '\0') {
      info->description = NULL;
   } else {
      info->description = (char *)malloc(strlenn(desc) + 1);
      /* If the description allocation fails, let it slide... */
      if(info->description != NULL)
         strcopyn(info->description, desc, strlenn(desc));
   }

   /* Register the scoring with the data registry. */
   if(!sc_registry_add_by_int(ec->registry, info, ec->registryclass, info->ident))
      goto out_desc;

   /* Successful exit, return true! */
   return(true);

   /* Error paths... */
   out_desc:
      free(info->description);
   out_name:
      free(info->name);
   out_info:
      free(info);
   out:
      return(false);

}



static const reg_class_default_data _reg_weapons_class[] = {
   REG_CLASS_DEFAULT_STRING  ("weaponName",    ""   ),
   REG_CLASS_DEFAULT_STRING  ("description",   ""   ),
   REG_CLASS_DEFAULT_INTEGER ("armsLevel",     0    ),
   REG_CLASS_DEFAULT_INTEGER ("price",         1    ),
   REG_CLASS_DEFAULT_INTEGER ("bundle",        1    ),
   REG_CLASS_DEFAULT_INTEGER ("radius",        0    ),
   REG_CLASS_DEFAULT_INTEGER ("force",         0    ),
   REG_CLASS_DEFAULT_INTEGER ("liquid",        0    ),
   REG_CLASS_DEFAULT_INTEGER ("scatter",       0    ),
   REG_CLASS_DEFAULT_INTEGER ("children",      0    ),
   REG_CLASS_DEFAULT_DOUBLEV ("angularWidth",  0.0  ),
   REG_CLASS_DEFAULT_BOOLEAN ("useless",       false),
   REG_CLASS_DEFAULT_BOOLEAN ("indirect",      false),
   REG_CLASS_DEFAULT_BITFIELD("stateFlags",    ""   ),
   REG_CLASS_DEFAULT_BITFIELD("phoenixFlags",  ""   ),
   REG_CLASS_DEFAULT_STRING  ("phoenixChild",  ""   ),
   REG_CLASS_DEFAULT_END
};

static const reg_class_default_list _reg_class_weapons_parent[] = {
   REG_CLASS_DEFAULT_LIST("weapons_class", _reg_weapons_class),
   REG_CLASS_DEFAULT_LIST_END
};


bool _sc_weapon_read_item(sc_weapon_config *wc, reg *reader, reg_var *item) {
/* _sc_weapon_read_item
   Read a weapon info and insert it into the registry. */

   int iterator;
   sc_weapon_info *info;
   sc_weapon_info *child;
   char childname[SC_INVENTORY_MAX_NAME_LEN];
   char desc[SC_INVENTORY_MAX_DESC_LEN];
   char name[SC_INVENTORY_MAX_NAME_LEN];

   assert(wc != NULL);
   assert(reader != NULL);
   assert(item != NULL);

   /* Make sure we're loading a weapon definition here. */
   if(strcomp(reg_get_var_class(item), "weapons_class")) goto out;

   /* Set defaults for this item. */
   if(!reg_set_var_class_defaults(reader, item, NULL, _reg_weapons_class)) goto out;

   /* Allocate the new info struct. */
   info = (sc_weapon_info *)malloc(sizeof(sc_weapon_info));
   if(info == NULL) goto out;

   /* Find out what it'll be named... */
   if(!reg_get_string(reader, item, "weaponName", name, SC_INVENTORY_MAX_NAME_LEN) || name[0] == '\0')
      goto out_info;

   /* Check the name for duplication */
   if(sc_weapon_lookup_by_name(wc, name, SC_WEAPON_LIMIT_NONE))
      goto out_info;

   /* Allocate space for the weapon's name. */
   info->name = (char *)malloc(strlenn(name) + 1);
   if(info->name == NULL) goto out_info;

   /* Copy the name from the temp buffer. */
   strcopyn(info->name, name, strlenn(name));

   /* Ask the registry for a unique ID for this weapon. */
   info->ident = sc_registry_get_next_new_key(wc->registry);
   if(info->ident < 0) goto out_name;

   /* Set in the other fields in the weapon_info struct. */
   reg_get_integer(reader, item, "armsLevel", &info->armslevel);
   reg_get_integer(reader, item, "price",     &info->price);
   reg_get_integer(reader, item, "bundle",    &info->bundle);
   reg_get_integer(reader, item, "radius",    &info->radius);
   reg_get_integer(reader, item, "force",     &info->force);
   reg_get_integer(reader, item, "liquid",    &info->liquid);
   reg_get_integer(reader, item, "scatter",   &info->scatter);
   reg_get_integer(reader, item, "children",  &info->children);

   /* Convert the angle for the explosion from degrees to radians. */
   reg_get_doublev(reader, item, "angularWidth", &info->angular_width);
   info->angular_width *= M_PI / 180.0;

   reg_get_boolean(reader, item, "useless",   &info->useless);
   reg_get_boolean(reader, item, "indirect",  &info->indirect);

   info->state = reg_get_bitmask_from_values(reader, item, "stateFlags",   0,
                                             sc_weapon_state_bit_names(),
                                             sc_weapon_state_bit_items());
   info->ph_fl = reg_get_bitmask_from_values(reader, item, "phoenixFlags", 0,
                                             sc_phoenix_flags_bit_names(),
                                             sc_phoenix_flags_bit_items());

   /* Read in the weapon description if there is one. */
   if(!reg_get_string(reader, item, "description", desc, SC_INVENTORY_MAX_DESC_LEN) || desc[0] == '\0') {
      info->description = NULL;
   } else {
      info->description = (char *)malloc(strlenn(desc) + 1);
      /* If the description allocation fails, let it slide... */
      if(info->description != NULL)
         strcopyn(info->description, desc, strlenn(desc));
   }

   /* Set the child if there is one. */
   if(reg_get_string(reader, item, "phoenixChild", childname, SC_INVENTORY_MAX_NAME_LEN) && childname[0] != '\0') {
      /* Try and find the child. */
      if(strsimilar(info->name, childname))
         child = info;
      else
         child = sc_weapon_lookup_by_name(wc, childname, SC_WEAPON_LIMIT_NONE);

      /* If we found the child, set it.  Otherwise, warn. */
      if(child == NULL)
         printf("saddconf - warning, \"%s\" has missing child \"%s\"\n", name, childname);
      else
         info->ph_ch = child->ident;
   }

   /* Test for weapon acceptance... */
   if(SC_WEAPON_IS_PHOENIX(info) && !sc_phoenix_verify(wc, info)) {
      printf("saddconf - \"%s\" is an invalid phoenix weapon, rejecting it\n", name);
      goto out_desc;
   }

   /* Clean up minor details such as player inventories. */
   for(iterator = 0; iterator < SC_MAX_PLAYERS; ++iterator) {
      if(SC_WEAPON_IS_INFINITE(info))
         info->inventories[iterator] = SC_INVENTORY_MAX_ITEMS;
      else
         info->inventories[iterator] = 0;
   }

   /* Register the weapon with the data registry. */
   if(!sc_registry_add_by_int(wc->registry, info, wc->registryclass, info->ident))
      goto out_desc;

   /* Successful exit, return true! */
   return(true);

   /* Error paths... */
   out_desc:
      free(info->description);
   out_name:
      free(info->name);
   out_info:
      free(info);
   out:
      return(false);

}



bool sc_addconf_append_file(sc_addconf_type type, const char *filename, void *container) {
/* sc_addconf_append_file
   Append a conf file to the data registry.  Returns true on success. */

   int count = 0;
   reg_var *item;
   reg_var *top;
   reg *reader;

   if(filename == NULL || container == NULL) return(false);

   /* Read the file into the file registry. */
   reader = reg_new(filename);
   if(reader == NULL) return(false);

   /* Inform the file regsitry how to parse the file. */
   switch(type) {
      case SC_ADDCONF_ACCESSORIES:
         reg_class_register_default_list(reader, _reg_class_accessories_parent);
         break;
      case SC_ADDCONF_SCORINGS:
         reg_class_register_default_list(reader, _reg_class_scorings_parent);
         break;
      case SC_ADDCONF_WEAPONS:
         reg_class_register_default_list(reader, _reg_class_weapons_parent);
         break;
   }

   /* Try to parse the conf file using the file registry. */
   if(!reg_load(reader)) {
      /* Failed to read file; signal error */
      printf("saddconf - aborting, can't load \"%s\"\n", filename);
      reg_free(&reader);
      return(false);
   }

   /* To simplify the switch below, we break this out with the preprocessor. */
   #define  iterate_calling(function)  while(item != NULL) { \
                                          if(function(container, reader, item)) ++count; \
                                          item = reg_get_next_var(item); \
                                       }

   /* Read the variables in order. */
   top = reg_get_top(reader);
   item = reg_get_block_head(top);
   switch(type) {
      case SC_ADDCONF_ACCESSORIES:
         iterate_calling(_sc_accessory_read_item);
         break;
      case SC_ADDCONF_SCORINGS:
         iterate_calling(_sc_scoring_read_item);
         break;
      case SC_ADDCONF_WEAPONS:
         iterate_calling(_sc_weapon_read_item);
         break;
   }

   /* Symmetry is Beautiful. */
   #undef  iterate_calling

   /* Release the registry. */
   reg_free(&reader);

   /* Let the next layer up know whether we found some configuration items. */
   if(count)
      return(true);
   else
      return(false);

}
