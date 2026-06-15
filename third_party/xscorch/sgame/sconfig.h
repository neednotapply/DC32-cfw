/* $Header: /fridge/cvs/xscorch/sgame/sconfig.h,v 1.18 2011-08-01 00:01:40 jacob Exp $ */
/*

   xscorch - sconfig.h        Copyright(c) 2000-2003 Justin David Smith
                              Copyright(c) 2001-2003 Jacob Luna Lundberg
   justins(at)chaos2.org      http://chaos2.org/
   jacob(at)gnifty.net        http://www.gnifty.net/

   Scorched configuration header


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
#ifndef __sconfig_h_included
#define __sconfig_h_included


/* Include files */
#include <xscorch.h>


/* Forward structure declarations */
struct _sc_accessory_config;
struct _sc_economy_config;
struct _sc_weapon_config;
struct _sc_ai_controller;
struct _sc_tank_profile;
struct _sc_registry;
struct _sc_lottery;
struct _sc_physics;
struct _sc_window;
struct _sc_player;
struct _sc_color;
struct _sc_land;
struct _sc_game;
struct _reg;

struct _sc_sound;
#if USE_NETWORK
   struct _sc_client;
   struct _sc_server;
#endif /* Network? */


/* Arms level constants */
#define  SC_ARMS_LEVEL_DEF   4         /* Default arms level */
#define  SC_ARMS_LEVEL_MAX   4         /* Maximum allowed arms level */


/* Basic constants */
#define  SC_DEF_PLAYERS          10    /* Default number of players */
#define  SC_DEF_ROUNDS           10    /* Default number of rounds */


/* The following macro determines if we should be "fast" */
#define  SC_CONFIG_GFX_FAST(c)   ((c)->graphics.gfxfast || ((c)->componly && (c)->graphics.gfxcompfast))
#define  SC_CONFIG_NO_ANIM(c)    (SC_CONFIG_GFX_FAST(c) || !(c)->graphics.gfxanimate)
#define  SC_CONFIG_ANIM(c)       (!SC_CONFIG_NO_ANIM(c))


/* Game mode */
typedef enum _sc_config_mode {
   SC_CONFIG_MODE_SEQUENTIAL,          /* Turn-based */
   SC_CONFIG_MODE_SYNCHRONOUS          /* Everyone enters orders, shoot at once */
} sc_config_mode;
const unsigned int *sc_config_mode_types(void);
const char **sc_config_mode_names(void);


/* Teams mode */
typedef enum _sc_config_team {
   SC_CONFIG_TEAM_NONE = 0,            /* No teams; everyone for themselves */
   SC_CONFIG_TEAM_STANDARD,            /* Standard teams */
   SC_CONFIG_TEAM_CORPORATE,           /* Only one bank account per team */
   SC_CONFIG_TEAM_VICIOUS              /* Fend for self, once 1 team left */
} sc_config_team;
const unsigned int *sc_config_team_types(void);
const char **sc_config_team_names(void);


/* Play order */
typedef enum _sc_config_order {
   SC_CONFIG_ORDER_RANDOM,             /* Players go in random order */
   SC_CONFIG_ORDER_LOSERS_FIRST,       /* Whoever is last place goes 1st */
   SC_CONFIG_ORDER_WINNERS_FIRST,      /* Whoever is winning goes 1st */
   SC_CONFIG_ORDER_ROUND_ROBIN         /* First round is random; ++/round */
} sc_config_order;
const unsigned int *sc_config_order_types(void);
const char **sc_config_order_names(void);


/* Who can talk? */
#define  SC_CONFIG_TALK_PROB_DEF 10    /* 10% chance tanks will speak */
typedef enum _sc_config_talk {
   SC_CONFIG_TALK_OFF = 0,             /* No one may speak */
   SC_CONFIG_TALK_COMPUTERS,           /* Only AI players speak */
   SC_CONFIG_TALK_ALL                  /* All tanks may speak */
} sc_config_talk;
const unsigned int *sc_config_talk_types(void);
const char **sc_config_talk_names(void);


