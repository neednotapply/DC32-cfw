/* $Header: /fridge/cvs/xscorch/sgame/sshield.h,v 1.22 2009-04-26 17:39:44 jacob Exp $ */
/*
   
   xscorch - sshield.h        Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Scorched shields
    

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
#ifndef __sshield_h_included
#define __sshield_h_included


/* Includes */
#include <xscorch.h>


/* Forward declarations */
struct _sc_accessory_info;
struct _sc_explosion;
struct _sc_player;
struct _sc_config;


/* Constants */
#define  SC_SHIELD_ABSORB_HIT       700   /* Damage to shield when it absorbs a missile */
#define  SC_SHIELD_SAPPER_RATE      2.0   /* Sappers take this * ABSORB_HIT out of shields */
#define  SC_SHIELD_RECHARGE_RATE    500   /* Each turn shield->life += this, if recharger */

/* LORC (Lots. Of. Rescale. Constants.) for magnetic ``physics'' */
#define  SC_SHIELD_MAG_MAX_DIST     100   /* Distance in pixels a magnetic shield can reach */
#define  SC_SHIELD_MAG_ATTENUATION  7.00  /* Number of pixels per unit attenuation */
#define  SC_SHIELD_MAG_TO_POWER     1.50  /* Change magnetic scale to power (tunable) */
#define  SC_SHIELD_MAG_TO_COST      0.75  /* Scale magnetic utilization to shield depletion */

/* Macros */
#define  SC_SHIELD_IS_WEAK(s)       ((s)->info->shield < 5000)
#define  SC_SHIELD_IS_MEDIUM(s)     ((s)->info->shield >= 5000 && (s)->info->shield < 10000)
#define  SC_SHIELD_IS_STRONG(s)     ((s)->info->shield >= 10000)


/* Current shield status */
typedef struct _sc_shield {
   struct _sc_accessory_info *info; /* Shield information */
   int life;                        /* Shield remaining life */
} sc_shield;


/* Shield creation and release */
sc_shield *sc_shield_new(struct _sc_accessory_info *acc);
void sc_shield_free(sc_shield **sh);
struct _sc_accessory_info *sc_shield_find_best(const struct _sc_config *c, const struct _sc_player *p);


/* Shield updates */
void sc_shield_init_turn(struct _sc_player *p);
bool sc_shield_would_impact(const struct _sc_config *c, const struct _sc_player *owner,
                            const struct _sc_player *p, int traj_flags,
                            double x, double y, double nextx, double nexty);
int  sc_shield_absorb_explosion(struct _sc_player *p, const struct _sc_explosion *e, int damage);
bool sc_shield_absorb_hit(struct _sc_player *p, bool sapper);


/* Shields that influence reality */
bool sc_shield_get_deflection(struct _sc_config *c, const struct _sc_player *owner, int traj_flags, 
                              double x, double y, double *vx, double *vy);
bool sc_shield_get_reflection(struct _sc_config *c, struct _sc_player *p, int traj_flags,
                              double *x, double *y, double *velx, double *vely);


#endif /* __sshield_h_included */

