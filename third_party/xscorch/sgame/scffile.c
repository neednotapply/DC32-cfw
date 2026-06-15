/* $Header: /fridge/cvs/xscorch/sgame/scffile.c,v 1.30 2009-04-26 17:39:38 jacob Exp $ */
/*
   
   xscorch - scffile.c        Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/
    
   Configuration file processing
    

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
#include <sys/stat.h>
#include <unistd.h>

#include <scffile.h>
#include <saddconf.h>
#include <sconfig.h>
#include <seconomy.h>
#include <sland.h>
#include <sphysics.h>
#include <splayer.h>
#include <stankpro.h>
#include <sweapon.h>

#include <sai/sai.h>

#include <libj/jreg/libjreg.h>
#include <libj/jstr/libjstr.h>



/* Old loaders */
int sc_old_config_file_load(sc_config *c);



/* Config class definitions */
static const reg_class_data _reg_file_class[] = {
 { "accessoryDefs",     REG_STRING,  NULL },
 { "economyDefs",       REG_STRING,  NULL },
 { "tankProfiles",      REG_STRING,  NULL },
 { "weaponDefs",        REG_STRING,  NULL },
 { 0, 0, 0 }
};
static const reg_class_data _reg_font_class[] = {
 { "fixedFont",         REG_STRING,  NULL },
 { "italicFixedFont",   REG_STRING,  NULL },
 { "boldFixedFont",     REG_STRING,  NULL },
 { 0, 0, 0 }
};
static const reg_class_data _reg_options_class[] = {
 { "mode",              REG_STRING,  NULL },
 { "interleave",        REG_BOOLEAN, NULL },
 { "teams",             REG_STRING,  NULL },
 { "order",             REG_STRING,  NULL },
 { "talkMode",          REG_STRING,  NULL },
 { "talkProbability",   REG_INTEGER, NULL },
 { "extendedStatus",    REG_BOOLEAN, NULL },
 { "tooltips",          REG_BOOLEAN, NULL },
 { 0, 0, 0 }
};
static const reg_class_data _reg_player_class[] = {
 { "playerType",        REG_STRING,  NULL },
 { "playerName",        REG_STRING,  NULL },
 { 0, 0, 0 }
};
static const reg_class_data _reg_players_class[] = {
 { "numPlayers",        REG_INTEGER, NULL },
 { "numRounds",         REG_INTEGER, NULL },
 { 0, 0, 0 }
};
static const reg_class_data _reg_graphics_class[] = {
 { "fieldWidth",        REG_INTEGER, NULL },
 { "fieldHeight",       REG_INTEGER, NULL },
 { "maxHeight",         REG_INTEGER, NULL },
 { "dithering",         REG_BOOLEAN, NULL },
 { "animation",         REG_BOOLEAN, NULL },
 { "graphicsFast",      REG_BOOLEAN, NULL },
 { "computersFast",     REG_BOOLEAN, NULL },
 { 0, 0, 0 }
};
static const reg_class_data _reg_economics_class[] = {
 { "interestRate",      REG_DOUBLEV, NULL },
 { "dynamicInterest",   REG_BOOLEAN, NULL },
 { "initialCash",       REG_INTEGER, NULL },
 { "computersBuy",      REG_BOOLEAN, NULL },
 { "computersAggressive",REG_BOOLEAN,NULL },
 { "freeMarket",        REG_BOOLEAN, NULL },
 { "lottery",           REG_BOOLEAN, NULL },
 { "scoring",           REG_STRING,  NULL }, /* TEMP - OBSOLETE */
 { "economy",           REG_STRING,  NULL },
 { 0, 0, 0 }
};
static const reg_class_data _reg_physics_class[] = {
 { "airViscosity",      REG_DOUBLEV, NULL },
 { "gravity",           REG_DOUBLEV, NULL },
 { "groundDamping",     REG_DOUBLEV, NULL },
 { "maxWindSpeed",      REG_DOUBLEV, NULL },
 { "dynamicWind",       REG_BOOLEAN, NULL },
 { "suspendDirt",       REG_INTEGER, NULL },
 { "tanksFall",         REG_INTEGER, NULL },
 { "bordersExtend",     REG_INTEGER, NULL },
 { "walls",             REG_STRING,  NULL },
 { 0, 0, 0 }
};
static const reg_class_data _reg_weapons_class[] = {
 { "armsLevel",         REG_INTEGER, NULL },
 { "bombIconSize",      REG_INTEGER, NULL },
 { "tunneling",         REG_BOOLEAN, NULL },
 { "scaling",           REG_DOUBLEV, NULL },
 { "tracePaths",        REG_BOOLEAN, NULL },
 { "uselessItems",      REG_BOOLEAN, NULL },
 { 0, 0, 0 }
};
static const reg_class_data _reg_landscape_class[] = {
 { "sky",               REG_STRING,  NULL },
 { "hostile",           REG_BOOLEAN, NULL },
 { "generator",         REG_STRING,  NULL },
 { "bumpiness",         REG_DOUBLEV, NULL },
 { 0, 0, 0 }
};
static const reg_class_data _reg_ai_controller_class[] = {
 { "humanTargets",      REG_BOOLEAN, NULL },
 { "allowOffsets",      REG_BOOLEAN, NULL },
 { "alwaysOffset",      REG_BOOLEAN, NULL },
 { "enableScan",        REG_BOOLEAN, NULL },
 { "noBudget",          REG_BOOLEAN, NULL },
 { 0, 0, 0 }
};
static const reg_class_data _reg_sound_class[] = {
 { "enableSound",       REG_BOOLEAN, NULL },
 { "useHQMixer",        REG_BOOLEAN, NULL },
 { 0, 0, 0 }
};