/* Graphics configuration */
typedef struct _sc_config_graphics {
   bool gfxdither;                     /* Nonzero if dithering enabled */
   bool gfxanimate;                    /* Animations enabled if nonzero */
   bool gfxfast;                       /* Faster graphics if enabled */
   bool gfxcompfast;                   /* Fast graphics if comp-only */
} sc_config_graphics;


/* Options configuration */
typedef struct _sc_config_options {
   sc_config_mode mode;                /* Game mode (sync, seq?) */
   sc_config_team team;                /* Teams enabled? */
   sc_config_order order;              /* Player turn order */
   sc_config_talk talk;                /* Is talking enabled? */
   int talkprob;                       /* Probability tanks will talk. */
   bool extstatus;                     /* Display extended status info? */
   bool tooltips;                      /* Display tooltips? */
   bool interleave;                    /* Interleaved tracking? */
} sc_config_options;


typedef struct _sc_config {
   /* Screen size */
   int fieldwidth;                     /* Field width */
   int fieldheight;                    /* Field height */
   int maxheight;                      /* Maximum land height */

   /* User-specific data files */
   char accessory_file[SC_FILENAME_LENGTH];  /* Accessories data */
   char profile_file[SC_FILENAME_LENGTH];    /* Tank profiles data */
   char scoring_file[SC_FILENAME_LENGTH];    /* Economy scoring data */
   char weapon_file[SC_FILENAME_LENGTH];     /* Weapons data */
   
   /* Interface fonts */
   char fixed_font[SC_FONT_LENGTH];          /* Fixed normal font */
   char italic_fixed_font[SC_FONT_LENGTH];   /* Fixed italicized font */
   char bold_fixed_font[SC_FONT_LENGTH];     /* Fixed boldface font */

   /* Basic options */
   sc_config_options options;          /* Basic gameplay options */
   sc_config_graphics graphics;        /* Graphics configuration */
   bool insanity;                      /*   DICK LAURANT IS DEAD   */

   /* Options with their own config files */
   struct _sc_accessory_config *accessories; /* Accessories configuration */
   struct _sc_ai_controller    *aicontrol;   /* AI controller configuration */
   struct _sc_economy_config   *economics;   /* Economics configuration */
   struct _sc_tank_profile     *tanks;       /* Player tank structures */
   struct _sc_weapon_config    *weapons;     /* Weapons configuration */

   /* Game data registry */
   struct _sc_registry *registry;      /* The runtime game data registry */

   /* File controller */
   struct _reg *cfreg;                 /* Config read from file */

   /* Window interface */
   struct _sc_window *window;          /* Pointer to the game window */

   /* Color manipulation */
   struct _sc_color *colors;           /* Information on each gradient */

   /* Sound driver? */
   bool enablesound;                   /* True if sound is permitted */
   bool usehqmixer;                    /* True if use the hq mixer */
   struct _sc_sound *sound;            /* Sound controller */

   /* Network? */
   #if USE_NETWORK
      struct _sc_client *client;       /* Network client connection */
      struct _sc_server *server;       /* Network server connection */
   #endif /* Network enabled? */

   /* Round information */
   int curround;                       /* Current round number. */
   int numrounds;                      /* Number of rounds to play */

   /* Player information */
   int numplayers;                     /* Total number of players */
   struct _sc_player *players[SC_MAX_PLAYERS];  /* Info on each player */
   struct _sc_player *plorder[SC_MAX_PLAYERS];  /* Game turn ordering */
   bool componly;                      /* Only computer players survive */
   int field_position[SC_MAX_PLAYERS]; /* Positions of players on the field */

   /* Other game information */
   struct _sc_land *land;              /* Current landscape. */
   struct _sc_game *game;              /* Current game state */
   struct _sc_physics *physics;        /* Game physics */
   struct _sc_lottery *lottery;        /* Lottery state */
} sc_config;


/* Configuration creation */
sc_config *sc_config_new(int *argc, char ***argv);
void sc_config_free(sc_config **c);
bool sc_config_okay_to_begin(const sc_config *c);
void sc_config_init_game(sc_config *c);
void sc_config_init_round(sc_config *c);
void sc_config_init_turn(sc_config *c);


#endif /* __sconfig_h_included */
