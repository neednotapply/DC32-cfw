/* $Header: /fridge/cvs/xscorch/sgame/sconfig.c,v 1.30 2009-04-26 17:39:38 jacob Exp $ */
/*

   xscorch - sconfig.c        Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Scorched configuration


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
#include <sconfig.h>       /* Configuration header */
#include <saccessory.h>    /* Accessory config */
#include <scffile.h>       /* Configuration file */
#include <scolor.h>        /* The colormap */
#include <seconomy.h>      /* Economy config */
#include <sgame.h>         /* Set up game state */
#include <sland.h>         /* Ahoy, landmass!*/
#include <slscape.h>       /* Landscape init */
#include <soptions.h>      /* Need to init game options */
#include <sphysics.h>      /* Game physics model */
#include <splayer.h>       /* Init each player */
#include <spreround.h>     /* Run the lottery */
#include <sregistry.h>     /* Game data registry */
#include <stankpro.h>      /* Tank profile data */
#include <sweapon.h>       /* Weapon config */
#include <swindow.h>       /* okay_to_begin needs */

#include <sai/sai.h>       /* AI controller structure */
#include <snet/snet.h>     /* If networking is permitted */
#include <ssound/ssound.h> /* We manipulate sound */
#include <sutil/srand.h>   /* Need to init random seed */

#include <libj/jreg/libjreg.h>
#include <libj/jstr/libjstr.h>