/* Configuration class names */
static const reg_class_list _reg_class_dataes[] = {
 { "file_class",        _reg_file_class },
 { "font_class",        _reg_font_class },
 { "player_class",      _reg_player_class },
 { "players_class",     _reg_players_class },
 { "options_class",     _reg_options_class },
 { "graphics_class",    _reg_graphics_class },
 { "economics_class",   _reg_economics_class },
 { "physics_class",     _reg_physics_class },
 { "weapons_class",     _reg_weapons_class },
 { "landscape_class",   _reg_landscape_class },
 { "aicontroller_class",_reg_ai_controller_class },
 { "sound_class",       _reg_sound_class },
 { 0, 0 }
};

/* Expected toplevel variables */
static const reg_class_data _reg_top_class[] = {
 { "files",             REG_BLOCK,   "file_class" },
 { "fonts",             REG_BLOCK,   "font_class" },
 { "players",           REG_BLOCK,   "players_class" },
 { "options",           REG_BLOCK,   "options_class" },
 { "graphics",          REG_BLOCK,   "graphics_class" },
 { "economics",         REG_BLOCK,   "economics_class" },
 { "physics",           REG_BLOCK,   "physics_class" },
 { "weapons",           REG_BLOCK,   "weapons_class" },
 { "landscape",         REG_BLOCK,   "landscape_class" },
 { "aicontroller",      REG_BLOCK,   "aicontroller_class" },
 { "sound",             REG_BLOCK,   "sound_class" },
 { 0, 0, 0 }
};



static int _sc_config_make_directory(void) {
/* sc_config_make_directory
   Construct a new local directory for config files, etc. */

   char dirname[SC_FILENAME_LENGTH];   /* Name of the directory */
   struct stat s;                      /* For the stat() calls */

   /* Figure out the appropriate directory name */
   sbprintf(dirname, sizeof(dirname), "%s/%s", getenv("HOME"), SC_LOCAL_CONFIG_DIR);

   /* Check if the directory exists; try to make it otherwise. */
   if(stat(dirname, &s) < 0) {
      if(mkdir(dirname, 0755) < 0) {
         fprintf(stderr, "warning:  cannot create configuration directory \"%s\"\n", dirname);
         return(false);
      } /* Made a new directory? */
   } /* Did the directory exist? */
   
   /* Return success */   
   return(true);

}



