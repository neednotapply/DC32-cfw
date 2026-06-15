/* $Header: /fridge/cvs/xscorch/sgame/spreround.h,v 1.5 2011-08-01 00:01:41 jacob Exp $ */
/*

   xscorch - spreround.h      Copyright(c) 2001 Jacob Luna Lundberg
   jacob(at)gnifty.net        http://www.gnifty.net/

   Scorched pre round setup called from state machine


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
#ifndef __spreround_h_included
#define __spreround_h_included


/* Includes */
#include <xscorch.h>


/* Forward declarations */
struct _sc_config;
struct _sc_player;
struct _sc_weapon_info;
struct _sc_accessory_info;


/*
 * Auto defense request settings (passed back from GUI code).
 * You must keep this sync'd with sc_auto_def_gtk in sautodef-gtk.c!
 */
typedef struct _sc_auto_def_set {
   struct _sc_accessory_info *auto_guidance;    /* special guidance */
   struct _sc_accessory_info *auto_shield;      /* activate a shield */
   int chute_height;                            /* parachute threshold */
   bool triggers;                               /* use contact triggers */
} sc_auto_def_set;


/* Information about who won the lottery */
typedef struct _sc_lottery {
   struct _sc_weapon_info *stake;       /* What was won */
   struct _sc_player *winner;           /* Who won it */
   bool displayed;                      /* Displayed these results yet? */
} sc_lottery;


/* What we can do with an Auto Defense */
bool sc_autodef_activate(const struct _sc_config *c, struct _sc_player *p, const sc_auto_def_set *ads);
bool sc_autodef_ai_activate(const struct _sc_config *c, struct _sc_player *p);


/* What we can do with The Lottery */
sc_lottery *sc_lottery_new(void);
void sc_lottery_free(sc_lottery **lottery);
void sc_lottery_run(struct _sc_config *c);

/* What we can do in pre-round. */
bool sc_preround_auto_defense(struct _sc_config *c, struct _sc_player *p);
bool sc_preround_lottery(struct _sc_config *c);


#endif /* __spreround_h_included */
