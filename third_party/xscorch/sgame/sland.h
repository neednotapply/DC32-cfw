/* $Header: /fridge/cvs/xscorch/sgame/sland.h,v 1.7 2009-04-26 17:39:41 jacob Exp $ */
/*
   
   xscorch - sland.h          Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Scorched Player land generation
    

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
#ifndef __sland_h_included
#define __sland_h_included


/* Includes */
#include <xscorch.h>


/* Forward declarations */
struct _sc_config;
struct _sc_player;


/* Maximum drop amounts */
#define  SC_LAND_MAX_AMOUNT            48    /* Maximum amount of ground to drop */
#define  SC_LAND_MAX_DROP_PER_CYCLE    48    /* Maximum distance to drop, /cycle */


/* Maximum values */
#define  SC_LAND_WIND_MAX        1.00  /* Maximum allowed wind speed */
#define  SC_LAND_BUMPINESS_MAX   100.0 /* Maximum bumpiness allowed */
#define  SC_LAND_SLOPE_MAX       100.0 /* Maximum slope allowed */


/* Default landscape values */
#define  SC_LAND_SMOOTHNESS_DEF  90    /* Probability land remains smooth (percent) */
#define  SC_LAND_BUMPINESS_DEF   50    /* Default bumpiness */
#define  SC_LAND_SLOPE_DEF       5.0   /* Default maximum slope */


/* Land manipulation masks */
#define  SC_LAND_COLOR_MASK      0x0000ff
#define  SC_LAND_TYPE_MASK       0xffff00


/* Land types */
#define  SC_LAND_NIGHT_SKY       0x000100
#define  SC_LAND_FIRE_SKY        0x000200
#define  SC_LAND_GROUND          0x010000
#define  SC_LAND_OBJECT          0x020000
#define  SC_LAND_SMOKE           0x100000
#define  SC_LAND_BURNT           0x200000


/* Land macros */
#define  SC_LAND_GET_COLOR(x)    ((x) & SC_LAND_COLOR_MASK)
#define  SC_LAND_GET_TYPE(x)     ((x) & SC_LAND_TYPE_MASK)

#define  SC_LAND_IS_SKY(x)       (SC_LAND_GET_TYPE(x) & (SC_LAND_NIGHT_SKY | SC_LAND_FIRE_SKY | SC_LAND_SMOKE))
#define  SC_LAND_IS_GROUND(x)    (SC_LAND_GET_TYPE(x) & SC_LAND_GROUND)
#define  SC_LAND_IS_OBJECT(x)    (SC_LAND_GET_TYPE(x) & SC_LAND_OBJECT)
#define  SC_LAND_IS_SMOKE(x)     (SC_LAND_GET_TYPE(x) & SC_LAND_SMOKE)
#define  SC_LAND_IS_BURNT(x)     (SC_LAND_GET_TYPE(x) & SC_LAND_BURNT)


/* Landscape sky types */
typedef enum _sc_land_sky {
   SC_SKY_NIGHT,
   SC_SKY_FIRE,
   SC_SKY_RANDOM,
} sc_land_sky;


/* Landscape generators */
typedef enum _sc_land_generator {
   SC_LAND_GEN_NONE,
   SC_LAND_GEN_CANYON,
   SC_LAND_GEN_DOUBLE_MOUNTAIN,
   SC_LAND_GEN_HILLSIDE,
   SC_LAND_GEN_MOUNTAIN,
   SC_LAND_GEN_PLAINS,
   SC_LAND_GEN_TRADITIONAL,
   SC_LAND_GEN_VALLEY,
   SC_LAND_GEN_RANDOM
} sc_land_generator;



/* Land flags */
#define  SC_LAND_DEFAULTS  0x0000   /* Equivalent to no walls */
#define  SC_LAND_WRAP_X    0x0001   /* If set, X boundaries wrap */
#define  SC_LAND_WALL_X    0x0002   /* If set, X boundaries are solid */
#define  SC_LAND_CEILING   0x0100   /* If set, there is a ceiling */