int sc_config_file_init(sc_config *c) {
/* sc_config_file_init
   Initialise a registry database.  This function only initialises
   the registry structure with the classes and variables defined
   above; it does not (yet) read the configuration file. */

   char buf[REG_BUFFER];   /* Random buffer */
   reg *r;                 /* New registry DB */
   int i;                  /* Player iterator */

   /* Make sure the local directory exists. */
   if(!_sc_config_make_directory()) return(false);

   /* Figure out the filename for the conf file, and allocate the
      new registry database now. */
   sbprintf(buf, sizeof(buf), "%s/%s/%s", getenv("HOME"), SC_LOCAL_CONFIG_DIR, SC_LOCAL_CONFIGURATION);
   c->cfreg = reg_new(buf);
   if(c->cfreg == NULL) return(false);

   /* Setup the class listing */
   r = c->cfreg;
   reg_class_register_list(r, _reg_class_dataes);

   /* Register required toplevel variables, by setting a 
      default type for the variables. */
   reg_class_register_vars(r, REG_TOP, _reg_top_class);
   for(i = 0; _reg_top_class[i].name != NULL; ++i) {
      reg_set_block(r, NULL, _reg_top_class[i].name, _reg_top_class[i].klass);
   }

   /* Setup the players class, which has to be constructed dynamically
      (because we don't know the number of players available). */   
   for(i = 0; i < SC_MAX_PLAYERS; ++i) {
      sbprintf(buf, sizeof(buf), "player_%d", i + 1);
      reg_class_register_var(r, "players_class", buf, REG_BLOCK, "player_class");
      sbprintf(buf, sizeof(buf), "players/player_%d", i + 1);
      reg_set_block(r, NULL, buf, "player_class");
   }

   /* Success */   
   return(true);

}



