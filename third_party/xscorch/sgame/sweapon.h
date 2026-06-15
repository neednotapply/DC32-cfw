/* $Header: /fridge/cvs/xscorch/sgame/sweapon.h,v 1.59 2011-08-01 00:01:42 jacob Exp $ */
/*

   xscorch - sweapon.h        Copyright(c) 2000-2003 Jacob Luna Lundberg
                              Copyright(c) 2000-2003 Justin David Smith
   jacob(at)gnifty.net        http://www.gnifty.net/
   justins(at)chaos2.org      http://chaos2.org/

   Scorched basic weapon configuration


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
#ifndef __sweapon_h_included
#define __sweapon_h_included


/* Includes */
#include <xscorch.h>


/* Forward declarations */
struct _sc_trajectory;
struct _sc_explosion;
struct _sc_registry;
struct _sc_player;
struct _sc_config;


/* Basic weapon constants */
#define  SC_WEAPON_BOMB_ICON_DEF    1        /* Default size of bomb icon */
#define  SC_WEAPON_BOMB_ICON_MAX    4        /* Maximum size of bomb icon */
#define  SC_WEAPON_SCALING_MAX      2.0      /* Maximum weapon scaling factor */


/* Definitions with respect to weapon explosions */
#define  SC_WEAPON_NAPALM_RADIUS    13       /* A typical napalm radius */
#define  SC_WEAPON_SMALL_EXPLOSION  40       /* Radius of a small explosion */
#define  SC_WEAPON_MEDIUM_EXPLOSION 120      /* Ridiculous speed ... */
#define  SC_WEAPON_LARGE_EXPLOSION  200      /* Ludicrous speed! ... */

#define  SC_WEAPON_SMALL_FORCE      100      /* Small force for small expl */
#define  SC_WEAPON_MEDIUM_FORCE     300      /* Medium force for medium expl */
#define  SC_WEAPON_LARGE_FORCE      700      /* Large force for big expl */
#define  SC_WEAPON_NAPALM_FORCE     30       /* Typical napalm force */
#define  SC_WEAPON_NAPALM_LIQUID    400      /* Typical amount of napalm */


/* Data on the various types of weapons */
typedef struct _sc_weapon_info {
   /* DO NOT CHANGE the order of the first four items:
      should be ident, armslevel, price, bundle.  */
   int ident;                       /* Unique ID (none if <= 0) */
   int armslevel;                   /* Arms level for this weapon */
   int price;                       /* Price (0 == infinite supply) */
   int bundle;                      /* Size of bundle (0 iff price==0) */
   int state;                       /* General weapon initial state flags */
   int radius;                      /* Explosion radius / radius of effect */
   double angular_width;            /* With in radians of wedge (use 0 for
                                       a normal circular explosion). */
   int force;                       /* Force of explosion (0 == nondestructive) */
   int liquid;                      /* Liquid amount (dribble, napalm, dirt) */
   int scatter;                     /* Scattering power of scatter phoenix */
   int children;                    /* Child counts for use by phoenix code */
   int ph_ch;                       /* ID of phoenix children (0 == no phoenix) */
   int ph_fl;                       /* Flags of phoenix children (0 == no kids) */
   int inventories[SC_MAX_PLAYERS]; /* Player weapon inventories */
   bool useless;                    /* Is weapon a useless item? */
   bool indirect;                   /* Is the weapon just a support def? */
   char *name;                      /* Weapon name */
   char *description;               /* Weapon information */
} sc_weapon_info;


/* Weapon scanning functions need to know what to find and what not to. */
#define  SC_WEAPON_LIMIT_NONE           0x0000
#define  SC_WEAPON_LIMIT_ARMS           0x0001
#define  SC_WEAPON_LIMIT_USELESS        0x0002
#define  SC_WEAPON_LIMIT_INDIRECT       0x0004
#define  SC_WEAPON_LIMIT_ALL            0x00ff

/* Weapon scanning functions need to know in what direction to scan. */
#define  SC_WEAPON_SCAN_DEFAULT         0x0000
#define  SC_WEAPON_SCAN_FORWARD         0x0100
#define  SC_WEAPON_SCAN_REVERSE         0x0200