sc_config *sc_config_new(int *argc, char ***argv) {
/* sc_config_new
   This function creates the configuration structure, initialises all
   values, and then calls the config file reader and options parser to 
   read in all the options.  It accepts the command line arguments as
   input.  */

   sc_config *c;           /* A new configuration structure */
   int i;                  /* General iterator */

   /* Initialise the random number generator */
   sys_randomize();

   /* Allocate the new configuration structure */
   c = (sc_config *)malloc(sizeof(sc_config));
   if(c == NULL) {
      fprintf(stderr, "config_new: failed to allocate config structure, this is critical.\n");
      return(NULL);
   }

   /* Setup config file parse tree */
   if(!sc_config_file_init(c) || c->cfreg == NULL) {
      fprintf(stderr, "config_new: failed to construct config file parser, this is critical.\n");
      return(NULL);
   }

   /* Clear out the user filenames */
   strcopyb(c->profile_file,     "", SC_FILENAME_LENGTH);
   strcopyb(c->weapon_file,      "", SC_FILENAME_LENGTH);
   strcopyb(c->accessory_file,   "", SC_FILENAME_LENGTH);

   /* Set default fontspecs */
   strcopyb(c->fixed_font,          "-adobe-courier-medium-r-*-*-*-120-*", SC_FONT_LENGTH);
   strcopyb(c->italic_fixed_font,   "-adobe-courier-medium-o-*-*-*-120-*", SC_FONT_LENGTH);
   strcopyb(c->bold_fixed_font,     "-adobe-courier-bold-r-*-*-*-120-*",   SC_FONT_LENGTH);

   /* Initialise screen size */
   c->fieldwidth = SC_DEF_FIELD_WIDTH;
   c->fieldheight= SC_DEF_FIELD_HEIGHT;
   c->maxheight  = SC_DEF_MAX_HEIGHT;

   /* Initialise local parameters/structures */
   c->insanity   = false;
   c->numrounds  = SC_DEF_ROUNDS;
   c->numplayers = SC_DEF_PLAYERS;
   c->window     = NULL;
   c->componly   = false;

   /* Set default configuration options */
   c->options.mode      = SC_CONFIG_MODE_SYNCHRONOUS;
   c->options.team      = SC_CONFIG_TEAM_NONE;
   c->options.order     = SC_CONFIG_ORDER_RANDOM;
   c->options.talk      = SC_CONFIG_TALK_ALL;
   c->options.talkprob  = SC_CONFIG_TALK_PROB_DEF;
   c->options.extstatus = true;
   c->options.tooltips  = true;
   c->options.interleave= false;

   /* Set the default graphics options */
   c->graphics.gfxdither   = true;
   c->graphics.gfxanimate  = true;
   c->graphics.gfxfast     = false;
   c->graphics.gfxcompfast = false;

   /* Setup AI controller */
   c->aicontrol = sc_ai_controller_new();
   if(c->aicontrol == NULL) {
      fprintf(stderr, "config_new: failed to build ai_controller.\n");
      return(NULL);
   }

   /* Initialise the physics model */   
   c->physics = sc_physics_new();
   if(c->physics == NULL) {
      fprintf(stderr, "config_new: failed to build physics.\n");
      return(NULL);
   }

   /* Create the new land structure (uninitialised) */
   /* WARNING: must be initialised AFTER physics model */
   c->land = sc_land_new(c->fieldwidth, c->fieldheight, sc_land_flags(c));
   if(c->land == NULL) {
      fprintf(stderr, "config_new: failed to build land.\n");
      return(NULL);
   }

   /* Allocate and initialize the game data registry (this is over 64KiB) */
   /* WARNING: must be initialized BEFORE any registry users
      (currently that means weapons and accessories storage) */
   c->registry = sc_registry_new();
   if(c->registry == NULL) {
      fprintf(stderr, "config_new: failed to build game data registry.\n");
      return(NULL);
   }

   /* Create the new lottery structure */
   c->lottery = sc_lottery_new();
   if(c->lottery == NULL) {
      fprintf(stderr, "config_new: failed to build lottery.\n");
      return(NULL);
   }

   /* Load default tank profiles */
   c->tanks = NULL;
   if(!sc_tank_profile_add(&c->tanks, SC_GLOBAL_DIR "/" SC_TANK_PROFILE_FILE) || c->tanks == NULL) {
      fprintf(stderr, "config_new: failed to build tanks_profile, or no tanks in def file.\n");
      return(NULL);
   }

   /* Create the players; clear the player order */
   /* WARNING: must be initialised AFTER tank profiles */
   for(i = 0; i < SC_MAX_PLAYERS; ++i) {
      c->plorder[i] = NULL;
      c->players[i] = sc_player_new(i, c->tanks);
      if(c->players[i] == NULL) {
         fprintf(stderr, "config_new: failed to build player, %d'th hunk failed.\n", i);
         return(NULL);
      }
   }

   /* Initialise the game state structure */
   c->game = sc_game_new();
   if(c->game == NULL) {
      fprintf(stderr, "config_new: failed to build game.\n");
      return(NULL);
   }

   /* Initialise the color structure */   
   c->colors = sc_color_new();
   if(c->colors == NULL) {
      fprintf(stderr, "config_new: failed to build colors.\n");
      return(NULL);
   }

   /* Initialise the accessories configuration */
   c->accessories = sc_accessory_config_create(c);
   if(c->accessories == NULL) {
      fprintf(stderr, "config_new: failed to build accessory_config, or no accessories in def file.\n");
      return(NULL);
   }

   /* Initialise the economy and markets */
   c->economics = sc_economy_config_create(c);
   if(c->economics == NULL) {
      fprintf(stderr, "config_new: failed to build economy_config, or no scorings in def file.\n");
      return(NULL);
   }

   /* Initialise the weapons configuration */
   c->weapons = sc_weapon_config_create(c);
   if(c->weapons == NULL) {
      fprintf(stderr, "config_new: failed to build weapon_config, or no weapons in def file.\n");
      return(NULL);
   }

   /* Initialise sound controller */
   c->enablesound = true;
   c->usehqmixer = false;
   sc_sound_init();

   /* Initialise network connections? */
   #if USE_NETWORK
      c->client = NULL;
      c->server = NULL;
   #endif /* Network allowed? */

   /* Parse command-line and file options */
   if(!sc_config_file_load(c)) {
      fprintf(stderr, "config_new: critical error loading config file.\n");
      return(NULL);
   }
   if(sc_options_parse(c, *argc, *argv)) return(NULL);

   /* Start sound playback, if permitted */
   c->sound = sc_sound_new(c->enablesound, c->usehqmixer);

   /* Return the new configuration structure */
   return(c);

}