int sc_config_file_load(sc_config *c) {
/* sc_config_file_load
   Load the registry from user's config file. */

   int version;            /* Version of the config file */
   char buf[REG_BUFFER];   /* A temporary buffer */
   reg *r;                 /* Registry database */
   reg_var *player;        /* Player structure */
   int i;                  /* Player iterator */

   /* Load the registry */
   r = c->cfreg;
   if(!reg_load(r)) {
      printf("Didn't find a configuration file, will use game defaults.\n");
      return(true);
   }

   /* Find out what version we're playing with */
   version = 0;
   reg_get_integer(r, NULL, "version", &version);
   if(version < SC_CONFIG_VERSION) {
      fprintf(stderr, "config_file_load: failed to load registry version %d\n", SC_CONFIG_VERSION);
      fprintf(stderr, "config_file_load: trying to load using older reader.\n");
      fprintf(stderr, "config_file_load: please update your config file by resaving it.\n");
      return(sc_old_config_file_load(c));
   }

   /* Get the file datum */
   reg_get_string(r, NULL, "files/accessoryDefs", c->accessory_file, SC_FILENAME_LENGTH);
   reg_get_string(r, NULL, "files/tankProfiles",  c->profile_file,   SC_FILENAME_LENGTH);
   reg_get_string(r, NULL, "files/scoringDefs",   c->scoring_file,   SC_FILENAME_LENGTH);
   reg_get_string(r, NULL, "files/weaponDefs",    c->weapon_file,    SC_FILENAME_LENGTH);

   /* Get the font datum */
   reg_get_string(r, NULL, "fonts/fixedFont",       c->fixed_font,        SC_FONT_LENGTH);
   reg_get_string(r, NULL, "fonts/italicFixedFont", c->italic_fixed_font, SC_FONT_LENGTH);
   reg_get_string(r, NULL, "fonts/boldFixedFont",   c->bold_fixed_font,   SC_FONT_LENGTH);

   /* Load the auxiliary files here */
   if(*c->accessory_file != '\0' && !sc_addconf_append_file(SC_ADDCONF_ACCESSORIES, c->accessory_file, c->accessories)) {
      fprintf(stderr, "Failed to load additional accessories data from \"%s\"\n", c->accessory_file);
   } /* Loading accessories... */
   if(*c->profile_file != '\0' && !sc_tank_profile_add(&c->tanks, c->profile_file)) {
      fprintf(stderr, "Failed to load additional tank profile data from \"%s\"\n", c->profile_file);
   } /* Loading tank profiles... */
   if(*c->scoring_file != '\0' && !sc_addconf_append_file(SC_ADDCONF_SCORINGS, c->scoring_file, c->economics)) {
      fprintf(stderr, "Failed to load additional economy scoring data from \"%s\"\n", c->scoring_file);
   } /* Loading economy scoring... */
   if(*c->weapon_file != '\0' && !sc_addconf_append_file(SC_ADDCONF_WEAPONS, c->weapon_file, c->weapons)) {
      fprintf(stderr, "Failed to load additional weapons data from \"%s\"\n", c->weapon_file);
   } /* Loading weapons... */

   /* Load player data */
   reg_get_integer(r, NULL, "players/numPlayers", &c->numplayers);
   reg_get_integer(r, NULL, "players/numRounds",  &c->numrounds);
   for(i = 0; i < SC_MAX_PLAYERS; ++i) {
      sbprintf(buf, sizeof(buf), "players/player_%d", i + 1);
      player = reg_get_var(r, NULL, buf);
      c->players[i]->aitype = reg_get_integer_from_values(r, player, "playerType", c->players[i]->aitype, sc_ai_names(), sc_ai_types());
      reg_get_string(r, player, "playerName", c->players[i]->name, SC_PLAYER_NAME_LENGTH);
   }

   /* Load basic options */   
   c->options.mode  = reg_get_integer_from_values(r, NULL, "options/mode",     c->options.mode,  sc_config_mode_names(),  sc_config_mode_types());
   c->options.team  = reg_get_integer_from_values(r, NULL, "options/teams",    c->options.team,  sc_config_team_names(),  sc_config_team_types());
   c->options.order = reg_get_integer_from_values(r, NULL, "options/order",    c->options.order, sc_config_order_names(), sc_config_order_types());
   c->options.talk  = reg_get_integer_from_values(r, NULL, "options/talkMode", c->options.talk,  sc_config_talk_names(),  sc_config_talk_types());
   reg_get_integer(r, NULL, "options/talkProbability", &c->options.talkprob);
   reg_get_boolean(r, NULL, "options/extendedStatus",  &c->options.extstatus);
   reg_get_boolean(r, NULL, "options/tooltips",        &c->options.tooltips);
   reg_get_boolean(r, NULL, "options/interleave",      &c->options.interleave);

   /* Load graphics settings */
   reg_get_integer(r, NULL, "graphics/fieldWidth",    &c->fieldwidth);
   reg_get_integer(r, NULL, "graphics/fieldHeight",   &c->fieldheight);
   reg_get_integer(r, NULL, "graphics/maxHeight",     &c->maxheight);
   reg_get_boolean(r, NULL, "graphics/dithering",     &c->graphics.gfxdither);
   reg_get_boolean(r, NULL, "graphics/animation",     &c->graphics.gfxanimate);
   reg_get_boolean(r, NULL, "graphics/graphicsFast",  &c->graphics.gfxfast);
   reg_get_boolean(r, NULL, "graphics/computersFast", &c->graphics.gfxcompfast);
   sc_land_setup(c->land, c->fieldwidth, c->fieldheight, sc_land_flags(c));

   /* Load economy options */   
   reg_get_doublev(r, NULL, "economics/interestRate",        &c->economics->interestrate);
   reg_get_boolean(r, NULL, "economics/dynamicInterest",     &c->economics->dynamicinterest);
   reg_get_integer(r, NULL, "economics/initialCash",         &c->economics->initialcash);
   reg_get_boolean(r, NULL, "economics/computersBuy",        &c->economics->computersbuy);
   reg_get_boolean(r, NULL, "economics/computersAggressive", &c->economics->computersaggressive);
   reg_get_boolean(r, NULL, "economics/freeMarket",          &c->economics->freemarket);
   reg_get_boolean(r, NULL, "economics/lottery",             &c->economics->lottery);
   reg_get_string( r, NULL, "economics/scoring",              c->economics->scoringname, SC_ECONOMY_MAX_NAME_LEN);

   /* Load physics options */   
   reg_get_doublev(r, NULL, "physics/airViscosity",  &c->physics->airviscosity);
   reg_get_doublev(r, NULL, "physics/gravity",       &c->physics->gravity);
   reg_get_doublev(r, NULL, "physics/tunnelDamping", &c->physics->damping);
   reg_get_doublev(r, NULL, "physics/maxWindSpeed",  &c->physics->maxwind);
   reg_get_boolean(r, NULL, "physics/dynamicWind",   &c->physics->dynamicwind);
   reg_get_integer(r, NULL, "physics/suspendDirt",   &c->physics->suspenddirt);
   reg_get_integer(r, NULL, "physics/tanksFall",     &c->physics->tanksfall);
   reg_get_integer(r, NULL, "physics/bordersExtend", &c->physics->bordersextend);
   c->physics->walls = reg_get_integer_from_values(r, NULL, "physics/walls", c->physics->walls, sc_physics_wall_names(), sc_physics_wall_types());

   /* Load weapons options */
   reg_get_integer(r, NULL, "weapons/armsLevel",    &c->weapons->armslevel);
   reg_get_integer(r, NULL, "weapons/bombIconSize", &c->weapons->bombiconsize);
   reg_get_boolean(r, NULL, "weapons/tunneling",    &c->weapons->tunneling);
   reg_get_doublev(r, NULL, "weapons/scaling",      &c->weapons->scaling);
   reg_get_boolean(r, NULL, "weapons/tracePaths",   &c->weapons->tracepaths);
   reg_get_boolean(r, NULL, "weapons/uselessItems", &c->weapons->uselessitems);

   /* Load AI Controller options */
   reg_get_boolean(r, NULL, "aicontroller/humanTargets", &c->aicontrol->humantargets);
   reg_get_boolean(r, NULL, "aicontroller/allowOffsets", &c->aicontrol->allowoffsets);
   reg_get_boolean(r, NULL, "aicontroller/alwaysOffset", &c->aicontrol->alwaysoffset);
   reg_get_boolean(r, NULL, "aicontroller/enableScan",   &c->aicontrol->enablescan);
   reg_get_boolean(r, NULL, "aicontroller/noBudget",     &c->aicontrol->nobudget);

   /* Load sound settings */
   reg_get_boolean(r, NULL, "sound/enableSound", &c->enablesound);
   reg_get_boolean(r, NULL, "sound/useHQMixer",  &c->usehqmixer);

   /* Load landscaping options */   
   c->land->sky       = reg_get_integer_from_values(r, NULL, "landscape/sky",       c->land->sky,       sc_land_sky_names(),       sc_land_sky_types());
   reg_get_boolean(r, NULL, "landscape/hostile",   &c->land->hostileenv);
   c->land->generator = reg_get_integer_from_values(r, NULL, "landscape/generator", c->land->generator, sc_land_generator_names(), sc_land_generator_types());
   reg_get_doublev(r, NULL, "landscape/bumpiness", &c->land->bumpiness);

   /* Success */
   return(true);

}



