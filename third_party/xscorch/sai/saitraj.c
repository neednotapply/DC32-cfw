/* $Header: /fridge/cvs/xscorch/sai/saitraj.c,v 1.17 2009-04-26 17:39:35 jacob Exp $ */
/*

   xscorch - saitraj.c        Copyright(c) 2000-2003 Justin David Smith
   justins(at)chaos2.org      http://chaos2.org/

   AI turn code: specific functions to calculate, refine trajectory


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
#include <assert.h>

#include <saiint.h>              /* Main internal AI */

#include <sgame/sconfig.h>       /* Need config struct */
#include <sgame/sland.h>         /* We work with land/validate */
#include <sgame/sphysics.h>      /* Need trajectories */
#include <sgame/splayer.h>       /* Need player structure */
#include <sgame/stankpro.h>      /* Need tank/shield radius */
#include <sgame/sweapon.h>       /* Need tunneling data */



/* TURN BACK NOW */



static inline void _sc_ai_trajectory_compensation(const sc_config *c, const sc_player *p, 
                                                  const sc_player *victim, int *deltax, int *deltay) {
/* sc_ai_trajectory_compensation 
   Compensation for shields, if offset targetting is allowed.
   The deltax, deltay values indicate the current position being
   targeted; they may be revised to indicate a new coordinate to
   target if offset targetting is used.  */

   assert(c != NULL && p != NULL && victim != NULL);
   assert(deltax != NULL && deltay != NULL);
   assert(SC_PLAYER_IS_ALIVE(victim));
   assert(p->index != victim->index);

   if(SC_AI_WILL_OFFSET(c, victim)) {
      /* deltax is always along the shortest path, so we will choose
         to compensate even _closer_, i.e. bring the target nearer
         to us in all possible cases.  If they are on a peak, this
         way instead of overshooting the mountain we take out the
         mountain >:) */
      *deltax -= (SC_AI_COMPENSATE_DELTA + victim->tank->radius) * SGN(*deltax); 
   }

}



static int _sc_ai_trajectory_angle_bounds(int deltax, int deltay, int maxdelta) {
/* sc_ai_trajectory_angle_bounds
   Calculate the maximum permitted delta angle, for scanning for an optimal
   angle to fire at.  maxdelta is the maximum delta angle permitted from the
   vertical, to either side.  This function is also aware that angle cannot
   go below the vector angle from us to the victim.  The value returned is
   the maximum delta from the vertical that angle may take on, and the
   return value <= maxdelta.  */

   double vectorangle;/*Vector angle from player to (dx,dy) */
   int delta;        /* Maximum angle we should take on, here. */

   /* Calculate the vector angle. */
   vectorangle = atan2(deltay, deltax) * 180 / M_PI;

   /* Start at the steepest angle allowed, and go down until we find a
      suitable trajectory.  Note, we only have to check angles from the
      vertical, down to vectorangle; the trajectory cannot be below the
      vector angle, due to gravity.  */
   if(deltax >= 0) {
      /* We are firing to the right. */
      if(vectorangle < 0) vectorangle = 0;
      delta = 90 - ceil(vectorangle);
   } else {
      /* We are firing to the left. */
      if(vectorangle < 0) vectorangle = 180;
      delta = floor(vectorangle) - 90;
   } /* Which way are we firing? */

   /* The AI also imposes certain max angles, as an extra sanity check. */
   if(delta > maxdelta) delta = maxdelta;
   return(delta);

}



static bool _sc_ai_validate_angle(const sc_config *c, sc_player *p, int angle) {
/* sc_ai_validate_angle
   Checks to see if there is a line of sight at least SC_AI_VALIDATE_DIST
   pixels away from the AI tank in the specified angle.  This is to prevent
   the AI from firing directly into an adjacent cliff, causing the AI to
   get absorbed into its own explosion.  Returns true if the path appears
   to be clear along the angle, and false otherwise.  This is HEURISTIC!  */

   double dx;
   double dy;
   double x;
   double y;
   int step;
   int cx;
   int cy;
   
   assert(c != NULL && p != NULL);

   dx = SC_AI_VALIDATE_DIST * cos(angle * M_PI / 180);
   dy = SC_AI_VALIDATE_DIST * sin(angle * M_PI / 180);

   if(fabs(dx) > fabs(dy)) {
      step = ceil(fabs(dx));
   } else {
      step = ceil(fabs(dy));
   }

   x = sc_player_turret_x(p, angle);
   y = sc_player_turret_y(p, angle);
   dx /= step;
   dy /= step;

   while(step > 0) {
      cx = rint(x);
      cy = rint(y);
      if(sc_land_translate_xy(c->land, &cx, &cy)) {
         if(!SC_LAND_IS_SKY(*SC_LAND_XY(c->land, cx, cy))) {
            return(false);
         }
      }
      x += dx;
      y += dy;
      --step;
   }

   return(true);

}



