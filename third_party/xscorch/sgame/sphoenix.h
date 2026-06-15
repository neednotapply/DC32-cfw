/* $Header: /fridge/cvs/xscorch/sgame/sphoenix.h,v 1.16 2011-08-01 00:01:41 jacob Exp $ */
/*

   xscorch - sphoenix.h       Copyright(c) 2000-2003 Jacob Luna Lundberg
   jacob(at)gnifty.net        http://www.gnifty.net/

   Scorched phoenix type weapons calls


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
#ifndef __sphoenix_h_included
#define __sphoenix_h_included


/* Includes */
#include <xscorch.h>


/* Type definitions */
struct _sc_config;
struct _sc_weapon;
struct _sc_explosion;
struct _sc_weapon_info;
struct _sc_weapon_config;


/* How likely is a RAND phoenix to modify at any given timestep? */
#define  SC_PHOENIX_PROB_AT_RAND        0.01

/* Default explosion chain size increase. */
#define  SC_PHOENIX_INCREASE_FACTOR     0.85

/* The bounds on bounce power for FROG phoenix weapons.
   They're not exactly real power.  To convert them do:
   sqrt(2 * x^2)  */
#define  SC_PHOENIX_MIN_FROG_POWER      70
#define  SC_PHOENIX_MAX_FROG_POWER      350

/* Scatter yield *= (this + scatter) / this. */
#define  SC_PHOENIX_YIELD_SCATTER_SCALE 10.0


/*
   This scale can be used to increase or decrease the force of the
   separation between parts of a phoenix type weapon like a mirv or
   a leap-frog.  The weapon definition itself will give a relative
   scatter rate between 0 and 15 velocity units inclusive.  This
   number will be multiplied by the SC_PHOENIX_SCATTER_SCALE in
   order to get the actual velocity scale.

   With the current physics, I do not recommend setting this above 2.
   Also, the scatter in the weapon definition is best if less than 8.
*/
#define  SC_PHOENIX_SCATTER_SCALE   1


/*
   These are the modifiers for spider explosion generation.
   o  ARC is how many pixels high to make the drawn trajectories of spider
      explosions.  This should eventually be scaled by the screen size setting.
   o  POWER is a multiplier that can be used to modify the scatter distance.
   o  MAX_LEG_ATTEMPTS limits the attempts to find an acceptable arc for each
      spider leg.  The main purpose of this is to ensure loop termination.
*/
#define  SC_PHOENIX_SPIDER_ARC      128.0
#define  SC_PHOENIX_SPIDER_POWER    0.25
#define  SC_PHOENIX_SPIDER_MAX_LEG_ATTEMPTS     100


/*
   NOTE that modifications to the following two sections must be reflected
   in the functions at the bottom of sphoenix.c.  Also they may require some
   changes to saddconf.c, and perhaps to the weapons.def file as well.
*/


/*
   Phoenix location classes.  These describe locations where phoenix weapons
   may change their composition.  The current options are:
   o  RAND causes the weapon to be modified at a random point in its flight arc.
      The modification is not guaranteed; that is, the weapon may detonate
      unmodified instead.  This is used in Black Market MIRVs.
   o  TANK means the weapon is modified as it is created.  This is for things
      like a "triple tank" that shoots three bullets at once.
   o  APEX means the weapon is modified at the peak of its trajectory.  This is
      used for MIRVs and Death's Heads.
   o  LAND means that the weapon will be modified when it lands, prior to normal
      detonation.  This is used for weapons like the Funky Bomb and Leap Frog.
   A note on Evil Weapon Design is that a weapon may utilize more than one of
   these tactics if it's done right...
*/
#define  SC_PHOENIX_AT_RAND         0x0100      /* Maybe modify weapon at random location */
#define  SC_PHOENIX_AT_TANK         0x0200      /* Modify weapon when fired */
#define  SC_PHOENIX_AT_APEX         0x0400      /* Modify weapon at the apex of its flight */
#define  SC_PHOENIX_AT_LAND         0x0800      /* Modify weapon when it lands */
#define  SC_PHOENIX_IS_AT_RAND(i)   ((i)->ph_fl & SC_PHOENIX_AT_RAND)
#define  SC_PHOENIX_IS_AT_TANK(i)   ((i)->ph_fl & SC_PHOENIX_AT_TANK)
#define  SC_PHOENIX_IS_AT_APEX(i)   ((i)->ph_fl & SC_PHOENIX_AT_APEX)
#define  SC_PHOENIX_IS_AT_LAND(i)   ((i)->ph_fl & SC_PHOENIX_AT_LAND)


