/* $Header: /fridge/cvs/xscorch/sgame/sexplosion.h,v 1.7 2009-04-26 17:39:39 jacob Exp $ */
/*

   xscorch - sexplosion.h     Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Scorched zone explosions


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
#ifndef __sexplosion_h_included
#define __sexplosion_h_included


#include <xscorch.h>


/* Forward declarations */
struct _sc_config;
struct _sc_land;


/* Explosion type characteristics */
typedef enum _sc_explosion_type {
   SC_EXPLOSION_NORMAL,          /* Normal red explosion */
   SC_EXPLOSION_SPIDER,          /* Funky bomb explosions */
   SC_EXPLOSION_PLASMA,          /* Plasma-class explosion */
   SC_EXPLOSION_NAPALM,          /* Napalm ONLY */
   SC_EXPLOSION_LIQ_DIRT,        /* Liquid dirt */
   SC_EXPLOSION_DIRT,            /* Flying Dirt clod */
   SC_EXPLOSION_RIOT             /* Riot bomb deton */
} sc_explosion_type;


/* Explosion characteristics */
typedef struct _sc_explosion {
   int centerx;      /* Center X coordinate */
   int centery;      /* Center Y coordinate */
   int radius;       /* Radius of explosion */
   int force;        /* Strength of explosion */
   int playerid;     /* Which player caused explosion? */
   double direction; /* Center of wedge shaped blast (radians, 0 = right, M_PI/2 = up) */
   double angular_width;         /* Angular width of wedge shaped blast (radians).
                                    If 0, the normal circular explosions are used. */
   sc_explosion_type type;       /* Explosion type */
   void *data;                   /* Associated state data */
   int idraw;                    /* Reserved state data - explosion drawing */
   struct _sc_expl_cache *cache; /* Cache for "growing" explosions */
   struct _sc_explosion *chain;  /* Chain to next explosion */
   unsigned long counter;        /* Used by state machine to animate */
} sc_explosion;


/* Explosion constants */
#define  SC_EXPL_NAPALM_FLAMES   16       /* Number of flame anims to draw */
#define  SC_EXPL_LIQUID_STEP     32       /* Amt of napalm to draw per step */
#define  SC_EXPL_EXPLOSION_STEP  24       /* Stepping on radius of explosion */
#define  SC_EXPL_DEFAULT_DIR     (M_PI / 2)  /* Default direction of explosion */
#define  SC_EXPL_DEBUG_WEDGES    0        /* Set to 1 to debug explosion wedges */


/* Explosion creation and freeing */
sc_explosion *sc_expl_new(int centerx, int centery, int radius, int force,
                          int playerid, sc_explosion_type type);
sc_explosion *sc_expl_new_with_angle(int centerx, int centery, int radius, int force,
                                     double direction, double angular_width,
                                     int playerid, sc_explosion_type type);
sc_explosion *sc_expl_add(sc_explosion **e, sc_explosion *add);
sc_explosion *sc_expl_index(sc_explosion *e, int index);
int  sc_expl_count(const sc_explosion *e);
void sc_expl_free(sc_explosion **e);
void sc_expl_free_chain(sc_explosion **e);


/* Explosions and related damages */
bool sc_expl_annihilate(struct _sc_config *c, sc_explosion *e);
bool sc_expl_annihilate_continue(struct _sc_config *c, sc_explosion *e);
bool sc_expl_annihilate_clear(struct _sc_config *c, sc_explosion *e);
bool sc_expl_annihilate_clear_continue(struct _sc_config *c, sc_explosion *e);
int  sc_expl_damage_at_point(const struct _sc_land *l, const sc_explosion *e, int x, int y);
sc_explosion *sc_expl_spider(struct _sc_config *c, const sc_explosion *e);


#endif /* __sexplosion_h_included */