bool sc_ai_trajectory(const sc_config *c, sc_player *p, const sc_player *victim) {
/* sc_ai_trajectory
   Calculates trajectory to the specified coordinates, taking into account
   only the gravity.  This function assumes we do not have a direct line of
   sight to (dx,dy); therefore, we must assume there is a tall mountain in
   the way, and find the steepest trajectory that is within our power
   limitations.  This function returns true if a suitable trajectory was
   found, and false if the target is "unreachable".  SHOOTER's use this
   mechanism primarily.  */

   sc_trajectory *t; /* Trajectory (temp variable) */
   int deltax;       /* Change in X, from player's position */
   int deltay;       /* Change in Y, from player's position */
   int angle;        /* Current angle we are trying. */
   int power;        /* Power required to reach (dx,dy) */
   int maxdelta;     /* Maximum delta scanning angle */

   assert(c != NULL && p != NULL && victim != NULL);
   assert(SC_PLAYER_IS_ALIVE(victim));
   assert(p->index != victim->index);

   /* Determine distance & angle between player and (dx,dy) */   
   if(!sc_land_calculate_deltas(c->land, &deltax, &deltay, p->x, p->y, victim->x, victim->y)) {
      deltax = victim->x - p->x;
      deltay = victim->y - p->y;
   }

   /* Compensation for shields */
   _sc_ai_trajectory_compensation(c, p, victim, &deltax, &deltay);

   /* Start at the steepest angle allowed, and go down until we find a
      suitable trajectory.  Note, we only have to check angles from the
      vertical, down to vectorangle; the trajectory cannot be below the
      vector angle, due to gravity.  */
   angle = 90 - SGN(deltax) * SC_AI_SHOOTER_START_A;
   maxdelta = _sc_ai_trajectory_angle_bounds(deltax, deltay, SC_AI_SHOOTER_DELTA_A);

   /* Scan for the steepest angle that gets us the trajectory we need. */
   while(abs(angle - 90) <= maxdelta) {
      if(_sc_ai_validate_angle(c, p, angle)) {
         t = sc_traj_new_dest_angle(c, p, SC_TRAJ_IGNORE_WIND | SC_TRAJ_IGNORE_VISC, 
                                    sc_player_turret_x(p, angle), sc_player_turret_y(p, angle), deltax, deltay,
                                    angle, SC_PLAYER_MAX_POWER * SC_PHYSICS_VELOCITY_SCL);
         if(t != NULL) {
            /* Found a suitable power level! */
            power = DIST(t->velx, t->vely) / SC_PHYSICS_VELOCITY_SCL;
            sc_traj_free(&t);
            sc_player_set_power(c, p, power);
            sc_player_set_turret(c, p, angle);
            p->ai->victim = victim;
            if(SC_AI_DEBUG_TRAJECTORY) {
               printf("AI_trajectory:   %s, %s no-line to %d, %d with power=%d, angle=%d\n", 
                      sc_ai_name(p->aitype), p->name, deltax, deltay, power, angle);
            }
            return(true);
         } /* Found a trajectory? */
      } /* Is the angle valid? */

      /* Try the next angle down... */
      angle -= SGN(deltax);
   }

   /* No suitable trajectory was found. */
   return(false);

}



