/* $Header: /fridge/cvs/xscorch/sai/saiint.h,v 1.17 2011-08-01 00:01:40 jacob Exp $ */
/*

   xscorch - saiint.h         Copyright(c) 2001,2000 Justin David Smith
                              Copyright(c) 2002 Jacob Luna Lundberg
   justins(at)chaos2.org      http://chaos2.org/
   jacob(at)gnifty.net        http://www.gnifty.net/

   Internal header file for AI code


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
#ifndef __saiint_h_included
#define __saiint_h_included


/* Includes */
#include <sai.h>           /* AI basic header */


/* Structures */
struct _sc_config;


/* Debugging definitions */
#define  SC_AI_DEBUG_BUY         0
#define  SC_AI_DEBUG_TRAJECTORY  0
#define  SC_AI_DEBUG_SELECT      0
#define  SC_AI_DEBUG_VICTIMS     0
#define  SC_AI_DEBUG_SCAN        0


/* General definitions */
#define  SC_AI_POWER_DELTA_MAX   100   /* Maximum change in power */
#define  SC_AI_ANGLE_DELTA_MAX   60    /* Maximum change in angle */
#define  SC_AI_COMPENSATE_DELTA  8     /* Compensation for shields */

/* Definitions specific to moron */
#define  SC_AI_MORON_MIN_POWER   200   /* Minimum permitted power */

/* Definitions specific to shooter */
#define  SC_AI_SHOOTER_START_A   3     /* Start angle for shooter */
#define  SC_AI_SHOOTER_DELTA_A   45    /* Delta angle (no line-of-sight) */
#define  SC_AI_SHOOTER_SIGHT_A   70    /* Sight angle (if we have line-of-sight) */

/* Constants for scan refinement */
#define  SC_AI_SCAN_MIN_POWER    200   /* Minimum permitted refinement power */


/* Definitions used by the validation */
#define  SC_AI_VALIDATE_DIST     100   /* Distance to validate trajectory to */


/* Budget concerns */
#define  SC_AI_CONSERVATIVE_BUDGET  0.10  /* Conservative budget (naive) */
#define  SC_AI_AGGRESSIVE_BUDGET    0.30  /* Aggressive budget (naive) */
#define  SC_AI_BUY_MAX_OF_WEAPON    3     /* Maximum amt of one weapon to buy */
#define  SC_AI_BUY_MAX_OF_ACCESSORY 1     /* Maximum amt of one acces. to buy */
#define  SC_AI_INVENTORY_HIGH       2     /* AI target purchase in bundles */
#define  SC_AI_MAX_BATTERIES        30    /* Maximum number of batteries to buy */


/* AI prediction for trajectory selection */
#define  SC_AI_WILL_OFFSET(c, p)    ( (c)->aicontrol->allowoffsets &&	\
                                     ((c)->aicontrol->alwaysoffset ||	\
                                      (p)->shield != NULL) )


/* Trajectory selection code */
bool sc_ai_trajectory(const struct _sc_config *c, struct _sc_player *p, const struct _sc_player *victim);
bool sc_ai_trajectory_line(const struct _sc_config *c, struct _sc_player *p, const struct _sc_player *victim);
bool sc_ai_trajectory_wind(const struct _sc_config *c, struct _sc_player *p, const struct _sc_player *victim);
bool sc_ai_trajectory_line_wind(const struct _sc_config *c, struct _sc_player *p, const struct _sc_player *victim);
bool sc_ai_trajectory_scan(const struct _sc_config *c, struct _sc_player *p, const struct _sc_player *victim);


#endif /* __saiint_h_included? */
