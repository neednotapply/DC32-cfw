/* $Header: /fridge/cvs/xscorch/sgame/sinventory.h,v 1.22 2011-08-01 00:01:41 jacob Exp $ */
/*

   xscorch - sinventory.h     Copyright(c) 2000 Justin David Smith
                              Copyright(c) 2001 Jacob Luna Lundberg
   justins(at)chaos2.org      http://chaos2.org/
   jacob(at)gnifty.net        http://www.gnifty.net/

   Scorched player inventory


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
#ifndef __sinventory_h_included
#define __sinventory_h_included


#include <xscorch.h>
#include <sgame/sweapon.h>
#include <sgame/saccessory.h>


/* Forward structures */
struct _sc_player;
struct _sc_config;


/* Maximum number of items allowed */
#define  SC_INVENTORY_MAX_ITEMS     99
#define  SC_INVENTORY_INFINITE      -1


/* String buffering constants. */
#define  SC_INVENTORY_MAX_NAME_LEN  20
#define  SC_INVENTORY_MAX_DESC_LEN  80


/* Inventory hashing */
#define  SC_INVENTORY_HASH_BITS     10       /* Must be smaller than bits in int */
#define  SC_INVENTORY_HASH_SIZE     (1 << SC_INVENTORY_HASH_BITS)  /* duh ... */


/* Other inventory information */
#define  SC_INVENTORY_CHEAPO_FACTOR 1000     /* Free stuff must be worthless, right? */


/* TEMP  --  Can/should we get rid of this? -JL  -- Inventory information */
typedef struct _sc_inventory_info {
   /* DO NOT CHANGE the order */
   int ident;                   /* Item identifier (uniq. within item class) */
   int armslevel;               /* Arms level for this item */
   int price;                   /* Cost of this items, per bundle */
   int bundle;                  /* Number of items in a bundle */
} sc_inventory_info;


/* Inventory queries */
bool sc_inventory_can_buy_weapon(const struct _sc_player *p, const sc_weapon_info *info, int budget);
bool sc_inventory_can_buy_accessory(const struct _sc_player *p, const sc_accessory_info *info, int budget);
bool sc_inventory_can_sell_weapon(const struct _sc_player *p, const sc_weapon_info *info);
bool sc_inventory_can_sell_accessory(const struct _sc_player *p, const sc_accessory_info *info);


/* Inventory purchases */
bool sc_inventory_buy_weapon(struct _sc_player *p, sc_weapon_info *info);
bool sc_inventory_buy_accessory(struct _sc_player *p, sc_accessory_info *info);
bool sc_inventory_sell_weapon(struct _sc_player *p, sc_weapon_info *info);
bool sc_inventory_sell_accessory(struct _sc_player *p, sc_accessory_info *info);
bool sc_inventory_award_weapon(sc_weapon_info *info, int player);


#endif /* __sinventory_h_included */