bool sc_ai_trajectory_line(const sc_config *c, sc_player *p, const sc_player *victim) {
/* sc_ai_trajectory_line
   Calculates trajectory to the specified coordinates, taking into account
   only the gravity.  This function assumes we do have a direct line of
   sight to (dx,dy); therefore, we can choose any trajectory we like.  Let's
   opt for the trajectory requiring the least amount of power.  */

   sc_trajectory *t; /* Trajectory (temp variable) */
   int deltax;       /* Change in X, from player's position */
   int deltay;       /* Change in Y, from player's position */
   int angle;        /* Current angle we are trying. */
   int power;        /* Power required to reach (dx,dy) */
   int maxdelta;     /* Maximum delta scanning angle */
   int powermin;     /* Best power value found. */
   int anglemin;     /* Best angle found so far */

   assert(c != NULL && p != NULL && victim != NULL);
   assert(SC_PLAYER_IS_ALIVE(victim));
   assert(p->index != victim->index);

   /* Determine distance & angle between player and (dx,dy) */   
   if(!sc_land_calculate_deltas(c->land, &deltax, &deltay, p->x, p->y, victim->x, victim->y)) {
      deltax = victim->x - p->x;
      deltay = victim->y - p->y;
   }

   /* Compensation for shields */
   _sc_ai_trajectory_compensation(c, p, victim, &deltax, &deltay);

   /* Start at the steepest angle allowed, and go down until we find a
      suitable trajectory.  Note, we only have to check angles from the
      vertical, down to vectorangle; the trajectory cannot be below the
      vector angle, due to gravity.  */
   angle = 90 - SGN(deltax) * SC_AI_SHOOTER_START_A;
   maxdelta = _sc_ai_trajectory_angle_bounds(deltax, deltay, SC_AI_SHOOTER_SIGHT_A);

   /* Try to find a trajectory consuming the least amount of power. */   
   anglemin = 90;
   powermin = SC_PLAYER_MAX_POWER + 1;
   while(abs(angle - 90) <= maxdelta) {
      if(_sc_ai_validate_angle(c, p, angle)) {
         t = sc_traj_new_dest_angle(c, p, SC_TRAJ_IGNORE_WIND | SC_TRAJ_IGNORE_VISC | SC_TRAJ_RESTRICTIVE, 
                                    sc_player_turret_x(p, angle), sc_player_turret_y(p, angle), deltax, deltay, 
                                    angle, SC_PLAYER_MAX_POWER * SC_PHYSICS_VELOCITY_SCL);
         if(t != NULL) {
            power = DIST(t->velx, t->vely) / SC_PHYSICS_VELOCITY_SCL;
            sc_traj_free(&t);
            if(power < powermin) {
               /* Found a better solution. */
               anglemin = angle;
               powermin = power;
            } /* Is this sol'n better? */
         } /* Found a valid trajectory */
      } /* Is the angle valid? */
      angle -= SGN(deltax);
   }

   /* Did we find an acceptable solution? */   
   if(powermin <= SC_PLAYER_MAX_POWER) {
      /* Yes; set new trajectory and power. */
      sc_player_set_power(c, p, powermin);
      sc_player_set_turret(c, p, anglemin);
      p->ai->victim = victim;
      if(SC_AI_DEBUG_TRAJECTORY) {
         printf("AI_trajectory:   %s, %s line to %d, %d with power=%d, angle=%d\n", 
                sc_ai_name(p->aitype), p->name, deltax, deltay, powermin, anglemin);
      }
      return(true);
   }

   /* No suitable trajectory found */
   return(false);

}