/*
   Bit names in the state flag.
   Changes to these bits must be reflected in the functions at
   the bottom of sweapon.c.  Also they may require some changes
   to saddconf.c, and perhaps to the weapons.def file as well.
   Note that there are still a couple bytes to spare.
*/
#define  SC_WEAPON_STATE_TYPES      0xffff00
#define  SC_WEAPON_STATE_FLAGS      0x0000ff

/* Bits in the weapon state FLAGS flag */
#define  SC_WEAPON_STATE_NULL       0x000001
#define  SC_WEAPON_STATE_DEFER      0x000002
#define  SC_WEAPON_STATE_SMOKE      0x000004

/* Bits in the weapon state TYPES flag */
#define  SC_WEAPON_STATE_PLASMA     0x000100   /* Plasma weapon, nuke */
#define  SC_WEAPON_STATE_LIQUID     0x000200   /* Liquid dirt, napalm */
#define  SC_WEAPON_STATE_DIRT       0x000400   /* Dirt balls (solid) */
#define  SC_WEAPON_STATE_BEAM       0x000800   /* Laser, beam weapons */
#define  SC_WEAPON_STATE_RIOT       0x001000   /* Riot bombs, e.g. */
#define  SC_WEAPON_STATE_TRIPLE     0x002000   /* Triple turret types */
#define  SC_WEAPON_STATE_ROLLER     0x004000   /* Rollable weapons */
#define  SC_WEAPON_STATE_DIGGER     0x008000   /* Digging weapons */
#define  SC_WEAPON_STATE_SAPPER     0x010000   /* Shield sapping weapons */


/* Macros for weapon info (use on sc_weapon_info only) */
#define  SC_WEAPON_TYPE(i)          ((i)->ident)
#define  SC_WEAPON_IS_EXPLOSION(i)  ((i)->force && (i)->radius)
#define  SC_WEAPON_IS_INFINITE(i)   ((i)->bundle <= 0 || (i)->price <= 0)
#define  SC_WEAPON_IS_USELESS(i)    ((i)->useless)

/* These may be called on either an sc_weapon or an sc_weapon_info */
#define  SC_WEAPON_SCATTERING(w)    ((w)->scatter)
#define  SC_WEAPON_CHILD_COUNT(w)   ((w)->children)
#define  SC_WEAPON_IS_NULL(w)       ((w) == NULL || (w)->state & SC_WEAPON_STATE_NULL)
#define  SC_WEAPON_IS_DEFERRING(w)  ((w)->state & SC_WEAPON_STATE_DEFER)
#define  SC_WEAPON_IS_SMOKING(w)    ((w)->state & SC_WEAPON_STATE_SMOKE)
#define  SC_WEAPON_IS_PLASMA(w)     ((w)->state & SC_WEAPON_STATE_PLASMA)
#define  SC_WEAPON_IS_LIQUID(w)     ((w)->state & SC_WEAPON_STATE_LIQUID)
#define  SC_WEAPON_IS_DIRT(w)       ((w)->state & SC_WEAPON_STATE_DIRT)
#define  SC_WEAPON_IS_NAPALM(w)     (SC_WEAPON_IS_LIQUID(w) && !SC_WEAPON_IS_DIRT(w))
#define  SC_WEAPON_IS_LIQ_DIRT(w)   (SC_WEAPON_IS_LIQUID(w) &&  SC_WEAPON_IS_DIRT(w))
#define  SC_WEAPON_IS_RIOT(w)       ((w)->state & SC_WEAPON_STATE_RIOT)
#define  SC_WEAPON_IS_TRIPLE(w)     ((w)->state & SC_WEAPON_STATE_TRIPLE)
#define  SC_WEAPON_IS_ROLLER(w)     ((w)->state & SC_WEAPON_STATE_ROLLER)
#define  SC_WEAPON_IS_DIGGER(w)     ((w)->state & SC_WEAPON_STATE_DIGGER)
#define  SC_WEAPON_IS_SAPPER(w)     ((w)->state & SC_WEAPON_STATE_SAPPER)