int sc_config_file_save(sc_config *c) {
/* sc_config_file_save
   Save the user options. */

   char buf[REG_BUFFER];   /* A temporary buffer */
   reg *r;                 /* Registry database */
   reg_var *player;        /* Player variable */
   int i;                  /* Player iterator */

   /* Get the registry DB */
   r = c->cfreg;

   /* Save the proper version number. */
   reg_set_integer(r, NULL, "version", SC_CONFIG_VERSION);

   /* Save the file datum */
   reg_set_string(r, NULL, "files/accessoryDefs", c->accessory_file);
   reg_set_string(r, NULL, "files/tankProfiles",  c->profile_file);
   reg_set_string(r, NULL, "files/scoringDefs",   c->scoring_file);
   reg_set_string(r, NULL, "files/weaponDefs",    c->weapon_file);

   /* Save the font datum */
   reg_set_string(r, NULL, "fonts/fixedFont",       c->fixed_font);
   reg_set_string(r, NULL, "fonts/italicFixedFont", c->italic_fixed_font);
   reg_set_string(r, NULL, "fonts/boldFixedFont",   c->bold_fixed_font);

   /* Save the current player data. */
   reg_set_integer(r, NULL, "players/numPlayers", c->numplayers);
   reg_set_integer(r, NULL, "players/numRounds",  c->numrounds);
   for(i = 0; i < SC_MAX_PLAYERS; ++i) {
      sbprintf(buf, sizeof(buf), "players/player_%d", i + 1);
      player = reg_get_var(r, NULL, buf);
      reg_set_string_from_values(r, player, "playerType", c->players[i]->aitype, sc_ai_names(), sc_ai_types());
      reg_set_string(r, player, "playerName", c->players[i]->name);
   }

   /* Save the basic options */   
   reg_set_string_from_values(r, NULL, "options/mode",     c->options.mode,  sc_config_mode_names(),  sc_config_mode_types());
   reg_set_string_from_values(r, NULL, "options/teams",    c->options.team,  sc_config_team_names(),  sc_config_team_types());
   reg_set_string_from_values(r, NULL, "options/order",    c->options.order, sc_config_order_names(), sc_config_order_types());
   reg_set_string_from_values(r, NULL, "options/talkMode", c->options.talk,  sc_config_talk_names(),  sc_config_talk_types());
   reg_set_integer(r, NULL, "options/talkProbability", c->options.talkprob);
   reg_set_boolean(r, NULL, "options/extendedStatus",  c->options.extstatus);
   reg_set_boolean(r, NULL, "options/tooltips",        c->options.tooltips);
   reg_set_boolean(r, NULL, "options/interleave",      c->options.interleave);

   /* Save the graphics options */
   reg_set_integer(r, NULL, "graphics/fieldWidth",    c->fieldwidth);
   reg_set_integer(r, NULL, "graphics/fieldHeight",   c->fieldheight);
   reg_set_integer(r, NULL, "graphics/maxHeight",     c->maxheight);
   reg_set_boolean(r, NULL, "graphics/dithering",     c->graphics.gfxdither);
   reg_set_boolean(r, NULL, "graphics/animation",     c->graphics.gfxanimate);
   reg_set_boolean(r, NULL, "graphics/graphicsFast",  c->graphics.gfxfast);
   reg_set_boolean(r, NULL, "graphics/computersFast", c->graphics.gfxcompfast);

   /* Save the economics options */
   reg_set_doublev(r, NULL, "economics/interestRate",    c->economics->interestrate);
   reg_set_boolean(r, NULL, "economics/dynamicInterest", c->economics->dynamicinterest);
   reg_set_integer(r, NULL, "economics/initialCash",     c->economics->initialcash);
   reg_set_boolean(r, NULL, "economics/computersBuy",    c->economics->computersbuy);
   reg_set_boolean(r, NULL, "economics/computersAggressive", c->economics->computersaggressive);
   reg_set_boolean(r, NULL, "economics/freeMarket",      c->economics->freemarket);
   reg_set_boolean(r, NULL, "economics/lottery",         c->economics->lottery);
   reg_set_string( r, NULL, "economics/scoring",         c->economics->scoringname);

   /* Save the physics options */
   reg_set_doublev(r, NULL, "physics/airViscosity",  c->physics->airviscosity);
   reg_set_doublev(r, NULL, "physics/gravity",       c->physics->gravity);
   reg_set_doublev(r, NULL, "physics/tunnelDamping", c->physics->damping);
   reg_set_doublev(r, NULL, "physics/maxWindSpeed",  c->physics->maxwind);
   reg_set_boolean(r, NULL, "physics/dynamicWind",   c->physics->dynamicwind);
   reg_set_integer(r, NULL, "physics/suspendDirt",   c->physics->suspenddirt);
   reg_set_integer(r, NULL, "physics/tanksFall",     c->physics->tanksfall);
   reg_set_integer(r, NULL, "physics/bordersExtend", c->physics->bordersextend);
   reg_set_string_from_values(r, NULL, "physics/walls", c->physics->walls, sc_physics_wall_names(), sc_physics_wall_types());

   /* Save the weapons options */
   reg_set_integer(r, NULL, "weapons/armsLevel",    c->weapons->armslevel);
   reg_set_integer(r, NULL, "weapons/bombIconSize", c->weapons->bombiconsize);
   reg_set_boolean(r, NULL, "weapons/tunneling",    c->weapons->tunneling);
   reg_set_doublev(r, NULL, "weapons/scaling",      c->weapons->scaling);
   reg_set_boolean(r, NULL, "weapons/tracePaths",   c->weapons->tracepaths);
   reg_set_boolean(r, NULL, "weapons/uselessItems", c->weapons->uselessitems);

   /* Save AI controller options */
   reg_set_boolean(r, NULL, "aicontroller/humanTargets", c->aicontrol->humantargets);
   reg_set_boolean(r, NULL, "aicontroller/allowOffsets", c->aicontrol->allowoffsets);
   reg_set_boolean(r, NULL, "aicontroller/alwaysOffset", c->aicontrol->alwaysoffset);
   reg_set_boolean(r, NULL, "aicontroller/enableScan",   c->aicontrol->enablescan);
   reg_set_boolean(r, NULL, "aicontroller/noBudget",     c->aicontrol->nobudget);

   /* Save sound settings */   
   reg_set_boolean(r, NULL, "sound/enableSound", c->enablesound);
   reg_set_boolean(r, NULL, "sound/useHQMixer",  c->usehqmixer);

   /* Save landscaping options */
   reg_set_string_from_values(r, NULL, "landscape/sky",       c->land->sky,       sc_land_sky_names(),       sc_land_sky_types());
   reg_set_boolean(r, NULL, "landscape/hostile",   c->land->hostileenv);
   reg_set_string_from_values(r, NULL, "landscape/generator", c->land->generator, sc_land_generator_names(), sc_land_generator_types());
   reg_set_doublev(r, NULL, "landscape/bumpiness", c->land->bumpiness);

   /* Write everything to a file. */
   return(reg_save(r));

}