/* Land data structure */
typedef struct _sc_land {
   /* Land width and height */
   /* WARNING: the master variables are in sc_config */
   int width;              /* Land width (pixels) */
   int height;             /* Land height (pixels) */
   int flags;              /* Land boundary flags */

   /* General land options */
   sc_land_generator generator;  /* Current land profile generator */
   sc_land_sky sky;        /* Current sky type */
   sc_land_sky realsky;    /* Current sky (- random) */
   bool hostileenv;        /* Nonzero if enable hostile environments */
   double bumpiness;       /* Bumpiness of land (max dY) */

   /*** WARNING
         The screen structure is organised differently than libmtx uses!
         Columns are contiguous in memory.  Insane?  Hell yes.  Pretty 
         much all of the 1-D loops in the game will be iterating along a 
         vertical vector -- this is for optimization purposes (an inc is
         cheaper than adding a variable).  If in doubt (and ESPECIALLY 
         if not in a tight loop), use the macros provided  ***/
   /*** Origin of the screen is lower-left ***/
   int *screen;
} sc_land;

/* Accessors for land */
#define  SC_LAND_XY_TO_I(l,x,y)  ((x) * (l)->height + (y))
#define  SC_LAND_I_TO_X(l,i)     ((i) / (l)->height)
#define  SC_LAND_I_TO_Y(l,i)     ((i) % (l)->height)
#define  SC_LAND_ORIGIN(l)       ((l)->screen)
#define  SC_LAND_XY(l,x,y)       (SC_LAND_ORIGIN(l) + SC_LAND_XY_TO_I((l), (x), (y)))


/* Create and generate land */
sc_land *sc_land_new(int width, int height, int flags);
bool     sc_land_setup(sc_land *l, int width, int height, int flags);
int      sc_land_flags(const struct _sc_config *c);
bool     sc_land_generate(const struct _sc_config *c, sc_land *l);
void     sc_land_free(sc_land **l);


/* Land Height calculations */
int      sc_land_height(const sc_land *l, int x, int y0);
int      sc_land_height_around(const sc_land *l, int x, int y0, int w);
int      sc_land_min_height_around(const sc_land *l, int x, int y0, int w);
int      sc_land_avg_height_around(const sc_land *l, int x, int y0, int w);


/* Land drill & fill operations */
void     sc_land_level_around(const struct _sc_config *c, sc_land *l, int x, int w, int ht);


/* Check the land support for a tank; 
   return 0 if tank is properly supported.  */
int      sc_land_support(const struct _sc_config *c, const sc_land *l, int x, int y, int r, int s);


/* Drop the land in a gravity field */
bool     sc_land_drop(const struct _sc_config *c, sc_land *l, int cx, int w);
bool     sc_land_drop_zone(const struct _sc_config *c, sc_land *l, int x1, int x2);


/* Sky cleaning */
void     sc_land_clear_smoke(const struct _sc_config *c, sc_land *l);


/* Line-of-sight and trajectory functions */
bool     sc_land_line_of_sight(const struct _sc_config *c, const sc_land *l, int x1, int y1, int x2, int y2);


/* Coordinate validation */
bool     sc_land_translate_x(const sc_land *l, int *x);
bool     sc_land_translate_y(const sc_land *l, int *y);
bool     sc_land_translate_xy(const sc_land *l, int *x, int *y);
bool     sc_land_translate_x_range(const sc_land *l, int *x1, int *x2);
bool     sc_land_overlap_x(const sc_land *l, int *x1, int *x2);
bool     sc_land_calculate_deltas(const sc_land *l, int *dx, int *dy, int x1, int y1, int x2, int y2);


/* Coordinate validation - double versions */
bool     sc_land_translate_x_d(const sc_land *l, double *x);
bool     sc_land_translate_y_d(const sc_land *l, double *y);
bool     sc_land_translate_xy_d(const sc_land *l, double *x, double *y);
bool     sc_land_calculate_deltas_d(const sc_land *l, double *dx, double *dy, double x1, double y1, double x2, double y2);


/* Misc */
const int *sc_land_sky_index(const struct _sc_config *c);
int      sc_land_sky_flag(const struct _sc_config *c);
void     sc_land_create_dirt(const struct _sc_config *c, sc_land *l, const int *xlist, const int *ylist, int size);
bool     sc_land_passable_point(const struct _sc_config *c, const struct _sc_player *p, int x, int y);
void     sc_land_clear_profile(const struct _sc_config *c, sc_land *l, const struct _sc_player *p);


/* Names and games ... */
const char **sc_land_sky_names(void);
const unsigned int *sc_land_sky_types(void);
const char **sc_land_generator_names(void);
const unsigned int *sc_land_generator_types(void);


#endif /* __sland_h_included */
