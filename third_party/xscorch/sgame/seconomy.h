/* $Header: /fridge/cvs/xscorch/sgame/seconomy.h,v 1.11 2009-04-26 17:39:39 jacob Exp $ */
/*
   
   xscorch - seconomy.h       Copyright(c) 2000 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Scorched economy
    

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
#ifndef __seconomy_h_included
#define __seconomy_h_included


#include <xscorch.h>


/* Forward structure definitions */
struct _sc_registry;
struct _sc_config;


/* Basic economical constants */
#define  SC_ECONOMY_MAX_CASH        1000000     /* 1mil dollars */
#define  SC_ECONOMY_DEF_CASH        100000      /* Start as broke */
#define  SC_ECONOMY_DEF_INTEREST    0.05        /* Default interest rate (percent) */
#define  SC_ECONOMY_MAX_INTEREST    0.30        /* Maximum interest rate */

/* Markups (values > 1) and markdowns (values < 1) */
#define  SC_ECONOMY_SPLIT_MARKUP    1.30        /* Percent markup if a bundle is split */
#define  SC_ECONOMY_SELL_MARKUP     0.50        /* Percent markup to sell (<1, markdown) */

#define  SC_ECONOMY_MAX_NAME_LEN    30          /* Longest allowed economy name */
#define  SC_ECONOMY_MAX_DESC_LEN    80          /* Longest allowed economy description */


/* Data on the various types of economy scorings */
typedef struct _sc_scoring_info {
   int ident;                    /* Unique ID (none if <= 0) */
   int survivalbonus;            /* Amount of money recv'd for survival */
   int damagebonus;              /* Amount of money recv'd for damaging */
   int killbonus;                /* Bonus for killing an opponent */
   int damageloss;               /* Amount of money lost for sustaining damage */
   int deathloss;                /* Amount of money lost for dying */
   int suicideloss;              /* Amount of money lost for killing self */
   bool fixed;                   /* Absolute or multiply by initialcash/100000 */
   char *name;                   /* Scoring name */
   char *description;            /* Scoring information */
} sc_scoring_info;


/* Economy configuration */
typedef struct _sc_economy_config {
   /* Various economics information */
   struct _sc_registry *registry;/* The game data registry */
   double interestrate;          /* Current interest rate (%/round) */
   bool dynamicinterest;         /* Nonzero if interest rate can change */
   double currentinterest;       /* Current interest rate */
   int initialcash;              /* Initial cash for all players */
   int registryclass;            /* Registry class of economy scorings */
   bool computersbuy;            /* Nonzero if computers can buy */
   bool computersaggressive;     /* Computers are agressive with money? */
   bool freemarket;              /* Nonzero if simulating a free mkt */
   bool lottery;                 /* The Scorched Lotto! */
   char scoringname[SC_ECONOMY_MAX_NAME_LEN]; /* Current economy */

   /* Run-time values (based on config->current) */
   int survivalbonus;            /* Amount of money recv'd for survival */
   int damagebonus;              /* Amount of money recv'd for damaging */
   int killbonus;                /* Bonus for killing an opponent */
   int damageloss;               /* Amt of money lost for sustaining damage */
   int deathloss;                /* Amt of money lost for dying */
   int suicideloss;              /* Amt of money lost for killing self */
} sc_economy_config;


/* Create and destroy economy conf storage */
sc_economy_config *sc_economy_config_create(struct _sc_config *c);
void sc_economy_config_destroy(sc_economy_config **ec);
void sc_scoring_info_free(sc_scoring_info **info);


/* Periodic maintenance on the economics. :) */
void sc_economy_init(sc_economy_config *ec);
void sc_economy_interest(struct _sc_config *c, sc_economy_config *ec);


/* Find economy scorings. */
sc_scoring_info *sc_scoring_lookup(const sc_economy_config *ec, int id);
sc_scoring_info *sc_scoring_lookup_by_name(const sc_economy_config *ec, const char *name);


/*
 * TEMP OBSOLETE? -JL
 * (__BUTTERFLIES_HAVE_STOLEN_YOUR_FAVORITE_CAR__)
 * It might work well to bring these back, actually.
 * I'm thinking about it; seconomy-gtk.c is *so* ugly right now...
 * const char **sc_economy_scoring_names(void);
 * const int *sc_economy_scoring_types(void);
 */


#endif /* __seconomy_h_included */
