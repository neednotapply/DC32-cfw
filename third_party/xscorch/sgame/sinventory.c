/* $Header: /fridge/cvs/xscorch/sgame/sinventory.c,v 1.20 2011-08-01 00:01:41 jacob Exp $ */
/*

   xscorch - sinventory.c     Copyright(c) 2001,2000 Justin David Smith
                              Copyright(c) 2001      Jacob Luna Lundberg
   justins(at)chaos2.org      http://chaos2.org/
   jacob(at)gnifty.net        http://www.gnifty.net/

   Scorched Player inventory


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
#include <sinventory.h>
#include <saccessory.h>
#include <sconfig.h>
#include <seconomy.h>
#include <splayer.h>
#include <sweapon.h>
#include <swindow.h>



static int _sc_inventory_quantity(int current, int bundlesize) {
/* sc_inventory_quantity
   Determine the number of items the user can purchase of this item. */

   if(current + bundlesize <= SC_INVENTORY_MAX_ITEMS) {
      /* User can buy an entire bundle. */
      return(bundlesize);
   } else {
      /* User can only purchase part of the bundle. */
      return(SC_INVENTORY_MAX_ITEMS - current);
   } /* Did we break a bundle? */

}



static int _sc_inventory_purchase_price(const sc_inventory_info *info, int quantity) {
/* sc_inventory_purchase_price
   Price required to purchase this item.  */

   int unitprice;       /* Cost of a single item. */

   if(quantity < info->bundle) {
      /* Bundle was split up; implement a markup. */
      unitprice = info->price / info->bundle;
      return(unitprice * quantity * SC_ECONOMY_SPLIT_MARKUP);
   } else {
      /* Bundle not split; return full price. */
      return(info->price);
   }

}



static int _sc_inventory_sale_price(const sc_inventory_info *info, int quantity) {
/* sc_inventory_sale_price
   Price required to sell this item.  */

   int unitprice;       /* Cost of a single item. */

   if(quantity < info->bundle) {
      /* Bundle is partial. */
      unitprice = info->price / info->bundle;
      return(unitprice * quantity * SC_ECONOMY_SELL_MARKUP);
   } else {
      /* Bundle complete; still markdown a bit */
      return(info->price * SC_ECONOMY_SELL_MARKUP);
   }

}



bool sc_inventory_can_buy_weapon(const sc_player *p, const sc_weapon_info *info, int budget) {
/* sc_inventory_can_buy_weapon
   Determine if this player can buy the specified weapon.  Returns true
   if the purchase can be made.  Budget is the amount of money allocated for
   the purchase, or may be -1 (player's total resources will be allowed into
   the calculation). */

   int count;        /* Number of weapons to buy */
   int cost;         /* Total cost to buy */

   /* Does the weapon exist? */
   if(info == NULL) return(false);

   /* Are we maxed out on inventory? */
   count = _sc_inventory_quantity(info->inventories[p->index], info->bundle);
   if(count == 0) return(false);

   /* Can we afford the weapon? */
   cost = _sc_inventory_purchase_price((sc_inventory_info *)info, count);
   if(cost > p->money) return(false);

   /* Is the weapon within our budget? */
   if(budget >= 0 && cost > budget) return(false);

   /* We can make this purchase. */
   return(true);

}



bool sc_inventory_buy_weapon(sc_player *p, sc_weapon_info *info) {
/* sc_inventory_buy_weapon
   Buys the specified weapon.  Returns true if the purchase was successful. */

   int count;        /* Number of weapons to buy */
   int cost;         /* Total cost to buy */

   /* Can we buy this weapon? */
   if(!sc_inventory_can_buy_weapon(p, info, SC_INVENTORY_INFINITE)) return(false);

   /* Get weapon purchase information.. */
   count = _sc_inventory_quantity(info->inventories[p->index], info->bundle);
   cost = _sc_inventory_purchase_price((sc_inventory_info *)info, count);

   /* Make the purchase */
   info->inventories[p->index] += count;
   p->money -= cost;

   /* Return success. */
   return(true);

}



bool sc_inventory_can_buy_accessory(const sc_player *p, const sc_accessory_info *info, int budget) {
/* sc_inventory_can_buy_accessory
   Determine if this player can buy the specified accessory.  Returns true
   if the purchase can be made.  Budget is the amount of money allocated for
   the purchase, or may be -1 (player's total resources will be allowed into
   the calculation). */

   int count;        /* Number of accessories to buy */
   int cost;         /* Total cost to buy */

   /* Does the accessory exist? */
   if(info == NULL) return(false);

   /* Is the accessory a one-time buy we already own? */
   if(SC_ACCESSORY_IS_PERMANENT(info) && info->inventories[p->index] > 0) return(false);

   /* Are we maxed out on inventory? */
   count = _sc_inventory_quantity(info->inventories[p->index], info->bundle);
   if(count == 0) return(false);

   /* Can we afford the accessory? */
   cost = _sc_inventory_purchase_price((sc_inventory_info *)info, count);
   if(cost > p->money) return(false);

   /* Is the accessory in our budget? */
   if(budget >= 0 && cost > budget) return(false);

   /* We can make this purchase. */
   return(true);

}



