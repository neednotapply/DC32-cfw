/* $Header: /fridge/cvs/xscorch/sai/sai.h,v 1.8 2009-04-26 17:39:35 jacob Exp $ */
/*
   
   xscorch - sai.h            Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/
    
   Main header file for AI code
    

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
#ifndef __sai_h_included
#define __sai_h_included


/* Includes */
#include <xscorch.h>       /* Basic declarations */


/* Forward declarations */
struct _sc_config;
struct _sc_player;
struct _sc_trajectory;


/* AI types */
typedef enum _sc_ai_type {
   SC_AI_HUMAN = 0,     /* Human player */
   SC_AI_NETWORK,       /* Network-controlled player */
   SC_AI_MORON,         /* AI is a moron (random shooting) */
   SC_AI_SHOOTER,       /* Goes for line of sight; this AI will buy weapons
                           that have the best yield/price, but will select
                           weapons that have the best yield.  */
   SC_AI_SPREADER,      /* Similar to SHOOTER, but buys weapons with best
                           yield (not the most economical ones).  */
   SC_AI_CHOOSER,       /* Chooses a victim, and attacks until they are
                           dead.  This AI buys only precision weaponry
                           to increase its deadly accuracy to victim.  */
   SC_AI_CALCULATER,    /* Like CHOOSER, but compensates for wind. */
   SC_AI_ANNIHILATER,   /* Shield? We don't need no steenking shield! */
   SC_AI_INSANITY,      /* No one knows ... */
   SC_AI_RANDOM         /* Randomly selected AI */
} sc_ai_type;


/* AI function return codes */
typedef enum _sc_ai_result {
   SC_AI_NO_ACTION = 0,       /* No action was taken (human player?) */
   SC_AI_CONTINUE             /* AI has acted; continue to next player */
} sc_ai_result;


/* AI state */
typedef struct _sc_ai {
   const struct _sc_player *victim; /* Current victim of this AI */
   sc_ai_type realaitype;     /* The real AI type */
} sc_ai;


/* AI controller config */
typedef struct _sc_ai_controller {
   bool humantargets;         /* Human target practice */
   bool allowoffsets;         /* Allow offset to compensate for shielding */
   bool alwaysoffset;         /* Always offset targetting - assume shields */
   bool enablescan;           /* Enable scan refinement on harder AI's */
   bool nobudget;             /* Disable the "typical" budget constraints */
} sc_ai_controller;


/* Condestructors */
sc_ai *sc_ai_new(void);
void sc_ai_free(sc_ai **ai);
sc_ai_controller *sc_ai_controller_new(void);
void sc_ai_controller_free(sc_ai_controller **aic);


/* AI functions */
void sc_ai_init_game(const struct _sc_config *c, struct _sc_player *p);
void sc_ai_init_round(const struct _sc_config *c, struct _sc_player *p);
sc_ai_result sc_ai_player_turn(const struct _sc_config *c, struct _sc_player *p);
sc_ai_result sc_ai_player_buy(const struct _sc_config *c, struct _sc_player *p);
void sc_ai_trajectory_terminus(const struct _sc_config *c, const struct _sc_trajectory *tr);

const char *        sc_ai_name(sc_ai_type ai);
const char **       sc_ai_names(void);
const unsigned int *sc_ai_types(void);
const char **       sc_ai_names_nonet(void);
const unsigned int *sc_ai_types_nonet(void);
const char **       sc_ai_names_nohuman(void);
const unsigned int *sc_ai_types_nohuman(void);


#endif /* __sai_h_included? */
