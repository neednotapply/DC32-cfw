/* $Header: /fridge/cvs/xscorch/sgame/sstate.h,v 1.13 2009-04-26 17:39:44 jacob Exp $ */
/*

   xscorch - sstate.h         Copyright(c) 2000-2004 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Scorched game state machine header


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
#ifndef __sstate_h_included
#define __sstate_h_included


#include <xscorch.h>



/* Forward structures */
struct _sc_config;
struct _sc_game;



/* Warning: when set to true, following flag spews a LOT of data */
#define  SC_STATE_DEBUG                0
#define  SC_STATE_TIMER_DEBUG          0



/* Game state flags */
#define  SC_STATE_PROMPT_FLAG          0x010000
#define  SC_STATE_INVENTORY_FLAG       0x020000
#define  SC_STATE_OPTIONS_FLAG         0x040000
#define  SC_STATE_PAUSE_FLAG           0x080000
#define  SC_STATE_ROUND_FLAG           0x100000
#define  SC_STATE_DONE_FLAG            0x800000

/* Game introduction states */
#define  SC_STATE_PRELUDE_BEGIN        (0x00 | SC_STATE_OPTIONS_FLAG)
#define  SC_STATE_PRELUDE_IDLE         (0x01 | SC_STATE_OPTIONS_FLAG)

/* Game initialisation */
#define  SC_STATE_GAME_BEGIN           (0x10)
#define  SC_STATE_GAME_END             (0x1e)
#define  SC_STATE_GAME_END_IDLE        (0x1f | SC_STATE_PAUSE_FLAG)
#define  SC_STATE_GAME_END_DONE        (SC_STATE_GAME_END_IDLE & ~SC_STATE_PAUSE_FLAG)

/* Buy weapons! */
#define  SC_STATE_INVENTORY_BEGIN      (0x20)
#define  SC_STATE_INVENTORY_PL_BEGIN   (0x21 | SC_STATE_INVENTORY_FLAG)
#define  SC_STATE_INVENTORY_PL_IDLE    (0x22 | SC_STATE_INVENTORY_FLAG)
#define  SC_STATE_INVENTORY_PL_DONE    (SC_STATE_INVENTORY_PL_IDLE | SC_STATE_DONE_FLAG)

/* Interactive part of round */
#define  SC_STATE_ROUND_BEGIN          (0x40)
#define  SC_STATE_AUTO_DEFENSE         (0x41)
#define  SC_STATE_AUTO_DEFENSE_LOOP    (SC_STATE_AUTO_DEFENSE | SC_STATE_DONE_FLAG)
#define  SC_STATE_LOTTERY_DISPLAY      (0x42)
#define  SC_STATE_LOTTERY_DISPLAY_WAIT (SC_STATE_LOTTERY_DISPLAY | SC_STATE_DONE_FLAG)
#define  SC_STATE_ROUND_END            (0x4e)
#define  SC_STATE_ROUND_END_IDLE       (0x4f | SC_STATE_PAUSE_FLAG)
#define  SC_STATE_ROUND_END_DONE       (SC_STATE_ROUND_END_IDLE & ~SC_STATE_PAUSE_FLAG)

/* Player turns */
#define  SC_STATE_TURN_BEGIN           (0x51 | SC_STATE_ROUND_FLAG)
#define  SC_STATE_TURN_PL_BEGIN        (0x52 | SC_STATE_ROUND_FLAG | SC_STATE_PROMPT_FLAG)
#define  SC_STATE_TURN_PL_IDLE         (0x53 | SC_STATE_ROUND_FLAG | SC_STATE_PROMPT_FLAG)
#define  SC_STATE_TURN_PL_DONE         (SC_STATE_TURN_PL_IDLE | SC_STATE_DONE_FLAG)
#define  SC_STATE_TURN_PL_NEXT         (0x54 | SC_STATE_ROUND_FLAG)
#define  SC_STATE_TURN_END             (0x5f | SC_STATE_ROUND_FLAG)

/* Weapon simulation part of round */
#define  SC_STATE_RUN_TALK             (0x80 | SC_STATE_ROUND_FLAG)
#define  SC_STATE_RUN_CREATION         (0x81 | SC_STATE_ROUND_FLAG)
#define  SC_STATE_RUN_WEAPONS          (0x82 | SC_STATE_ROUND_FLAG)
#define  SC_STATE_RUN_EXPLOSION        (0x83 | SC_STATE_ROUND_FLAG)
#define  SC_STATE_RUN_PLAYER_DROP      (0x93 | SC_STATE_ROUND_FLAG)
#define  SC_STATE_RUN_PLAYER_DEATH     (0x95 | SC_STATE_ROUND_FLAG)
#define  SC_STATE_RUN_END              (0xff | SC_STATE_ROUND_FLAG)



/* Game state information */
#define  SC_STATE_IS_PROMPT(g)         ((g)->state & SC_STATE_PROMPT_FLAG)
#define  SC_STATE_IS_DONE(g)           ((g)->state & SC_STATE_DONE_FLAG)
#define  SC_STATE_IS_ENABLED(g)        (SC_STATE_IS_PROMPT(g) && !SC_STATE_IS_DONE(g))
#define  SC_STATE_IS_OPTIONS(g)        ((g)->state & SC_STATE_OPTIONS_FLAG)
#define  SC_STATE_IS_INVENTORY(g)      ((g)->state & SC_STATE_INVENTORY_FLAG)
#define  SC_STATE_IS_PAUSE(g)          ((g)->state & SC_STATE_PAUSE_FLAG)
#define  SC_STATE_IS_ROUND(g)          ((g)->state & SC_STATE_ROUND_FLAG)



/* Game delay times (in millisec) */
#define  SC_DELAY_SHORT                50
#define  SC_DELAY_LONG                 800
#define  SC_DELAY_PROMPT               SC_DELAY_SHORT
#define  SC_DELAY_TALK                 1200
#define  SC_STATE_MACHINE_INTERVAL     50
#define  SC_STATE_EXPLOSION_STEPS      10
#define  SC_STATE_NEXTITER_STEPS       1



#define  SC_STATE_FAST(c, t)           (SC_CONFIG_GFX_FAST(c) ? 0 : (t))
#define  SC_STATE_ANY_EXPL(g)          ((g)->expl_init != NULL || \
                                        (g)->expl_draw != NULL || \
                                        (g)->expl_clear_init != NULL || \
                                        (g)->expl_clear != NULL || \
                                        (g)->expl_done != NULL)



void sc_state_run(struct _sc_config *c, struct _sc_game *g);


#endif /* __sstate_h_included? */
