/* $Header: /fridge/cvs/xscorch/sgame/sphysics.h,v 1.30 2009-05-25 04:43:22 jacob Exp $ */
/*

   xscorch - sphysics.h       Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   Scorched physics


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
#ifndef __sphysics_h_included
#define __sphysics_h_included


#include <xscorch.h>


/* Debugging control */
#define  SC_PHYSICS_DEBUG_ROLLER 1        /* Set to 1 to debug rollers */


/* Forward structure definitions */
struct _sc_config;
struct _sc_player;

/* Physics definitions (see below for gravity, velocity calculations) */
#define  SC_PHYSICS_BORDERS_DEF  2500     /* Default dist to simulate */
#define  SC_PHYSICS_BORDERS_MAX  10000    /* Max dist to simulate fm screen */
#define  SC_PHYSICS_GRAVITY_DEF  0.500    /* Default gravity */
#define  SC_PHYSICS_GRAVITY_MAX  10.0     /* Maximum gravity */
#define  SC_PHYSICS_VELOCITY_SCL 0.027    /* Velocity scaling */
#define  SC_PHYSICS_VISCOUS_DEF  0        /* Default air viscosity */
#define  SC_PHYSICS_VISCOUS_MAX  0.200    /* Maximum air viscosity */
#define  SC_PHYSICS_WIND_DEF     0.05     /* Default max wind */
#define  SC_PHYSICS_WIND_MAX     10.0     /* Maximum wind vel. (pix/step) */
#define  SC_PHYSICS_DELTA_WIND_MAX  0.2   /* Maximum change (% of max wind) */


/* Wall effects */
typedef enum _sc_physics_walls {
   SC_WALL_CONCRETE,    /* Solid, all dirs (weapons hitting will explode) */
   SC_WALL_PADDED,      /* Shots reflected, but some energy is absorbed */
   SC_WALL_RUBBER,      /* Shots reflected */
   SC_WALL_SPRING,      /* Shots reflected, but with an additional "kick" */
   SC_WALL_WRAP,        /* Boundaries wrap horizonally; top is open */
   SC_WALL_NONE,        /* Weapons go off screen horiz; top is open */
   SC_WALL_RANDOM       /* Walls are selected at random */
} sc_physics_walls;


/* Elasticity of certain walls */
#define  SC_PHYSICS_ELASTIC_PADDED  0.5   /* Some velocity lost */
#define  SC_PHYSICS_ELASTIC_RUBBER  1.0   /* All velocity is reflected */
#define  SC_PHYSICS_ELASTIC_SPRING  1.5   /* Give weapon a little kick */


/* Trajectory type */
typedef enum _sc_trajectory_type {
   SC_TRAJ_FLIGHT,      /* Trajectory is airborne, and following physics */
   SC_TRAJ_ROLLER,      /* Trajectory is rolling on the ground somewhere */
   SC_TRAJ_DIGGER       /* Trajectory is deliberately tunnelling in soil */
} sc_trajectory_type;


/* Identify what flight mode a trajectory is in. */
#define SC_TRAJ_IS_FLYING(tr)    ((tr)->type == SC_TRAJ_FLIGHT)
#define SC_TRAJ_IS_ROLLING(tr)   ((tr)->type == SC_TRAJ_ROLLER)
#define SC_TRAJ_IS_DIGGING(tr)   ((tr)->type == SC_TRAJ_DIGGER)


/* Rollers use these values in energy/hill calculations.  ROLLER_FALL
   is an additive gain in the speed of the roller when it falls down.
   ROLLER_GAIN determines how much momentum is *lost* if the roller
   is forced to ascend.  */
#define SC_TRAJ_ROLLER_FALL   0.5         /* Added to momentum on fall */
#define SC_TRAJ_ROLLER_GAIN   1.3         /* Removed from mom. on ascent */


/* A physical parabola.  The impact cases usually result in detonation
   of the associated weapon, but may imply reflection or other special
   processing in certain cases --- e.g. rollers will attempt to roll
   uphill on receiving IMPACT_LAND, instead of detonating.  When a
   function returns an IMPACT code, usually the curx,cury position
   reflects the point of impact, and the lastx,lasty position reflects
   the last position that was "clear".  */