bool sc_inventory_buy_accessory(sc_player *p, sc_accessory_info *info) {
/* sc_inventory_buy_accessory
   Buys the specified accessory.  Returns true if the purchase was successful. */

   int count;        /* Number of accessories to buy */
   int cost;         /* Total cost to buy */

   /* Can we buy this accessory? */
   if(!sc_inventory_can_buy_accessory(p, info, SC_INVENTORY_INFINITE)) return(false);

   /* Get accessory purchase information.. */
   count = _sc_inventory_quantity(info->inventories[p->index], info->bundle);
   cost = _sc_inventory_purchase_price((sc_inventory_info *)info, count);

   /* Make the purchase */
   info->inventories[p->index] += count;
   p->money -= cost;

   /* Add any attributes granted by the accessory. */
   p->ac_state |= info->state;

   /* Return success. */
   return(true);

}



bool sc_inventory_can_sell_weapon(const sc_player *p, const sc_weapon_info *info) {
/* sc_inventory_can_sell_weapon
   Determine if this player can sell the specified weapon.
   Returns true if the sale can be made. */

   /* Does the weapon exist? */
   if(info == NULL) return(false);

   /* Are we min'd out on inventory? */
   if(info->inventories[p->index] <= 0)
      return(false);

   /* Is the weapon infinite? */
   if(SC_WEAPON_IS_INFINITE(info))
      return(false);

   /* We can make this transaction. */
   return(true);

}



bool sc_inventory_sell_weapon(sc_player *p, sc_weapon_info *info) {
/* sc_inventory_sell_weapon
   Sells the specified weapon.  Returns true if the transaction was successful. */

   int count;        /* Number of weapons to buy */
   int cost;         /* Total cost to buy */

   /* Can we sell this weapon? */
   if(!sc_inventory_can_sell_weapon(p, info)) return(false);

   /* Get weapon sale information.. */
   count = info->inventories[p->index];
   if(count > info->bundle) count = info->bundle;
   cost = _sc_inventory_sale_price((sc_inventory_info *)info, count);

   /* Sell the item */
   info->inventories[p->index] -= count;
   p->money += cost;

   /* Return success. */
   return(true);

}



bool sc_inventory_can_sell_accessory(const sc_player *p, const sc_accessory_info *info) {
/* sc_inventory_can_sell_accessory
   Determine if this player can sell the specified accessory.
   Returns true if the sale can be made. */

   /* Does the accessory exist? */
   if(info == NULL) return(false);

   /* Are we min'd out on inventory? */
   if(info->inventories[p->index] <= 0)
      return(false);

   /* Is the accessory infinite? */
   if(SC_ACCESSORY_IS_INFINITE(info))
      return(false);

   /* We can make this transaction. */
   return(true);

}



bool sc_inventory_sell_accessory(sc_player *p, sc_accessory_info *info) {
/* sc_inventory_sell_accessory
   Sells the specified accessory.  Returns true if the transaction was successful. */

   int count;        /* Number of accessories to buy */
   int cost;         /* Total cost to buy */

   /* Can we sell this accessory? */
   if(!sc_inventory_can_sell_accessory(p, info)) return(false);

   /* Get accessory sale information.. */
   count = info->inventories[p->index];
   if(count > info->bundle) count = info->bundle;
   cost = _sc_inventory_sale_price((sc_inventory_info *)info, count);

   /* TEMP HACK - This strips benefit unconditionally, and if
      another accessory also grants it, too bad.  Fix it! - JL
      NOTE - Thinking on this more, what will likely need to
      be done is loop though the accessories and look for any
      others that grant any of the same bits and then re-
      grant those bits in the state int.  hmm - JL */
   /* Remove any attributes granted by the accessory. */
   p->ac_state &= ~info->state;

   /* Sell the item */
   info->inventories[p->index] -= count;
   p->money += cost;

   /* Return success. */
   return(true);

}



bool sc_inventory_award_weapon(sc_weapon_info *info, int player) {
/* sc_inventory_award_weapon
   Give a player what he can take of a weapon for free. */

   int quantity;

   if(info == NULL) return(false);

   /* Find out how many to award. */
   quantity = _sc_inventory_quantity(info->inventories[player], info->bundle);
   if(quantity <= 0) return(false);

   /* Make the award. */
   info->inventories[player] += quantity;
   return(true);

}