void sc_config_free(sc_config **c) {
/* sc_config_free
   Release the configuration structure, and all associated substructures. */

   int i;                  /* General iterator */

   /* Sanity checks */
   if(c == NULL || *c == NULL) return;

   /* Release all substructures */
   sc_accessory_config_destroy(&(*c)->accessories);
   sc_economy_config_destroy(&(*c)->economics);
   sc_weapon_config_destroy(&(*c)->weapons);
   sc_color_free(&(*c)->colors);
   sc_game_free(&(*c)->game);
   sc_land_free(&(*c)->land);
   sc_physics_free(&(*c)->physics);
   sc_lottery_free(&(*c)->lottery);
   sc_ai_controller_free(&(*c)->aicontrol);
   sc_tank_profile_free(&(*c)->tanks);
   for(i = 0; i < SC_MAX_PLAYERS; ++i) {
      sc_player_free(&((*c)->players[i]));
   }
   reg_free(&(*c)->cfreg);
   sc_sound_free(&(*c)->sound);
   #if USE_NETWORK
      sc_net_client_free(&(*c)->client, "Client shutdown");
      sc_net_server_free(&(*c)->server, "Server shutdown");
   #endif /* Network allowed? */
   sc_registry_free(&(*c)->registry);

   /* Release the config structure */
   free(*c);
   *c = NULL;

}



bool sc_config_okay_to_begin(const sc_config *c) {
/* sc_config_okay_to_begin
   Returns true if it is okay to begin a new game. This performs some
   sanity checking (right number of players, network game setup as
   appropriate, etc) and returns true if all sanity checks pass. Note
   this will try to spawn off some error windows if it finds a prob. */

   bool networkplay;
   int  numhuman;
   int  i;

   /* Number of players? */
   if(c->numplayers < 2) {
      sc_window_message( c->window, "Cannot Start", 
                         "There are fewer than 2 players in the game. "\
                         "If this is a network game, wait for someone "\
                         "else to connect. If this is a private game, "\
                         "add some human or AI players." );
      return(false);
   }

   /* No network AI players in nonnetwork game; human count okay? */
   #if USE_NETWORK
   networkplay = (c->client != NULL);
   #else  /* Network not allowed */
   networkplay = false;
   #endif /* Network game? */
   numhuman = 0;

   /* Count human and network players */
   for(i = 0; i < c->numplayers; ++i) {
      if(c->players[i]->aitype == SC_AI_HUMAN) ++numhuman;
      else if(c->players[i]->aitype == SC_AI_NETWORK && !networkplay) {
         sc_window_message( c->window, "Cannot Start",
                            "A network player has been selected, but "\
                            "we are not in a network game. This should "\
                            "not have happened; please go to the Players "\
                            "window and setup the players to all be human "\
                            "or AI players." );
         return(false);
      }
   }

   /* Make sure there is a human player */
   if(numhuman == 0) {
      sc_window_message( c->window, "Cannot Start",
                         "There are no (local) human players in the game. "\
                         "Please go to the Players window and setup some "\
                         "players to be Human." );
      return(false);
   }

   /* Okay, we can start */
   return(true);

}



void sc_config_init_game(sc_config *c) {

   int i;

   sc_physics_init_game(c->physics);
   for(i = 0; i < c->numplayers; ++i) {
      sc_player_init_game(c, c->players[i]);
   }
   sc_game_init(c->game);
   sc_economy_init(c->economics);
   c->curround = -1;

   /* Clear out inventories! */
   sc_accessory_inventory_clear(c->accessories);
   sc_weapon_inventory_clear(c->weapons);

}



static void _sc_config_rotate_players(sc_config *c) {

   sc_player *pllast;
   int i;

   pllast = c->plorder[0];
   for(i = 0; i < c->numplayers - 1; ++i) {
      c->plorder[i] = c->plorder[i + 1];
   }
   c->plorder[c->numplayers - 1] = pllast;

}