typedef enum _sc_trajectory_result {
   SC_TRAJ_CONTINUE,       /* The trajectory has not terminated. */
   SC_TRAJ_IMPACT_TANK,    /* The trajectory impacted a tank */
   SC_TRAJ_IMPACT_LAND,    /* The trajectory impacted land */
   SC_TRAJ_IMPACT_WALL,    /* The trajectory hit a vertical wall or ceiling */
   SC_TRAJ_IMPACT_GROUND,  /* The trajectory hit the ground */
   SC_TRAJ_IMPACT_SHIELD,  /* The trajectory hit a shield */
   SC_TRAJ_SIZZLE          /* Timed out/faded without a detonate */
} sc_trajectory_result;
#define  SC_TRAJ_IS_IMPACT(n)    ((n) == SC_TRAJ_IMPACT_TANK || \
                                  (n) == SC_TRAJ_IMPACT_LAND || \
                                  (n) == SC_TRAJ_IMPACT_WALL || \
                                  (n) == SC_TRAJ_IMPACT_GROUND || \
                                  (n) == SC_TRAJ_IMPACT_SHIELD)

/* WARNING:  You must be aware of the "standard" units in these
   calculations.  Timesteps are _timesteps_... they are not seconds.
   Location is in pixel units.  The trajectory routines will do no
   unit conversion; you must call them with the standard units!  */
/* Note:
   ROLLERS store ground-tangential velocity in velx and leave vely 0.
   ROLLERS do not maintain an acceleration term and are immune to the
   effects of air viscosity. */
typedef struct _sc_trajectory {
   /* Type flag */
   sc_trajectory_type type;      /* What type of trajectory is it? */

   /* Internal params -- DO NOT EDIT directly.  These parametres
      control the trajectory for flight-based models.  The current
      intended position is projected using these constants which
      define the trajectory.  */
   /*   cur_x = ctr_x + vel_x t + 1/2 acc_x t^2   */
   /*   cur_y = ctr_y + vel_y t + 1/2 acc_y t^2   */
   double ctrx;                  /* Center X coordinate */
   double ctry;                  /* Center Y coordinate */
   double accx;                  /* Acceleration X */
   double accy;                  /* Acceleration Y */
   double velx;                  /* Initial velocity of X */
   double vely;                  /* Initial velocity of Y */

   /* Current state values -- DO NOT EDIT directly.  If victim
      is negative, then the trajectory has not directly impacted
      with a particular player; if the trajectory has impacted
      with someone, then victim is set to the appropriate player
      ID.   */
   int victim;                   /* Victim in trajectory path */
   double timestep;              /* Current timestep */
   double stopstep;              /* Step we "timeout" at */
   double finalstep;             /* Stop timestep for this step */
   double curx;                  /* Current X coordinate */
   double cury;                  /* Current Y coordinate */
   double lastx;                 /* X coordinate, prev PIXEL */
   double lasty;                 /* Y coordinate, prev PIXEL */
   double oldx;                  /* X coordinate, prev step */
   double oldy;                  /* Y coordinate, prev step */
   double oaccx;                 /* Acceleration X (originally) */
   double oaccy;                 /* Acceleration Y (originally) */
   int dir;                      /* Current direction (diggers) */

   /* The following parametres are used to recover
      the original control equation in event of a
      reinitialize call, for flight-based trajectories. */
   double _ictrx;                /* Center X coordinate */
   double _ictry;                /* Center Y coordinate */
   double _iaccx;                /* Acceleration X */
   double _iaccy;                /* Acceleration Y */
   double _ivelx;                /* Initial velocity of X */
   double _ively;                /* Initial velocity of Y */

   /* The following two values control landfall for a
      trajectory (when the trajectory has erased some land).
      This is used to determine what sections of screen may
      need to be redrawn.  */
   int landfall_x1;
   int landfall_x2;

   /* Who owns the trajectory (if anyone, else NULL)? */
   const struct _sc_player *player;
} sc_trajectory;

/* Action hook for the path tracing function */
typedef sc_trajectory_result (*sc_trajectory_action)(struct _sc_config *c,
                                                     sc_trajectory *tr,
                                                     void *data);


/* Trajectory modelling constants */
#define  SC_TRAJ_TIME_STEP       1.0    /* Take this many timesteps per step */
#define  SC_TRAJ_TIME_SUBSTEP    16     /* Take this many substeps per step */
#define  SC_TRAJ_TIMEOUT         240    /* Number of timesteps before timeout */
#define  SC_TRAJ_THRESHOLD       16     /* Threshold in certain estimation code. */
#define  SC_TRAJ_TUNNEL_DAMPING  0.8    /* Damping for tunneling weapons */
#define  SC_TRAJ_DAMPING_MAX     1.0    /* Maximum legal value for damping (duh) */
#define  SC_TRAJ_TUNNEL_MIN_VEL  0.5    /* Min velocity to tunnel - pixel/timeslice */
#define  SC_TRAJ_DIGGER_PROB     15     /* Prob. that digger direction will change */