bool sc_ai_trajectory_wind(const sc_config *c, sc_player *p, const sc_player *victim) {
/* sc_ai_trajectory_wind
   Calculates trajectory to the specified coordinates, taking into account
   gravity and wind.  This function assumes we do not have a direct line of
   sight to (dx,dy); therefore, we must assume there is a tall mountain in
   the way, and find the steepest trajectory that is within our power
   limitations.  This function returns 1 if a suitable trajectory was found,
   and zero if the target is "unreachable".  CALCULATER's use this mechanism
   primarily.  */

   sc_trajectory *t; /* Trajectory (temp variable) */
   int deltax;       /* Change in X, from player's position */
   int deltay;       /* Change in Y, from player's position */
   int angle;        /* Current angle we are trying. */
   int power;        /* Power required to reach (dx,dy) */
   int height;       /* Height reached by this trajectory */
   int powermax;     /* Power which maximizes height */
   int anglemax;     /* Angle which maximizes height */
   int heightmax;    /* Maximum attained height. */
   int flags;        /* Default trajectory flags */

   assert(c != NULL && p != NULL && victim != NULL);
   assert(SC_PLAYER_IS_ALIVE(victim));
   assert(p->index != victim->index);

   /* Determine distance & angle between player and (dx,dy) */   
   if(!sc_land_calculate_deltas(c->land, &deltax, &deltay, p->x, p->y, victim->x, victim->y)) {
      deltax = victim->x - p->x;
      deltay = victim->y - p->y;
   }

   /* Compensation for shields */
   _sc_ai_trajectory_compensation(c, p, victim, &deltax, &deltay);

   /* Start at the steepest angle allowed, and go down until we find a
      suitable trajectory.  Note, we only have to check angles from the
      vertical, down to vectorangle; the trajectory cannot be below the
      vector angle, due to gravity.  */
   angle = 90 - SC_AI_SHOOTER_DELTA_A;

   /* Scan for the angle that gets us the highest trajectory we need. */
   heightmax= -1;
   powermax = SC_PLAYER_MAX_POWER + 1;
   anglemax = 0;
   while(angle <= 90 + SC_AI_SHOOTER_DELTA_A) {
      if(_sc_ai_validate_angle(c, p, angle)) {
         if(c->weapons->tunneling && !p->contacttriggers) flags = SC_TRAJ_TUNNELING;
         else flags = SC_TRAJ_DEFAULTS;
         t = sc_traj_new_dest_angle(c, p, flags, sc_player_turret_x(p, angle), sc_player_turret_y(p, angle), 
                                    deltax, deltay, angle, SC_PLAYER_MAX_POWER * SC_PHYSICS_VELOCITY_SCL);
         if(t != NULL) {
            power = DIST(t->velx, t->vely) / SC_PHYSICS_VELOCITY_SCL;
            height = sc_traj_height(t);
            sc_traj_free(&t);
            if(height > heightmax && power <= SC_PLAYER_MAX_POWER) {
               /* Found a new ideal trajectory */
               heightmax= height;
               powermax = power;
               anglemax = angle;
            } /* Better traj? */
         } /* Found a valid trajectory? */
      } /* Is the angle valid? */
   
      /* Try the next angle down... */
      ++angle;
   }

   if(powermax <= SC_PLAYER_MAX_POWER) {
      /* Found a suitable power level! */
      sc_player_set_power(c, p, powermax);
      sc_player_set_turret(c, p, anglemax);
      p->ai->victim = victim;
      if(SC_AI_DEBUG_TRAJECTORY) {
         printf("AI_trajectory:   %s, %s no-line,wind to %d, %d with power=%d, angle=%d\n",
                sc_ai_name(p->aitype), p->name, deltax, deltay, powermax, anglemax);
      }
      return(true);
   } /* Found a trajectory? */

   /* No suitable trajectory was found. */
   return(false);

}



