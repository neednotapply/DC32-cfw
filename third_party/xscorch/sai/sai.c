/* $Header: /fridge/cvs/xscorch/sai/sai.c,v 1.12 2009-04-26 17:39:34 jacob Exp $ */
/*
   
   xscorch - sai.c            Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/
    
   Main file for AI code
    

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
#include <saiint.h>              /* Main AI internal header */

#include <sgame/sconfig.h>       /* Need config data structure */
#include <sgame/sland.h>         /* Need sc_land_calculate_deltas */
#include <sgame/sphysics.h>      /* Need for debugging code */
#include <sgame/splayer.h>       /* Need player data structure */
#include <sutil/srand.h>         /* Random AI selection */



void sc_ai_init_game(__libj_unused const struct _sc_config *c, sc_player *p) {
/* sc_ai_init_game */

   if(p->aitype == SC_AI_RANDOM) {
      do { /* Need a random AI */
         p->ai->realaitype = game_lrand(SC_AI_RANDOM);
      } while(p->ai->realaitype == SC_AI_HUMAN);
   } else {
      p->ai->realaitype = p->aitype;
   } /* Selecting a random AI? */

}



void sc_ai_init_round(__libj_unused const struct _sc_config *c, sc_player *p) {
/* sc_ai_init_round */

   p->ai->victim = NULL;

}



sc_ai *sc_ai_new(void) {

   return((sc_ai *)malloc(sizeof(sc_ai)));

}



void sc_ai_free(sc_ai **ai) {

   if(ai == NULL || *ai == NULL) return;
   free(*ai);
   *ai = NULL;

}



sc_ai_controller *sc_ai_controller_new(void) {

   sc_ai_controller *aic;
   
   aic = (sc_ai_controller *)malloc(sizeof(sc_ai_controller));
   if(aic == NULL) return(NULL);
   
   aic->humantargets = false;
   aic->allowoffsets = false;
   aic->alwaysoffset = false;
   aic->enablescan = false;
   aic->nobudget = false;
   
   return(aic);

}



void sc_ai_controller_free(sc_ai_controller **aic) {

   if(aic == NULL || *aic == NULL) return;
   free(*aic);
   *aic = NULL;

}



void sc_ai_trajectory_terminus(const sc_config *c, const sc_trajectory *tr) {

   if(SC_AI_DEBUG_VICTIMS) {
      const sc_player *vp;
      const sc_player *p;
      int dx;
      int dy;

      if(c == NULL || tr == NULL || tr->player == NULL) return;
      p = tr->player;
      if(p->ai == NULL || p->ai->victim == NULL) return;

      /* We hit something; was it near intended victim? */
      vp = p->ai->victim;
      if(vp->index != tr->victim && tr->victim >= 0) {
         printf("trajectory(%d): intended victim %d, hit victim %d\n", 
                p->index, vp->index, tr->victim);
      }
      if(sc_land_calculate_deltas(c->land, &dx, &dy, vp->x, vp->y, tr->curx, tr->cury)) {
         printf("trajectory(%d): was %g pixels away from intended victim %d\n",
                p->index, sqrt(SQR(dx) + SQR(dy)), vp->index);
      }
   }
   
}



/* AI types and codes */
static const char *_sc_player_ai_names[] = {
   "Human",
   "Network",
   "Moron",
   "Shooter",
   "Spreader",
   "Chooser",      
   "Calculater",    
   "Annihilater",
   "Insanity",
   "Unknown",
   NULL
};
static const unsigned int _sc_player_ai_types[] = {
   SC_AI_HUMAN,
   SC_AI_NETWORK,
   SC_AI_MORON,
   SC_AI_SHOOTER,
   SC_AI_SPREADER,
   SC_AI_CHOOSER,
   SC_AI_CALCULATER,
   SC_AI_ANNIHILATER,
   SC_AI_INSANITY,
   SC_AI_RANDOM,
   0
};


const char **sc_ai_names(void) {
   
   return(_sc_player_ai_names);

}


const unsigned int *sc_ai_types(void) {
   
   return(_sc_player_ai_types);

}


const char *sc_ai_name(sc_ai_type ai) {

   int index = 0;

   while(_sc_player_ai_names[index] != NULL) {
      if(_sc_player_ai_types[index] == ai) return(_sc_player_ai_names[index]);
      ++index;
   }
   return("Unknown");

}



/* AI types and codes */
static const char *_sc_player_ai_names_nonet[] = {
   "Human",
   "Moron",
   "Shooter",
   "Spreader",
   "Chooser",      
   "Calculater",    
   "Annihilater",
   "Insanity",
   "Unknown",
   NULL
};
static const unsigned int _sc_player_ai_types_nonet[] = {
   SC_AI_HUMAN,
   SC_AI_MORON,
   SC_AI_SHOOTER,
   SC_AI_SPREADER,
   SC_AI_CHOOSER,
   SC_AI_CALCULATER,
   SC_AI_ANNIHILATER,
   SC_AI_INSANITY,
   SC_AI_RANDOM,
   0
};


const char **sc_ai_names_nonet(void) {
   
   return(_sc_player_ai_names_nonet);

}


const unsigned int *sc_ai_types_nonet(void) {
   
   return(_sc_player_ai_types_nonet);

}



/* AI types and codes */
static const char *_sc_player_ai_names_nohuman[] = {
   "Moron",
   "Shooter",
   "Spreader",
   "Chooser",      
   "Calculater",    
   "Annihilater",
   "Insanity",
   "Unknown",
   NULL
};
static const unsigned int _sc_player_ai_types_nohuman[] = {
   SC_AI_MORON,
   SC_AI_SHOOTER,
   SC_AI_SPREADER,
   SC_AI_CHOOSER,
   SC_AI_CALCULATER,
   SC_AI_ANNIHILATER,
   SC_AI_INSANITY,
   SC_AI_RANDOM,
   0
};


const char **sc_ai_names_nohuman(void) {
   
   return(_sc_player_ai_names_nohuman);

}


const unsigned int *sc_ai_types_nohuman(void) {
   
   return(_sc_player_ai_types_nohuman);

}


