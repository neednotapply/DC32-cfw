/* $Header: /fridge/cvs/xscorch/sgame/saccessory.h,v 1.27 2011-08-01 00:01:40 jacob Exp $ */
/*

   xscorch - saccessory.h     Copyright(c) 2000-2003 Justin David Smith
                              Copyright(c) 2000-2003 Jacob Luna Lundberg
   justins(at)chaos2.org      http://chaos2.org/
   jacob(at)gnifty.net        http://www.gnifty.net/

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
#ifndef __saccessory_h_included
#define __saccessory_h_included


/* Includes */
#include <xscorch.h>


/* Forward declarations */
struct _sc_registry;
struct _sc_config;


/* Data on the various types of accessories */
typedef struct _sc_accessory_info {
   /* DO NOT CHANGE the order of the first four items:
      should be ident, armslevel, price, bundle.  */
   int ident;                       /* Accessory identifier */
   int armslevel;                   /* Arms level for this accessory */
   int price;                       /* Price ($) */
   int bundle;                      /* Bundle size */
   int shield;                      /* Shield strength (0 == not a shield) */
   int fuel;                        /* Amount of fuel (0 == not a fuel tank) */
   int repulsion;                   /* Shield repulsion power */
   int state;                       /* Accessory state/type bits */
   int inventories[SC_MAX_PLAYERS]; /* Player accessory inventories */
   bool useless;                    /* Is this a useless item? */
   bool indirect;                   /* Accessed only internally? */
   char *name;                      /* Accessory name */
   char *description;               /* Accessory information */
} sc_accessory_info;


/* Accessory scanning functions need to know what to find and what not to. */
#define  SC_ACCESSORY_LIMIT_NONE        0x0000
#define  SC_ACCESSORY_LIMIT_ARMS        0x0001
#define  SC_ACCESSORY_LIMIT_USELESS     0x0002
#define  SC_ACCESSORY_LIMIT_INDIRECT    0x0004
#define  SC_ACCESSORY_LIMIT_ALL         0x00ff

/* Accessory scanning functions need to know in what direction to scan. */
#define  SC_ACCESSORY_SCAN_DEFAULT      0x0000
#define  SC_ACCESSORY_SCAN_FORWARD      0x0100
#define  SC_ACCESSORY_SCAN_REVERSE      0x0200


/*
   Bit names in the state flag.
   Changes to these bits must be reflected in the functions at
   the bottom of saccessory.c.  Also they may require some changes
   to saddconf.c, and perhaps to the accessories.def file as well.
   Note that there are still a couple bytes to spare.
*/
#define  SC_ACCESSORY_STATE_TYPES       0xff0000
#define  SC_ACCESSORY_STATE_SHIELDS     0x00ff00
#define  SC_ACCESSORY_STATE_FLAGS       0x0000ff

/* Bits in the accessory state FLAGS flag */
#define  SC_ACCESSORY_STATE_NULL        0x000001
#define  SC_ACCESSORY_STATE_CONSUMABLE  0x000002
#define  SC_ACCESSORY_STATE_PERMANENT   0x000004

/* Bits in the accessory state SHIELD flag */
#define  SC_ACCESSORY_SHIELD_STANDARD   0x000100
#define  SC_ACCESSORY_SHIELD_MAGNETIC   0x000200
#define  SC_ACCESSORY_SHIELD_FORCE      0x000400

/* Bits in the accessory state TYPES flag */
#define  SC_ACCESSORY_STATE_SHIELD      0x010000
#define  SC_ACCESSORY_STATE_TRIPLE      0x020000
#define  SC_ACCESSORY_STATE_AUTO_DEF    0x040000
#define  SC_ACCESSORY_STATE_TRIGGER     0x080000
#define  SC_ACCESSORY_STATE_FUEL        0x100000
#define  SC_ACCESSORY_STATE_BATTERY     0x200000
#define  SC_ACCESSORY_STATE_RECHARGE    0x400000
#define  SC_ACCESSORY_STATE_GUIDANCE    0x800000