void sc_config_init_round(sc_config *c) {

   int i;

   /* Clear the player location table */
   for(i = 0; i < SC_MAX_PLAYERS; ++i) {
      c->field_position[i] = -1;
   }

   sc_physics_init_round(c->physics);
   sc_land_setup(c->land, c->fieldwidth, c->fieldheight, sc_land_flags(c));
   sc_land_generate(c, c->land);
   for(i = 0; i < c->numplayers; ++i) {
      sc_player_init_round(c, c->players[i]);
   }
   ++c->curround;

   if(c->curround == 0) {
      sc_player_random_order(c, c->plorder);
   } else switch(c->options.order) {
      case SC_CONFIG_ORDER_RANDOM:
         sc_player_random_order(c, c->plorder);
         break;
      case SC_CONFIG_ORDER_ROUND_ROBIN:
         _sc_config_rotate_players(c);
         break;
      case SC_CONFIG_ORDER_WINNERS_FIRST:
         sc_player_winner_order(c, c->plorder);
         break;
      case SC_CONFIG_ORDER_LOSERS_FIRST:
         sc_player_loser_order(c, c->plorder);
         break;
   }

   /* Run the lotto, if it's turned on. */
   if(c->economics->lottery)
      sc_lottery_run(c);

   c->componly = false;

}



void sc_config_init_turn(sc_config *c) {

   int i;

   c->componly = true;
   i = c->numplayers;
   while(--i >= 0) {
      if(c->players[i]->aitype == SC_AI_HUMAN && SC_PLAYER_IS_ALIVE(c->players[i])) c->componly = false;
      if(SC_PLAYER_IS_ALIVE(c->players[i])) sc_player_init_turn(c, c->players[i]);
   }
   sc_physics_update_wind(c->physics);

   /* Update the status bar and display */
   sc_window_paint(c->window, 0, 0, 0, 0, SC_CLEAR_WIND_ARROW | SC_REDRAW_WIND_ARROW);

}



/***     Function hooks for enumerated types    ***/



/* configuration modes */
static const unsigned int _sc_config_mode_types[] = {
   SC_CONFIG_MODE_SEQUENTIAL,
   SC_CONFIG_MODE_SYNCHRONOUS,
   0
};
static const char *_sc_config_mode_names[] = {
   "Sequential",
   "Synchronous",
   NULL
};


const unsigned int *sc_config_mode_types(void) {

   return(_sc_config_mode_types);

}


const char **sc_config_mode_names(void) {

   return(_sc_config_mode_names);

}



/* Team types */
static const unsigned int _sc_config_team_types[] = {
   SC_CONFIG_TEAM_NONE,
   SC_CONFIG_TEAM_STANDARD,
   SC_CONFIG_TEAM_CORPORATE,
   SC_CONFIG_TEAM_VICIOUS,
   0
};
static const char *_sc_config_team_names[] = {
   "None",
   "Standard",
   "Corporate",
   "Viscious",
   NULL
};


const unsigned int *sc_config_team_types(void) {

   return(_sc_config_team_types);

}


const char **sc_config_team_names(void) {

   return(_sc_config_team_names);

}



/* Play order configuration */
static const unsigned int _sc_config_order_types[] = {
   SC_CONFIG_ORDER_RANDOM,
   SC_CONFIG_ORDER_LOSERS_FIRST,
   SC_CONFIG_ORDER_WINNERS_FIRST,
   SC_CONFIG_ORDER_ROUND_ROBIN,
   0
};
static const char *_sc_config_order_names[] = {
   "Random",
   "Losers first",
   "Winners first",
   "Round-robin",
   NULL
};


const unsigned int *sc_config_order_types(void) {

   return(_sc_config_order_types);

}


const char **sc_config_order_names(void) {

   return(_sc_config_order_names);

}



/* Talk modes */
static const unsigned int _sc_config_talk_types[] = {
   SC_CONFIG_TALK_OFF,
   SC_CONFIG_TALK_COMPUTERS,
   SC_CONFIG_TALK_ALL,
   0
};
static const char *_sc_config_talk_names[] = {
   "Off",
   "Computers only",
   "Everyone",
   NULL
};


const unsigned int *sc_config_talk_types(void) {

   return(_sc_config_talk_types);

}


const char **sc_config_talk_names(void) {

   return(_sc_config_talk_names);

}