/*
   Phoenix weapon action functions.  These specify the function that will be
   used to modify a weapon when it reaches its modification location(s).
   o  CHAIN is used to make weapons that "bounce" and get progressively smaller
      or larger depending on the value of SC_PHOENIX_INCREASE_FACTOR (sweapon.h).
   o  SPIDER explosions create instant explosions in nearby locations.  This
      will be influenced by the yield defined for the spider weapon itself.
   o  SCATTER takes a single weapon and splits it into one or more child weapons.
      The children are scattered depending on SC_PHOENIX_SCATTER_SCALE (above).
   o  CONVERT modifies the weapon's tracking method, effectively converting the
      weapon to a new ``type'' of weapon.  The new tracking method *must not*
      detect phoenix events in the same location as the old did.
   o  DETONATE will cause the weapon to detonate immediately.
*/
#define  SC_PHOENIX_TYPE_CHAIN      0x0001
#define  SC_PHOENIX_TYPE_SPIDER     0x0002
#define  SC_PHOENIX_TYPE_SCATTER    0x0004
#define  SC_PHOENIX_TYPE_CONVERT    0x0008
#define  SC_PHOENIX_TYPE_DETONATE   0x0010
#define  SC_PHOENIX_IS_CHAIN(i)     ((i)->ph_fl & SC_PHOENIX_TYPE_CHAIN)
#define  SC_PHOENIX_IS_SPIDER(i)    ((i)->ph_fl & SC_PHOENIX_TYPE_SPIDER)
#define  SC_PHOENIX_IS_SCATTER(i)   ((i)->ph_fl & SC_PHOENIX_TYPE_SCATTER)
#define  SC_PHOENIX_IS_CONVERT(i)   ((i)->ph_fl & SC_PHOENIX_TYPE_CONVERT)
#define  SC_PHOENIX_IS_DETONATE(i)  ((i)->ph_fl & SC_PHOENIX_TYPE_DETONATE)


/* Constants pertaining to phoenix integers */
#define  SC_PHOENIX_LOCATION        0xff00
#define  SC_PHOENIX_TYPE            0x00ff


/* Macros for generic phoenix weapon info */
#define  SC_PHOENIX_CHILD_COUNT(i)  ((i)->children)
#define  SC_PHOENIX_CHILD_TYPE(i)   ((i)->ph_ch)
#define  SC_PHOENIX_SCATTER(i)      ((i)->scatter)
#define  SC_WEAPON_IS_PHOENIX(i)    ((i)->ph_fl)


/* Return values for status checking */
typedef enum _sc_phoenix_result {
   SC_PHOENIX_NO_ACTION = 0,        /* No action was taken */
   SC_PHOENIX_FAILURE,              /* We try to leave the weapon unmodified */
   SC_PHOENIX_SIZZLE,               /* The weapon effectively sizzled; no explosion */
   SC_PHOENIX_DETONATE,             /* The weapon exploded and was replaced by children */
   SC_PHOENIX_RESET                 /* The tracking was changed and should be reset */
} sc_phoenix_result;


/* sc_phoenix(), which is a unified interface to the different phoenix functions and locations. */
sc_phoenix_result sc_phoenix(int locate, const struct _sc_config *c, struct _sc_weapon **wp, struct _sc_explosion **e);

/* sc_phoenix_verify() scans a phoenix weapon definition and returns non-zero
   if the definition is valid.  Specifically, it returns the number of levels
   of children that are involved in the phoenix definition. */
int sc_phoenix_verify(const struct _sc_weapon_config *wc, const struct _sc_weapon_info *info);

/* Phoenix flag information for saddconf.c */
const char **sc_phoenix_flags_bit_names(void);
const unsigned int *sc_phoenix_flags_bit_items(void);


#endif /* __sphoenix_h_included */