bool sc_ai_trajectory_line_wind(const sc_config *c, sc_player *p, const sc_player *victim) {
/* sc_ai_trajectory_line_wind
   Calculates trajectory to the specified coordinates, taking into account
   gravity and wind.  This function assumes we do have a direct line of
   sight to (dx,dy); therefore, we can choose any trajectory we like.  Let's
   opt for the trajectory requiring the least amount of power.  */

   sc_trajectory *t; /* Trajectory (temp variable) */
   int deltax;       /* Change in X, from player's position */
   int deltay;       /* Change in Y, from player's position */
   int angle;        /* Current angle we are trying. */
   int power;        /* Power required to reach (dx,dy) */
   int powermin;     /* Best power value found. */
   int anglemin;     /* Best angle found so far */

   assert(c != NULL && p != NULL && victim != NULL);
   assert(SC_PLAYER_IS_ALIVE(victim));
   assert(p->index != victim->index);

   /* Determine distance & angle between player and (dx,dy) */   
   if(!sc_land_calculate_deltas(c->land, &deltax, &deltay, p->x, p->y, victim->x, victim->y)) {
      deltax = victim->x - p->x;
      deltay = victim->y - p->y;
   }

   /* Compensation for shields */
   _sc_ai_trajectory_compensation(c, p, victim, &deltax, &deltay);

   /* Start at the steepest angle allowed, and go down until we find a
      suitable trajectory.  Note, we only have to check angles from the
      vertical, down to vectorangle; the trajectory cannot be below the
      vector angle, due to gravity.  */
   angle = 90 - SC_AI_SHOOTER_SIGHT_A;

   /* Try to find a trajectory consuming the least amount of power. */   
   anglemin = 90;
   powermin = SC_PLAYER_MAX_POWER + 1;
   while(angle <= 90 + SC_AI_SHOOTER_SIGHT_A) {
      if(_sc_ai_validate_angle(c, p, angle)) {
         t = sc_traj_new_dest_angle(c, p, SC_TRAJ_RESTRICTIVE, 
                                    sc_player_turret_x(p, angle), sc_player_turret_y(p, angle), 
                                    deltax, deltay, angle, SC_PLAYER_MAX_POWER * SC_PHYSICS_VELOCITY_SCL);
         if(t != NULL) {
            power = DIST(t->velx, t->vely) / SC_PHYSICS_VELOCITY_SCL;
            sc_traj_free(&t);
            if(power < powermin) {
               /* Found a better solution. */
               anglemin = angle;
               powermin = power;
            } /* Is this sol'n better? */
         } /* Found a valid trajectory? */
      } /* Is the angle valid? */
      ++angle;
   }

   /* Did we find an acceptable solution? */   
   if(powermin <= SC_PLAYER_MAX_POWER) {
      /* Yes; set new trajectory and power. */
      sc_player_set_power(c, p, powermin);
      sc_player_set_turret(c, p, anglemin);
      p->ai->victim = victim;
      if(SC_AI_DEBUG_TRAJECTORY) {
         printf("AI_trajectory:   %s, %s line,wind to %d, %d with power=%d, angle=%d\n", 
                sc_ai_name(p->aitype), p->name, deltax, deltay, powermin, anglemin);
      }
      return(true);
   }

   /* No suitable trajectory found */
   return(false);

}



