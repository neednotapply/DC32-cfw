/* $Header: /fridge/cvs/xscorch/sgame/splayer.h,v 1.21 2009-04-26 17:39:43 jacob Exp $ */
/*

   xscorch - splayer.h        Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Scorched player information


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
#ifndef __splayer_h_included
#define __splayer_h_included


#include <xscorch.h>
#include <sai/sai.h>


/* Forward declarations */
struct _sc_config;
struct _sc_explosion;
struct _sc_weapon_info;
struct _sc_weapon_config;
struct _sc_accessory_info;
struct _sc_accessory_config;


#define  SC_PLAYER_NAME_LENGTH      32       /* Length of player name (chars) */

#define  SC_TANK_MAX_DROP_PER_CYCLE 32       /* Maximum distance tank drops per cycle */
#define  SC_TANK_CLIMB_HEIGHT       3        /* Height tank can climb upward */

#define  SC_PLAYER_MAX_LIFE         1000     /* Maximum life (before rescaling) */
#define  SC_PLAYER_MAX_POWER        1000     /* Maximum power */
#define  SC_PLAYER_POWER_STEP       1        /* Stepping constants */
#define  SC_PLAYER_POWER_BIGSTEP    20
#define  SC_PLAYER_TURRET_STEP      1
#define  SC_PLAYER_TURRET_BIGSTEP   5
#define  SC_BATTERY_RECHARGE_PERCT  1        /* Percentage of life to restore when battery used */

#define  SC_PLAYER_SHIELD_DEFAULTS  0        /* Defaults for sc_player_advance_shield */
#define  SC_PLAYER_SHIELD_CHECK_CUR (1 << 1) /* Check if current shield valid before advance */

#define  SC_PLAYER_IS_ALIVE(p)      ((!(p)->dead) && ((p)->life > 0))


/* Player needs weapons, inventory... */
struct _sc_weapon;
struct _sc_shield;
struct _sc_inventory;
struct _sc_tank_profile;


/* Player data */
typedef struct _sc_player {
   /* Basic player information */
   int index;                       /* Player number */
   char name[SC_PLAYER_NAME_LENGTH];/* Player name */
   const struct _sc_tank_profile *tank;/* Player's tank */
   sc_ai_type aitype;               /* Current AI mode */
   sc_ai *ai;                       /* AI state information */
   
   /*
      Note below that life and death may not always appear consistent.
      If life is nonpositive but dead is false, then the tank still
      exists on the screen, but is inactive and will soon detonate.
      When life is nonpositive, the tank should not perform any further
      actions but is still subject to gravity, etc...  It is only when
      the dead flag goes true that the state machine generally thinks
      the tank is gone, kaput, dust.
    */

   /* Tank information */
   int turret;                      /* Turret angle (degrees, in [0, 180]) */
   int power;                       /* Firing power level */
   int life;                        /* Life remaining */
   bool dead;                       /* Nonzero if tank destroyed */
   int fuel;                        /* Current fuel level * 100 */
   int x;                           /* X coordinate of tank */
   int y;                           /* Y coordinate of tank */
   int field_index;                 /* Index on the playing field */

   /* Number of wins */
   int numwins;                     /* Zero-- you're a loser */
   int kills;                       /* Number of tanks killed */
   int suicides;                    /* Number of suicides */
   int killedby;                    /* Killed by whom? */

   /* Currency and inventory */
   int money;                       /* Player's current money */
   int oldmoney;                    /* Money at begin of round */
   struct _sc_inventory *inventory; /* Player's inventory of weapons */

   /* Player weapons */
   bool armed;                      /* True if weapon is armed */
   int armslevel;                   /* Current player arms level */
   struct _sc_weapon_info *selweapon;/* Currently selected weapon */
   bool contacttriggers;            /* Set if we should try using triggers */
   struct _sc_weapon *weapons;      /* Weapons that are currently active */

   /* Player's accessories */
   int ac_state;                    /* Accessory state/types */
   struct _sc_shield *shield;       /* Player shields (NULL==no shielding) */
   struct _sc_accessory_info *selshield;/* Currently selected shield */
} sc_player;


/* Player creation and initialization */
sc_player *sc_player_new(int index, const struct _sc_tank_profile *tank);
void sc_player_free(sc_player **p);
void sc_player_init_game(const struct _sc_config *c, sc_player *p);
void sc_player_init_round(struct _sc_config *c, sc_player *p);
void sc_player_init_turn(const struct _sc_config *c, sc_player *p);


/* User control of tank */
void sc_player_advance_power( const struct _sc_config *c, sc_player *p, int delta);
void sc_player_advance_turret(const struct _sc_config *c, sc_player *p, int delta);
void sc_player_advance_weapon(const struct _sc_config *c, sc_player *p, int delta);
void sc_player_advance_shield(const struct _sc_config *c, sc_player *p, int flags);
void sc_player_set_power( const struct _sc_config *c, sc_player *p, int power);
void sc_player_set_turret(const struct _sc_config *c, sc_player *p, int turret);
void sc_player_set_weapon(const struct _sc_config *c, sc_player *p, struct _sc_weapon_info *info);
void sc_player_set_shield(const struct _sc_config *c, sc_player *p, struct _sc_accessory_info *info);
bool sc_player_activate_shield(const struct _sc_config *c, sc_player *p);
bool sc_player_activate_best_shield(const struct _sc_config *c, sc_player *p);
bool sc_player_activate_battery(const struct _sc_config *c, sc_player *p);
void sc_player_set_position(const struct _sc_config *c, sc_player *p, int x, int y);
void sc_player_set_contact_triggers(const struct _sc_config *c, sc_player *p, bool flag);
void sc_player_toggle_contact_triggers(const struct _sc_config *c, sc_player *p);
bool sc_player_use_contact_trigger(const struct _sc_config *c, sc_player *p);


/* Miscellaneous inventory data */
int sc_player_battery_count(const struct _sc_config *c, const sc_player *p);
int sc_player_contact_trigger_count(const struct _sc_config *c, const sc_player *p);


/* Player scoring */
void sc_player_inc_wins(struct _sc_config *c, sc_player *p);
void sc_player_died(struct _sc_config *c, sc_player *p);


/* Drop the tank (land no longer supports it) */
bool sc_player_drop_all(struct _sc_config *c);
void sc_player_damage_all(struct _sc_config *c, const struct _sc_explosion *e);
void sc_player_death(const struct _sc_config *c, const struct _sc_player *p, struct _sc_explosion **e);


/* Tank talk */
const char *sc_player_talk(const struct _sc_config *c, const sc_player *p);
const char *sc_player_death_talk(const struct _sc_config *c, const sc_player *p);


/* Player order selection */
sc_player **sc_player_random_order(struct _sc_config *c, sc_player **playerlist);
sc_player **sc_player_winner_order(struct _sc_config *c, sc_player **playerlist);
sc_player **sc_player_loser_order(struct _sc_config *c, sc_player **playerlist);


/* Player movement */
int  sc_player_total_fuel(const struct _sc_accessory_config *ac, const sc_player *p);
bool sc_player_move(const struct _sc_config *c, sc_player *p, int delta);
bool sc_player_passable(const struct _sc_config *c, const sc_player *p, int x, int y);


/* Tank turret position */
int sc_player_turret_x(const sc_player *p, int angle);
int sc_player_turret_y(const sc_player *p, int angle);


/* Impact data */
bool sc_player_would_impact(const struct _sc_config *c, const sc_player *p, int x, int y);


#endif /* __splayer_h_included */