/* Stuff we can call on weapon_configs */
#define  SC_WEAPON_DEFAULT(wc)      (sc_weapon_first((wc), SC_WEAPON_LIMIT_ALL))
#define  SC_WEAPON_TRAJ_FLAGS(c, w) ((((c)->weapons->tunneling && !(w)->triggered) ? \
                                       SC_TRAJ_TUNNELING : SC_TRAJ_DEFAULTS) | \
                                     (SC_WEAPON_IS_SAPPER(w) ? \
                                       SC_TRAJ_HIT_SHIELDS : SC_TRAJ_DEFAULTS))


/* Basic weapon data structure */
typedef struct _sc_weapon {
   struct _sc_trajectory *tr;       /* Weapon's trajectory and heading */
   double exp_res;                  /* explosion size modifier */
   int state;                       /* Weapon state info */
   int playerid;                    /* Who owns this weapon? */
   int children;                    /* Run-time tracking of child count */
   bool triggered;                  /* Weapon has contact trigger */
   struct _sc_weapon *chain;        /* Needed for MIRV or other weapons which
                                       track at multiple locations */
   sc_weapon_info *weaponinfo;      /* Information on this weapon */
} sc_weapon;


/* Weapon configuration */
typedef struct _sc_weapon_config {
   struct _sc_registry *registry;   /* The game data registry */
   int armslevel;                   /* Current technology (arms) level */
   int bombiconsize;                /* Size of a bomb icon (pixels) */
   int registryclass;               /* Registry class ID for weapons */
   bool tunneling;                  /* Nonzero if weapon tunneling allowed */
   bool tracepaths;                 /* Nonzero if weapon tracing enabled */
   bool uselessitems;               /* Useless items appear in inventory? */
   double scaling;                  /* Weapon scaling factor */
} sc_weapon_config;


/* Weapon statistics */
typedef enum {
   SC_WEAPON_STAT_PRICE,
   SC_WEAPON_STAT_YIELD,
   SC_WEAPON_STAT_PRECISION,
   SC_WEAPON_STAT_ECON_YIELD,
   SC_WEAPON_STAT_ECON_PRECISION,
   SC_WEAPON_STAT_PRECISION_YIELD
} sc_weapon_stat;


/* Weapon information lookup */
int sc_weapon_count(const sc_weapon_config *wc, int flags);
sc_weapon_info *sc_weapon_lookup(const sc_weapon_config *wc, int id, int flags);
sc_weapon_info *sc_weapon_lookup_by_name(const sc_weapon_config *wc, const char *name, int flags);
sc_weapon_info *sc_weapon_first(const sc_weapon_config *wc, int flags);
sc_weapon_info *sc_weapon_next(const sc_weapon_config *wc, const sc_weapon_info *info, int flags);


/* Weapon statistics */
double sc_weapon_statistic(const sc_weapon_config *wc, const sc_weapon_info *info, const struct _sc_player *p, sc_weapon_stat statistic);
void sc_weapon_info_line(const sc_weapon_config *wc, const sc_weapon_info *info, char *buffer, int buflen);


/* Weapon creation and release */
sc_weapon *sc_weapon_new(const struct _sc_config *c, sc_weapon_info *i, double x, double y, double vx, double vy, bool ct, int playerid);
void sc_weapon_landfall(struct _sc_config *c, const sc_weapon *wp);
void sc_weapon_free_chain(sc_weapon **wp);
void sc_weapon_free(sc_weapon **wp);


/* Weapon configuration setup */
sc_weapon_config *sc_weapon_config_create(const struct _sc_config *c);
void sc_weapon_config_destroy(sc_weapon_config **wc);
void sc_weapon_info_free(sc_weapon_info **wi);


/* Misc.:  Clear player inventories.  Create explosion from weapon. */
void sc_weapon_inventory_clear(sc_weapon_config *wc);
struct _sc_explosion *sc_weapon_get_explosion(const struct _sc_config *c, const sc_weapon *wp, int x, int y, double direction);


/* Create all weapons for a turn */
bool sc_weapon_create_all(struct _sc_config *c, struct _sc_explosion **e);


/* Print the yields of weapons */
void sc_weapon_print_yields(const sc_weapon_config *wc);


/* Summary of the weapon state bits, for use in saddconf.c */
const char **sc_weapon_state_bit_names(void);
const unsigned int *sc_weapon_state_bit_items(void);


#endif /* __sweapon_h_included */