static bool _sc_ai_trajectory_scan(const sc_config *c, sc_player *p, const sc_player *victim, 
                                   int *newpower, int *newangle, int minangle, int maxangle, int deltaangle, 
                                   int minpower, int maxpower, int deltapower, double maxdist) {
/* _sc_ai_trajectory_scan
   Scans all possible turret angles and power levels, trying to find the
   best way to hit the intended victim.  This uses the real trajectory
   mechanism so this should take every possible contingency into account. */

   sc_trajectory_result result;
   sc_trajectory *t; /* Trajectory (temp variable) */
   int deltax;       /* Change in X, from player's position */
   int deltay;       /* Change in Y, from player's position */
   double dist;      /* Distance off from desired destination */
   int angle;        /* Current angle we are trying. */
   int power;        /* Power required to reach (dx,dy) */
   double distmin;   /* Best distance found... */
   int powermin;     /* Best power value found. */
   int anglemin;     /* Best angle found so far */
   int tx;           /* Targetted X coordinate */
   int ty;           /* Targetted Y coordinate */

   assert(c != NULL && p != NULL && victim != NULL);
   assert(newpower != NULL && newangle != NULL);
   assert(minangle <= maxangle);
   assert(minpower <= maxpower);
   assert(deltaangle > 0);
   assert(deltapower > 0);
   assert(maxdist > 0);
   assert(SC_PLAYER_IS_ALIVE(victim));
   assert(p->index != victim->index);

   /* Determine distance & angle between player and (dx,dy) */   
   if(!sc_land_calculate_deltas(c->land, &deltax, &deltay, p->x, p->y, victim->x, victim->y)) {
      deltax = victim->x - p->x;
      deltay = victim->y - p->y;
   }

   /* Compensation for shields */
   _sc_ai_trajectory_compensation(c, p, victim, &deltax, &deltay);

   /* Construct the desired target coordinate */
   tx = p->x + deltax;
   ty = p->y + deltay;

   /* Try to find a trajectory consuming the least amount of power. */   
   if(SC_AI_DEBUG_SCAN) {
      printf("aiscan(%d): scanning\n", p->index);
   }
   anglemin = 90;
   powermin = SC_PLAYER_MAX_POWER + 1;
   distmin  = 0;
   for(angle = minangle; angle <= maxangle; angle += deltaangle) {
      if(SC_AI_DEBUG_SCAN) {
         printf("aiscan(%d):  scanning angle %d\n", p->index, angle);
      }
      for(power = minpower; power < maxpower && power < SC_PLAYER_MAX_POWER; power += deltapower) {
         if(power >= SC_AI_SCAN_MIN_POWER) {
            t = sc_traj_new_power_angle(c, p, SC_TRAJ_NO_MODIFY, 
                                        sc_player_turret_x(p, angle), sc_player_turret_y(p, angle), 
                                        power * SC_PHYSICS_VELOCITY_SCL, angle);
            if(t != NULL) {
               result = sc_traj_run((sc_config *)c, t, SC_TRAJ_NO_MODIFY, NULL, NULL);
               if(SC_TRAJ_IS_IMPACT(result) && result != SC_TRAJ_IMPACT_SHIELD) {
                  /* Check to see how close the impact was */
                  if(sc_land_calculate_deltas(c->land, &deltax, &deltay, tx, ty, rint(t->curx), rint(t->cury))) {
                     dist = SQR((double)deltax) + SQR((double)deltay);
                     if(dist < distmin || powermin > SC_PLAYER_MAX_POWER) {
                        anglemin = angle;
                        powermin = power;
                        distmin  = dist;
                     } /* new optimal */
                  } else if(SC_AI_DEBUG_SCAN) {
                     printf("    %d, %d: no deltas!\n", power, angle);
                  }
               } else if(SC_AI_DEBUG_SCAN) {
                  printf("    %d, %d: no impact!\n", power, angle);
               }
               sc_traj_free(&t);
            } else if(SC_AI_DEBUG_SCAN) {
               printf("    %d, %d: no traj!\n", power, angle);
            }
         } /* min power satisfied? */
      } /* power loop */
   } /* angle loop */

   /* Did we find an acceptable solution? */   
   if(powermin <= SC_PLAYER_MAX_POWER && distmin >= 0 && distmin <= SQR(maxdist)) {
      /* Yes; set new trajectory and power. */
      *newpower = powermin;
      *newangle = anglemin;
      if(SC_AI_DEBUG_SCAN) {
         printf("aiscan(%d): done with %g at pow %d, ang %d\n", p->index, sqrt(distmin), powermin, anglemin);
      }
      return(true);
   }

   /* No suitable trajectory found */
   if(SC_AI_DEBUG_SCAN) {
      printf("aiscan(%d): done with %g, bad\n", p->index, sqrt(distmin));
   }
   return(false);

}



bool sc_ai_trajectory_scan(const sc_config *c, sc_player *p, const sc_player *victim) {
/* sc_ai_trajectory_scan
   Scan for an optimal trajectory to the specified victim.  Returns true if
   one was found, false otherwise.  */

   int power;
   int angle;

   assert(c != NULL && p != NULL && victim != NULL);
   assert(SC_PLAYER_IS_ALIVE(victim));
   assert(p->index != victim->index);

   power = p->power;
   angle = p->turret;
   if(SC_AI_DEBUG_SCAN) {
      printf("aiscan(%d): initial configuration is  pow %d  ang %d\n", p->index, power, angle);
   }

   if(!_sc_ai_trajectory_scan(c, p, victim, &power, &angle,
                              angle - 20, angle + 20, 10,
                              power - 100, power + 100, 50, 150)) return(false);
   if(!_sc_ai_trajectory_scan(c, p, victim, &power, &angle,
                              angle - 4, angle + 4, 1, 
                              power - 20, power + 20, 5, 50)) return(false);
   if(power == p->power && angle == p->turret) return(false);

   if(SC_AI_DEBUG_SCAN) {
      printf("aiscan(%d): accepted refinement to  pow %d  ang %d\n", p->index, power, angle);
   }
   sc_player_set_power(c, p, power);
   sc_player_set_turret(c, p, angle);
   p->ai->victim = victim;
   return(true);

}