/* Trajectory modelling flags */
#define  SC_TRAJ_DEFAULTS     0         /* Defaults in trajectory computation */
#define  SC_TRAJ_HIT_SHIELDS  (1 << 0)  /* Must hit any shields we impact */
#define  SC_TRAJ_IGNORE_WIND  (1 << 1)  /* Ignore wind in trajectory computation */
#define  SC_TRAJ_IGNORE_VISC  (1 << 2)  /* Ignore viscosity in computations */
#define  SC_TRAJ_RESTRICTIVE  (1 << 3)  /* Restrict calculated traj to screen */
#define  SC_TRAJ_TERMINUS     (1 << 4)  /* Weapon must halt at specified dest */
#define  SC_TRAJ_TUNNELING    (1 << 5)  /* Weapon is tunneling if set. */
#define  SC_TRAJ_NO_MODIFY    (1 << 6)  /* Do not modify player or config data */
#define  SC_TRAJ_BRIEF        (1 << 7)  /* Run at lower max timestep */

/* These modifiers are specific to stepping */
#define  SC_TRAJ_IGNORE_TANK  (1 << 8)  /* Ignore tanks in stepping */
#define  SC_TRAJ_IGNORE_LAND  (1 << 9)  /* Ignore land in stepping */


/* Low-level trajectory timestep size */
#define  SC_TRAJ_TIME_STEPS_PER_SUBSTEP   (SC_TRAJ_TIME_STEP / SC_TRAJ_TIME_SUBSTEP)


/* Physics configuration */
typedef struct _sc_physics {
   double airviscosity;          /* Air viscosity (0==projectiles not slowed) */
   double gravity;               /* Gravity field (pixels/step/step) */
   int suspenddirt;              /* Probability that dirt remains suspended */
   int tanksfall;                /* Nonzero if tanks should fall */
   int bordersextend;            /* Distance out borders extend (simulation) */
   double damping;               /* Ground damping (for tunnelling) */
   double maxwind;               /* Maximum wind force */
   double curwind;               /* Current wind force */
   bool dynamicwind;             /* Wind is dynamic? */
   sc_physics_walls walls;       /* Effect of walls */
} sc_physics;


/* Physics modelling -- basic */
sc_physics *sc_physics_new(void);
void sc_physics_free(sc_physics **ph);
void sc_physics_init_game(sc_physics *ph);
void sc_physics_init_round(sc_physics *ph);
void sc_physics_update_wind(sc_physics *ph);


/* Trajectory setup - flight */
sc_trajectory *sc_traj_new_velocities( const struct _sc_config *c,
                                       const struct _sc_player *p,
                                       int flags,
                                       double cx, double cy,
                                       double velx, double vely);
sc_trajectory *sc_traj_new_power_angle(const struct _sc_config *c,
                                       const struct _sc_player *p,
                                       int flags,
                                       double cx, double cy,
                                       double power, double angle);
sc_trajectory *sc_traj_new_dest_angle( const struct _sc_config *c,
                                       const struct _sc_player *p,
                                       int flags,
                                       double cx, double cy,
                                       double deltax, double deltay,
                                       double angle, double maxpower);
sc_trajectory *sc_traj_new_dest_height(const struct _sc_config *c,
                                       const struct _sc_player *p,
                                       int flags,
                                       double cx, double cy,
                                       double deltax, double deltay,
                                       double height);
void sc_traj_landfall(struct _sc_config *c, const sc_trajectory *tr);
void sc_traj_free(sc_trajectory **tr);


/* Conversion between traj types? */
bool sc_trajectory_convert(const struct _sc_config *c, sc_trajectory *tr,
                           int flags, sc_trajectory_type type);


/* Trajectory calculations */
void sc_traj_reinitialize(sc_trajectory *tr);
sc_trajectory_result sc_traj_step(struct _sc_config *c,
                                  sc_trajectory *tr, int flags,
                                  sc_trajectory_action action, void *data);
sc_trajectory_result sc_traj_run( struct _sc_config *c,
                                  sc_trajectory *tr, int flags,
                                  sc_trajectory_action action, void *data);
void sc_traj_set_velocity(sc_trajectory *tr, double velx, double vely);
void sc_traj_revise_velocity(sc_trajectory *tr, double velx, double vely);
void sc_traj_revise_acceleration(sc_trajectory *tr, double accx, double accy);
void sc_traj_restore_acceleration(sc_trajectory *tr);
void sc_traj_new_control_equation(sc_trajectory *tr, double ctrx, double ctry,
                                  double velx, double vely);
double sc_traj_get_velocity_x(const sc_trajectory *tr);
double sc_traj_get_velocity_y(const sc_trajectory *tr);
double sc_traj_get_current_x(const sc_trajectory *tr);
double sc_traj_get_current_y(const sc_trajectory *tr);
double sc_traj_height(const sc_trajectory *tr);


/* Named parametres */
const char **sc_physics_wall_names(void);
const unsigned int *sc_physics_wall_types(void);



#endif /* __sphysics_h_included */