/* Macros for accessory info */
#define  SC_ACCESSORY_TYPE(a)           ((a)->ident)
#define  SC_ACCESSORY_IS_USELESS(a)     ((a)->useless)
#define  SC_ACCESSORY_IS_INFINITE(a)    ((a)->bundle <= 0 || (a)->price <= 0)
#define  SC_ACCESSORY_IS_NULL(a)        ((a) == NULL || (a)->state & SC_ACCESSORY_STATE_NULL)
#define  SC_ACCESSORY_IS_FUEL(a)        ((a)->state & SC_ACCESSORY_STATE_FUEL)
#define  SC_ACCESSORY_IS_SHIELD(a)      ((a)->state & SC_ACCESSORY_STATE_SHIELD)
#define  SC_ACCESSORY_IS_TRIPLE(a)      ((a)->state & SC_ACCESSORY_STATE_TRIPLE)
#define  SC_ACCESSORY_IS_TRIGGER(a)     ((a)->state & SC_ACCESSORY_STATE_TRIGGER)
#define  SC_ACCESSORY_IS_AUTO_DEF(a)    ((a)->state & SC_ACCESSORY_STATE_AUTO_DEF)
#define  SC_ACCESSORY_IS_PERMANENT(a)   ((a)->state & SC_ACCESSORY_STATE_PERMANENT)
#define  SC_ACCESSORY_IS_CONSUMABLE(a)  ((a)->state & SC_ACCESSORY_STATE_CONSUMABLE)
#define  SC_ACCESSORY_IS_BATTERY(a)     ((a)->state & SC_ACCESSORY_STATE_BATTERY)
#define  SC_ACCESSORY_IS_RECHARGER(a)   ((a)->state & SC_ACCESSORY_STATE_RECHARGE)
#define  SC_ACCESSORY_IS_GUIDANCE(a)    ((a)->state & SC_ACCESSORY_STATE_GUIDANCE)

/* Macros for shield info */
#define  SC_ACCESSORY_SHIELD_IS_STANDARD(a)  ((a)->state & SC_ACCESSORY_SHIELD_STANDARD)
#define  SC_ACCESSORY_SHIELD_IS_MAGNETIC(a)  ((a)->state & SC_ACCESSORY_SHIELD_MAGNETIC)
#define  SC_ACCESSORY_SHIELD_IS_FORCE(a)     ((a)->state & SC_ACCESSORY_SHIELD_FORCE)
#define  SC_ACCESSORY_SHIELD_CHAR(a)         (SC_ACCESSORY_SHIELD_IS_MAGNETIC(a) ? 'M' : \
                                                (SC_ACCESSORY_SHIELD_IS_FORCE(a) ? 'F' : 'S'))

/* Stuff we can call on accessory_configs */
#define  SC_ACCESSORY_DEFAULT(ac)   (sc_accessory_first((ac), SC_ACCESSORY_LIMIT_ALL))


/* Accessory configuration */
typedef struct _sc_accessory_config {
   struct _sc_registry *registry;   /* The game data registry */
   int armslevel;                   /* Current technology (arms) level */
   int registryclass;               /* Registry class ID for accessories */
   bool uselessitems;               /* Useless items appear in inventory? */
} sc_accessory_config;


/* Accessory information lookup */
int sc_accessory_count(const sc_accessory_config *ac, int flags);
sc_accessory_info *sc_accessory_lookup(const sc_accessory_config *ac, int id, int flags);
sc_accessory_info *sc_accessory_lookup_by_name(const sc_accessory_config *ac, const char *name, int flags);
sc_accessory_info *sc_accessory_first(const sc_accessory_config *ac, int flags);
sc_accessory_info *sc_accessory_next(const sc_accessory_config *ac, const sc_accessory_info *info, int flags);


/* Accessory statistics */
void sc_accessory_info_line(const sc_accessory_config *ac, const sc_accessory_info *info, char *buffer, int buflen);


/* Weapon configuration setup */
sc_accessory_config *sc_accessory_config_create(const struct _sc_config *c);
void sc_accessory_config_destroy(sc_accessory_config **ac);
void sc_accessory_info_free(sc_accessory_info **ai);


/* Misc.:  Clear player inventories. */
void sc_accessory_inventory_clear(sc_accessory_config *ac);


/* Summary of the accessory state bits, for use in saddconf.c */
const char **sc_accessory_state_bit_names(void);
const unsigned int *sc_accessory_state_bit_items(void);


#endif /* __saccessory_h_included */
